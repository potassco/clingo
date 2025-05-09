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
    virtual auto do_program_name() noexcept -> std::string_view { return CLINGO_EXECUTABLE; }
    virtual auto do_version() noexcept -> std::string_view { return CLINGO_VERSION; }
};

namespace Detail {

struct AppData {
    App *app;
    Options::ParserList parsers;
    std::exception_ptr ptr;
    std::optional<Control> ctl;

    auto active_ptr() -> std::exception_ptr & { return ctl ? ctl->data_().ptr : ptr; }
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
    [](clingo_control_t *ctl, clingo_string_t const *files, size_t size, void *data) -> clingo_result_t {
        auto &app_data = *static_cast<AppData *>(data);
        CLINGO_TRY {
            app_data.ctl.emplace(ctl, true);
            auto cpp_files =
                transform(std::span{files, size}, [](auto const &x) { return std::string_view{x.data, x.size}; });
            app_data.app->main(*app_data.ctl, cpp_files);
        }
        CLINGO_CATCH;
    },
    [](clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data,
       void *data) -> clingo_result_t {
        auto &app_data = *static_cast<AppData *>(data);
        CLINGO_TRY {
            app_data.app->print_model(ConstModel{model},
                                      [printer, printer_data]() { handle_error(printer(printer_data)); });
        }
        CLINGO_CATCH_PTR(app_data.active_ptr());
    },
    [](clingo_options_t *options, void *data) -> clingo_result_t {
        auto &app_data = *static_cast<AppData *>(data);
        CLINGO_TRY {
            app_data.app->register_options(Options{options, app_data.parsers});
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
        auto data = Detail::AppData{app, {}, {}, {}};
        Detail::handle_error(clingo_main(c_cast(lib), c_args.data(), c_args.size(),
                                         app != nullptr ? &Detail::c_app : nullptr,
                                         app != nullptr ? static_cast<void *>(&data) : nullptr, &code));
        auto &ptr = data.active_ptr();
        auto &gptr = Detail::get_exception_ptr();
        if (ptr != nullptr) {
            gptr = nullptr;
            std::rethrow_exception(std::exchange(ptr, nullptr));
        }
        if (gptr != nullptr) {
            std::rethrow_exception(std::exchange(gptr, nullptr));
        }
    } catch ([[maybe_unused]] Detail::clingo_error const &e) {
        // NOTE: clingo errors should have been reported by clasp already
        if (raise_errors) {
            throw;
        }
    } catch (std::exception const &e) {
        if (raise_errors) {
            throw;
        }
        auto name = app != nullptr ? app->program_name() : CLINGO_EXECUTABLE;
        fprintf(stderr, "*** ERROR: (%.*s): %s\n", (int)name.size(), name.data(), e.what());
    }
    return code;
}

} // namespace Clingo
