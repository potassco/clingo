#pragma once

#include <clingo/control.hh>

#include <clingo/app.h>

namespace Clingo {

class Options {
  public:
    using Parser = std::function<bool(std::string_view value)>;
    using ParserList = std::forward_list<Parser>;

    Options(clingo_options_t *opts, ParserList &parsers) : opts_{opts}, parsers_{&parsers} {}

    friend auto c_cast(Options const &x) -> clingo_options_t * { return x.opts_; }

    void add(std::string_view group, std::string_view option, std::string_view description, Parser parser, bool multi,
             std::optional<std::string_view> argument) {
        parsers_->emplace_front(std::move(parser));
        static constexpr auto cparser = [](char const *value, size_t size, void *data,
                                           bool *result) -> clingo_result_t {
            auto &parser = *static_cast<Parser *>(data);
            CLINGO_TRY {
                *result = parser({value, size});
            }
            CLINGO_CATCH;
        };
        Detail::handle_error(
            clingo_options_add(opts_, group.data(), group.size(), option.data(), option.size(), description.data(),
                               description.size(), cparser, static_cast<void *>(&parsers_->front()), multi,
                               argument ? argument->data() : nullptr, argument ? argument->size() : 0));
    }

    void add_flag(std::string_view group, std::string_view option, std::string_view description, bool &flag) {
        Detail::handle_error(clingo_options_add_flag(opts_, group.data(), group.size(), option.data(), option.size(),
                                                     description.data(), description.size(), &flag));
    }

  private:
    clingo_options_t *opts_;
    ParserList *parsers_;
};

using ModelPrinter = std::function<void()>;

class App {
  public:
    virtual ~App() = default;
    auto program_name() noexcept -> std::string_view { return do_program_name(); }
    auto program_version() noexcept -> std::string_view { return do_version(); }
    void main(Control const &control, std::span<std::string_view const> files) { do_main(control, files); }
    void print_model(ConstModel model, ModelPrinter const &printer) { do_print_model(model, printer); }
    void register_options([[maybe_unused]] Options options) {}
    void validate_options() { do_validate_options(); }

  private:
    virtual void do_main(Control const &control, std::span<std::string_view const> files) {
        control.parse_files(files);
        control.main();
    }
    virtual void do_print_model([[maybe_unused]] ConstModel model, ModelPrinter const &printer) { printer(); }
    virtual void do_register_options([[maybe_unused]] Options options) {}
    virtual void do_validate_options() {}
    virtual auto do_program_name() noexcept -> std::string_view { return "clingo"; }
    virtual auto do_version() noexcept -> std::string_view { return CLINGO_VERSION; }
};

namespace Detail {

// FIXME: put somewhere else
static inline Options::ParserList parsers_; // NOLINT

static constexpr clingo_application_t c_app = {
    [](void *data, clingo_string_t *name) -> void {
        auto &app = *static_cast<App *>(data);
        auto str = app.program_name();
        name->data = str.data();
        name->size = str.size();
    },
    [](void *data, clingo_string_t *version) -> void {
        auto &app = *static_cast<App *>(data);
        auto str = app.program_version();
        version->data = str.data();
        version->size = str.size();
    },
    [](clingo_control_t *ctl, clingo_string_t const *files, size_t size, void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            auto cpp_ctl = Control(ctl, true);
            auto cpp_files =
                transform(std::span{files, size}, [](auto const &x) { return std::string_view{x.data, x.size}; });
            app.main(cpp_ctl, cpp_files);
        }
        CLINGO_CATCH;
    },
    [](clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data,
       void *data) -> clingo_result_t {
        // FIXME: the print_model callback does not necessarily run in the main thread and thus has to store the error
        // in a custom exception pointer.
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.print_model(ConstModel{model}, [printer, printer_data]() { handle_error(printer(printer_data)); });
        }
        CLINGO_CATCH;
    },
    [](clingo_options_t *options, void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.register_options(Options{options, parsers_});
        }
        CLINGO_CATCH;
    },
    [](void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        try {
            app.validate_options();
        } catch (std::invalid_argument &e) {
            // Report option validation errors right away.
            auto str = app.program_name();
            fprintf(stderr, "*** ERROR: (%.*s): %s\n", (int)str.size(), str.data(), e.what());
            return clingo_result_invalid;
        } catch (...) {
            Detail::get_exception_ptr() = std::current_exception();
            return clingo_result_unknown;
        }
        return clingo_result_success;
    }};

} // namespace Detail

auto main(Library &lib, std::span<std::string_view const> arguments, App *app = nullptr, bool raise_errors = false)
    -> int {
    auto code = 1;
    try {
        auto c_args = Detail::transform(arguments, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        Detail::handle_error(clingo_main(c_cast(lib), c_args.data(), c_args.size(),
                                         app != nullptr ? &Detail::c_app : nullptr,
                                         app != nullptr ? static_cast<void *>(app) : nullptr, &code));
        auto &ptr = Detail::get_exception_ptr();
        if (ptr != nullptr) {
            std::rethrow_exception(ptr);
        }
    } catch (std::exception const &e) {
        // NOTE: already reported exceptions are included here as well
        if (raise_errors) {
            throw;
        }
        auto name = app != nullptr ? app->program_name() : "clingo";
        fprintf(stderr, "*** ERROR: (%.*s): %s\n", (int)name.size(), name.data(), e.what());
    }
    return code;
}

} // namespace Clingo
