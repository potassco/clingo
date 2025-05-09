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

/*
class App {
  public:
    App(std::optional<std::string> program_name, std::optional<std::string> version)
        : program_name_{std::move(program_name)}, version_{std::move(version)} {}
    App(App const &other) = delete;
    App(App &&other) = delete;
    auto operator=(App const &other) -> App & = delete;
    auto operator=(App const &&) -> App & = delete;

    void main(Annotation<Control> const &control, std::span<std::string const> files) {
        PYBIND11_OVERRIDE_NAME(void, App, "main", no_op_, control, files);
    }

    void print_model(Model model, std::function<void()> printer) {
        PYBIND11_OVERRIDE_NAME(void, App, "print_model", no_op_, model, printer);
    }

    void register_options(Options options) { PYBIND11_OVERRIDE_NAME(void, App, "register_options", no_op_, options); }

    void validate_options() { PYBIND11_OVERRIDE_NAME(void, App, "validate_options", no_op_); }

    auto program_name() -> char const * { return program_name_ ? program_name_->c_str() : "clingo"; }

    auto version() -> char const * {
        assert(version_);
        return version_->c_str();
    }

    auto prepare() -> clingo_application_t {
        return {
            program_name_ ? get_program_name_ : nullptr,
            version_ ? get_version_ : nullptr,
            has_override_("main") ? &main_ : nullptr,
            has_override_("print_model") ? print_model_ : nullptr,
            has_override_("register_options") ? register_options_ : nullptr,
            has_override_("validate_options") ? validate_options_ : nullptr,
        };
    }

    static void setup(PyHeapTypeObject *heap_type) {
        auto *type = &heap_type->ht_type;
        type->tp_flags |= Py_TPFLAGS_HAVE_GC;
        type->tp_traverse = [](PyObject *self_base, visitproc visit, void *arg) -> int {
            auto &self = py::cast<App &>(py::handle(self_base));
            for (auto const &parser : self.parsers_) {
                Py_VISIT(parser.ptr());
            }
            return 0;
        };
        type->tp_clear = [](PyObject *self_base) -> int {
            auto &self = py::cast<App &>(py::handle(self_base));
            self.parsers_.clear();
            return 0;
        };
    }

  private:
    template <class... Args> void no_op_([[maybe_unused]] Args const &...args) {}

    auto has_override_(char const *name) const -> bool { return bool(py::get_override(this, name)); }

    static auto get_program_name_(void *data) -> char const * {
        auto &app = *static_cast<App *>(data);
        try {
            return app.program_name();
        } catch (std::exception const &e) {
            printf("panic: %s\n", e.what());
            std::abort();
        }
    }

    static auto get_version_(void *data) -> char const * {
        auto &app = *static_cast<App *>(data);
        try {
            return app.version();
        } catch (std::exception const &e) {
            printf("panic: %s\n", e.what());
            std::abort();
        }
    }

    static auto main_(clingo_control_t *ctl, char const *const *files, size_t files_size, void *data)
        -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            auto pyctl = Control::cast(ctl, true);
            auto cfiles = std::span{files, files_size};
            app.main(pyctl, std::vector<std::string>{cfiles.begin(), cfiles.end()});
        }
        CLINGO_CATCH;
    }

    static auto print_model_(clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data,
                             void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.print_model(Model{model}, [printer, printer_data]() { handle_error(printer(printer_data)); });
        }
        CLINGO_CATCH;
    }

    static auto register_options_(clingo_options_t *options, void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.register_options(Options{options, app.parsers_});
        }
        CLINGO_CATCH;
    }

    static auto validate_options_(void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        try {
            app.validate_options();
        } catch (py::error_already_set &e) {
            // we report option validation errors here to avoid long winded
            // messages with traces later
            if (e.matches(PyExc_ValueError)) {
                std::string msg = py::str(e.value());
                fprintf(stderr, "*** ERROR: (%s): %s\n", app.program_name(), msg.c_str());
                return clingo_result_invalid;
            }
            return handle_error();
        } catch (...) {
            return handle_error();
        }
        return clingo_result_success;
    }

    //! The list of option parsers.
    Options::ParserList parsers_;
    //! The applications name.
    std::optional<std::string> program_name_;
    //! The applications version.
    std::optional<std::string> version_;
};

auto pyentry() -> int {
    py::module sys = py::module::import("sys");
    py::module app = py::module::import("clingo.app");
    py::module core = py::module::import("clingo.core");
    py::module script = py::module::import("clingo.script");

    py::object clingo_main = app.attr("clingo_main");
    py::object Library = core.attr("Library");
    py::object enable_python = script.attr("enable_python");

    auto argv = sys.attr("argv").attr("__getitem__")(py::slice{py::int_{1}, py::none(), py::none()}).cast<py::list>();
    py::object lib = Library();
    enable_python(lib);
    return py::cast<int>(clingo_main(lib, argv));
}

auto pymain(Library &lib, std::span<std::string const> arguments, std::optional<App *> app, bool raise_errors) -> int {
    auto capp = std::optional<clingo_application_t>{};
    if (app) {
        capp.emplace(app.value()->prepare());
    }
    auto cargs = transform(arguments, [](auto const &x) { return x.c_str(); });
    auto code = 0;
    auto ret = clingo_main(lib, cargs.data(), cargs.size(), capp ? &*capp : nullptr,
                           app ? static_cast<void *>(*app) : nullptr, &code);
    // NOTE: Clasp's main is noexcept, it will just report the exception and
    // return some arcane exit code. Hence, we simply check if an error has
    // been set and forward it here.
    try {
        handle_error_no_code(ret);
    } catch (py::error_already_set const &e) {
        if (raise_errors) {
            throw;
        }
        if (!is_clingo_error(e)) {
            auto const *name = app ? app.value()->program_name() : "clingo";
            fprintf(stderr, "*** ERROR: (%s): %s\n", name, e.what());
        }
    } catch (PyClingoError const &e) {
        if (raise_errors) {
            throw;
        }
    } catch (std::exception const &e) {
        if (raise_errors) {
            throw;
        }
        auto const *name = app ? app.value()->program_name() : "clingo";
        fprintf(stderr, "*** ERROR: (%s): %s\n", name, e.what());
    }
    clear_error();
    return code;
}
*/

} // namespace Clingo
