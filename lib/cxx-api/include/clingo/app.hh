#pragma once

#include <clingo/control.hh>

#include <clingo/app.h>

#include <forward_list>

namespace Clingo {

//! @addtogroup cpp_app
//! Support for building applications on top of clingo.
//!
//! @{

//! Object to add command-line options.
class Options {
  public:
    //! An option parser.
    //!
    //! The function takes the value to parse and returns whether the value has
    //! been parsed successfully.
    using Parser = std::function<bool(std::string_view value)>;
    //! A list of option parsers.
    using ParserList = std::forward_list<Parser>;

    //! Construct an options object.
    //!
    //! For internal use.
    //!
    //! @param opts the corresponding C type
    //! @param parsers a list to store option parsers in
    Options(clingo_options_t *opts, ParserList &parsers) : opts_{opts}, parsers_{&parsers} {}

    //! Convert the class to it's underlying C type.
    friend auto c_cast(Options const &x) -> clingo_options_t * { return x.opts_; }

    //! Add an option that is processed with a custom parser.
    //!
    //! Note that the parser that the parse is responsible to store the
    //! semantic value.
    //!
    //! Parameter option specifies the name(s) of the option. For example,
    //! "-p,ping" adds the short option "-p" and its long form "--ping". It is
    //! also possible to associate an option with a help level by prepending
    //! "@l" to the option specification. Options with a level greater than
    //! zero are only shown if the argument to help is greater or equal to l.
    //!
    //! @param group the section in the help output
    //! @param option the option specification
    //! @param description the option description
    //! @param parser a parser callback
    //! @param multi whether the option can be specified multiple times
    //! @param argument the value placeholder for the help output
    void add(std::string_view group, std::string_view option, std::string_view description, Parser parser,
             bool multi = false, std::optional<std::string_view> argument = std::nullopt) {
        parsers_->emplace_front(std::move(parser));
        static constexpr auto cparser = [](char const *value, size_t size, void *data) -> bool {
            auto &parser = *static_cast<Parser *>(data);
            CLINGO_TRY {
                parser({value, size});
            }
            CLINGO_CATCH;
        };
        Detail::handle_error(
            clingo_options_add(opts_, group.data(), group.size(), option.data(), option.size(), description.data(),
                               description.size(), cparser, static_cast<void *>(&parsers_->front()), multi,
                               argument ? argument->data() : nullptr, argument ? argument->size() : 0));
    }

    //! Add an option that is a simple flag.
    //!
    //! This function provides a simpler alternative to Options::add() to add
    //! flags, which do not take values. When the flag appears on the command
    //! line, the target parameter is set to true. Explicit values can also be
    //! provided using `--flag={yes|no}`.
    //!
    //! @param group the section in the help output
    //! @param option the option specification
    //! @param description the option description
    //! @param flag a reference to the semantic value
    void add_flag(std::string_view group, std::string_view option, std::string_view description, bool &flag) {
        Detail::handle_error(clingo_options_add_flag(opts_, group.data(), group.size(), option.data(), option.size(),
                                                     description.data(), description.size(), &flag));
    }

  private:
    clingo_options_t *opts_;
    ParserList *parsers_;
};

//! A callback to print the current model.
using ModelPrinter = std::function<void()>;

//! Interface to build applications on top of clingo.
class App {
  public:
    //! The default constructor.
    App() = default;
    //! Disable copying and moving.
    App(App &&other) = delete;
    //! Disable copying and moving.
    auto operator=(App &&other) -> App & = delete;
    //! The default destructor.
    virtual ~App() = default;

    //! Get the name of the application.
    //!
    //! Returns "clingo" by default.
    //!
    //! @return the name
    auto program_name() noexcept -> std::string_view { return do_program_name(); }

    //! Get the version of the application.
    //!
    //! Returns the clingo version by default.
    //!
    //! @return the version
    auto program_version() noexcept -> std::string_view { return do_version(); }

    //! Run the main control flow.
    //!
    //! Does nothing by default.
    //!
    //! @param control the control object
    //! @param files the files to parse
    void main(Control const &control, std::span<std::string_view const> files) { do_main(control, files); }

    //! Customize model printing.
    //!
    //! Calls the default model printer by default.
    //!
    //! @param model the current model
    //! @param printer the default model printer
    void print_model(ConstModel model, ModelPrinter const &printer) { do_print_model(model, printer); }

    //! Optionally register additional application options.
    //!
    //! @param options the options object
    void register_options(Options options) { do_register_options(options); }

    //! Validate previously parsed options.
    //!
    //! Should throw std::invalid_value if conflicting options are found.
    void validate_options() { do_validate_options(); }

  private:
    virtual void do_main(Control const &control, std::span<std::string_view const> files) {
        control.parse_files(files);
        control.main();
    }
    virtual void do_print_model([[maybe_unused]] ConstModel model, ModelPrinter const &printer) { printer(); }
    virtual void do_register_options([[maybe_unused]] Options options) {}
    virtual void do_validate_options() {}
    virtual auto do_program_name() noexcept -> std::string_view { return CLINGO_EXECUTABLE; }
    virtual auto do_version() noexcept -> std::string_view { return CLINGO_VERSION; }
};

namespace Detail {

struct AppData {
    App *app;
    Options::ParserList parsers;
};

static constexpr clingo_application_t c_app = {
    [](void *data, clingo_string_t *name) -> void {
        auto &app_data = *static_cast<AppData *>(data);
        auto str = app_data.app->program_name();
        name->data = str.data();
        name->size = str.size();
    },
    [](void *data, clingo_string_t *version) -> void {
        auto &app_data = *static_cast<AppData *>(data);
        auto str = app_data.app->program_version();
        version->data = str.data();
        version->size = str.size();
    },
    [](clingo_control_t *ctl, clingo_string_t const *files, size_t size, void *data) -> bool {
        auto &app_data = *static_cast<AppData *>(data);
        CLINGO_TRY {
            auto cpp_ctl = Control{ctl, true};
            auto cpp_files =
                transform(std::span{files, size}, [](auto const &x) { return std::string_view{x.data, x.size}; });
            app_data.app->main(cpp_ctl, cpp_files);
        }
        CLINGO_CATCH;
    },
    [](clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data, void *data) -> bool {
        auto &app_data = *static_cast<AppData *>(data);
        CLINGO_TRY {
            app_data.app->print_model(ConstModel{model},
                                      [printer, printer_data]() { handle_error(printer(printer_data)); });
        }
        CLINGO_CATCH;
    },
    [](clingo_options_t *options, void *data) -> bool {
        auto &app_data = *static_cast<AppData *>(data);
        CLINGO_TRY {
            app_data.app->register_options(Options{options, app_data.parsers});
        }
        CLINGO_CATCH;
    },
    [](void *data) -> bool {
        auto &app_data = *static_cast<AppData *>(data);
        CLINGO_TRY {
            app_data.app->validate_options();
        }
        CLINGO_CATCH;
    }};

} // namespace Detail

//! Run a clingo based application with the given arguments.
//!
//! @param lib the library to store symbols
//! @param arguments the command line arguments
//! @param app an optional application to customize solving
//! @param raise_errors whether to raise or report errors
//! @return the exit code
inline auto main(Library &lib, std::span<std::string_view const> arguments = {}, App *app = nullptr,
                 bool raise_errors = false) -> int {
    auto code = 1;
    try {
        auto c_args = Detail::transform(arguments, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        auto data = Detail::AppData{app, {}};
        Detail::handle_error_no_code(clingo_main(c_cast(lib), c_args.data(), c_args.size(),
                                                 app != nullptr ? &Detail::c_app : nullptr,
                                                 app != nullptr ? static_cast<void *>(&data) : nullptr, &code));
    } catch (std::exception const &e) {
        if (raise_errors) {
            throw;
        }
        auto name = app != nullptr ? app->program_name() : CLINGO_EXECUTABLE;
        fprintf(stderr, "*** ERROR: (%.*s): %s\n", static_cast<int>(name.size()), name.data(), e.what());
    }
    return code;
}

//! Run a clingo based application with the given arguments.
//!
//! Convenience overload to specify arguments via initializer_lists.
//!
//! @param lib the library to store symbols
//! @param arguments the command line arguments
//! @param app an optional application to customize solving
//! @param raise_errors whether to raise or report errors
//! @return the exit code
inline auto main(Library &lib, std::initializer_list<std::string_view const> arguments, App *app = nullptr,
                 bool raise_errors = false) -> int {
    return main(lib, std::span{arguments}, app, raise_errors);
}

//! @}

} // namespace Clingo
