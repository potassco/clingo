#include <clingo/app.h>

#include <utility>

#include "control.hh"
#include "util.hh"

namespace Clingo::Python {

namespace {

struct Flag {
    bool value = false;
};

class Options {
  public:
    using Parser = std::function<bool(char const *value)>;
    using ParserList = std::forward_list<std::pair<Parser, clingo_lib_t *>>;

    Options(clingo_options_t *opts, ParserList &parsers, clingo_lib_t *lib)
        : opts_{opts}, parsers_{&parsers}, lib_{lib} {}

    void add(char const *group, char const *option, char const *description, Parser parser, bool multi,
             std::optional<char const *> argument) {
        parsers_->emplace_front(std::move(parser), lib_);
        static constexpr auto cparser = [](char const *value, void *data, bool *result) -> clingo_result_t {
            auto &[parser, ptr] = *static_cast<ParserList::value_type *>(data);
            CLINGO_TRY {
                *result = parser(value);
            }
            CLINGO_CATCH(ptr);
        };
        handle_error(clingo_options_add(opts_, group, option, description, cparser,
                                        static_cast<void *>(&parsers_->front()), multi,
                                        argument ? argument.value() : nullptr));
    }

    void add_flag(char const *group, char const *option, char const *description, Annotation<Flag> const &flag) {
        auto &cflag = flag.cast<Flag &>();
        handle_error(clingo_options_add_flag(opts_, group, option, description, &cflag.value));
    }

  private:
    //! The C options object.
    clingo_options_t *opts_;
    //! The list of parsers.
    ParserList *parsers_;
    //! The library object for error reporting.
    clingo_lib_t *lib_;
};

class App {
  public:
    App(std::string program_name, std::string version)
        : program_name_{std::move(program_name)}, version_{std::move(version)} {}
    App(App const &other) = delete;
    App(App &&other) = delete;
    auto operator=(App const &other) -> App & = delete;
    auto operator=(App const &&) -> App & = delete;

    void main(Annotation<Control> control, std::span<std::string const> files) {
        PYBIND11_OVERRIDE_NAME(void, App, "main", no_op_, control, files);
    }

    void print_model(Model model, std::function<void()> printer) {
        PYBIND11_OVERRIDE_NAME(void, App, "print_model", no_op_, model, printer);
    }

    void register_options(Options options) { PYBIND11_OVERRIDE_NAME(void, App, "register_options", no_op_, options); }

    void validate_options() { PYBIND11_OVERRIDE_NAME(void, App, "validate_options", no_op_); }

    auto program_name() -> char const * { return program_name_.c_str(); }

    auto version() -> char const * { return version_.c_str(); }

    auto lib() -> clingo_lib_t * { return lib_; }

    auto prepare(clingo_lib_t *lib) -> clingo_application_t {
        lib_ = lib;
        return {
            get_program_name_,
            get_version_,
            has_override_("main") ? &main_ : nullptr,
            has_override_("print_Model") ? print_model_ : nullptr,
            has_override_("register_options") ? register_options_ : nullptr,
            has_override_("validate_options") ? validate_options_ : nullptr,
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

    static auto main_(clingo_control_t *ctl, char const *const *files, size_t size, void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            auto pyctl = py::cast(Control(ctl));
            auto cfiles = std::span{files, size};
            app.main(pyctl, std::vector<std::string>{cfiles.begin(), cfiles.end()});
        }
        CLINGO_CATCH(app.lib());
    }

    static auto print_model_(clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data,
                             void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.print_model(Model{model}, [printer, printer_data]() { handle_error(printer(printer_data)); });
        }
        CLINGO_CATCH(app.lib());
    }

    static auto register_options_(clingo_options_t *options, void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.register_options(Options{options, app.parsers_, app.lib_});
        }
        CLINGO_CATCH(app.lib());
    }

    static auto validate_options_(void *data) -> clingo_result_t {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.validate_options();
        }
        CLINGO_CATCH(app.lib());
    }

    //! The list of option parsers.
    Options::ParserList parsers_;
    //! The applications name.
    std::string program_name_;
    //! The applications version.
    std::string version_;
    //! Lib object to report exceptions.
    clingo_lib_t *lib_ = nullptr;
};

auto pymain(Library const &lib, std::span<std::string const> arguments, std::optional<App *> app) -> int {
    auto capp = std::optional<clingo_application_t>{};
    if (app) {
        capp.emplace(app.value()->prepare(lib));
    }
    auto cargs = transform(arguments, [](auto const &x) { return x.c_str(); });
    return clingo_main(lib, cargs.data(), cargs.size(), capp ? &*capp : nullptr,
                       app ? static_cast<void *>(*app) : nullptr);
}

} // namespace

void register_app(pybind11::module &m) {
    using namespace Clingo::Python;

    auto app = m.def_submodule("app", R"(
TODO
)"_d);

    py::class_<Flag>(app, "Flag", R"(
TODO
)"_d)
        .def(py::init<bool>(), py::arg("value") = false, R"(
TODO
)"_d)
        .def_readwrite("value", &Flag::value, "Get/set the value of the flag.");

    py::class_<Options>(app, "AppOptions", R"(
TODO
)"_d)
        .def("add", &Options::add, py::arg("group"), py::arg("option"), py::arg("description"), py::arg("parser"),
             py::arg("multi") = false, py::arg("argument") = std::nullopt, R"(
TODO
)"_d)
        .def("add_flag", &Options::add_flag, py::arg("group"), py::arg("option"), py::arg("description"),
             py::arg("flag"), R"(
TODO
)"_d);

    py::class_<App>(app, "App", R"(
TODO
)"_d)
        .def(py::init<char const *, char const *>(), py::arg("program_name"), py::arg("version"), R"(
TODO
)"_d)
        .def("register_options", &App::register_options, py::arg("options"), R"(
TODO
)"_d)
        .def("validate_options", &App::validate_options, R"(
TODO
)"_d)
        .def("print_model", &App::print_model, py::arg("model"), py::arg("default_printer"), R"(
TODO
)"_d)
        .def("main", &App::main, py::arg("control"), py::arg("files"), R"(
TODO
)"_d);

    app.def("main", &pymain, py::arg("lib"), py::arg("arguments"), py::arg("app") = std::nullopt, R"(
TODO
)"_d);
}

} // namespace Clingo::Python
