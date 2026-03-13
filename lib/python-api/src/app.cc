#include <iostream>
#include <utility>

#include "app.hh"
#include "control.hh"
#include "util.hh"

namespace PyClingo {

namespace {

struct Flag {
    bool value = false;
};

class Options {
  public:
    using Parser = std::function<void(std::string_view)>;

    Options(clingo_options_t *opts, py::list &parsers) : opts_{opts}, parsers_{&parsers} {}

    void add(std::string_view group, std::string_view option, std::string_view description, Annotation<Parser> parser,
             bool multi, std::optional<std::string_view> argument) {
        parsers_->append(parser);
        static constexpr auto cparser = [](char const *value, size_t size, void *data) -> bool {
            auto parser = py::handle{static_cast<PyObject *>(data)};
            CLINGO_TRY {
                py::cast<Parser>(parser)({value, size});
            }
            CLINGO_CATCH;
        };
        handle_error(clingo_options_add(opts_, group.data(), group.size(), option.data(), option.size(),
                                        description.data(), description.size(), cparser,
                                        static_cast<void *>(parser.ptr()), multi, argument ? argument->data() : nullptr,
                                        argument ? argument->size() : 0));
    }

    void add_flag(std::string_view group, std::string_view option, std::string_view description,
                  Annotation<Flag> const &flag) {
        auto &cflag = flag.cast<Flag &>();
        handle_error(clingo_options_add_flag(opts_, group.data(), group.size(), option.data(), option.size(),
                                             description.data(), description.size(), &cflag.value));
    }

    void set_default_value(std::string_view option, std::string_view value) {
        handle_error(clingo_options_set_default_value(opts_, option.data(), option.size(), value.data(), value.size()));
    }

    auto c_ptr() -> clingo_options_t * { return opts_; }

  private:
    //! The C options object.
    clingo_options_t *opts_;
    //! The list of parsers.
    py::list *parsers_;
};

class App : public reference_keeper<App> {
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

    auto program_name() -> std::string_view {
        return program_name_ ? std::string_view{*program_name_} : CLINGO_EXECUTABLE;
    }

    auto version() -> std::string_view {
        assert(version_);
        return *version_;
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

  private:
    template <class... Args> void no_op_([[maybe_unused]] Args const &...args) {}

    auto has_override_(char const *name) const -> bool { return bool(py::get_override(this, name)); }

    static void get_program_name_(void *data, clingo_string_t *res) {
        auto &app = *static_cast<App *>(data);
        try {
            auto str = app.program_name();
            res->data = str.data();
            res->size = str.size();
        } catch (std::exception const &e) {
            fprintf(stderr, "panic: %s\n", e.what());
            std::terminate();
        }
    }

    static void get_version_(void *data, clingo_string_t *res) {
        auto &app = *static_cast<App *>(data);
        try {
            auto str = app.version();
            res->data = str.data();
            res->size = str.size();
        } catch (std::exception const &e) {
            fprintf(stderr, "panic: %s\n", e.what());
            std::terminate();
        }
    }

    static auto main_(clingo_control_t *ctl, clingo_string_t const *files, size_t size, void *data) -> bool {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            auto pyctl = Control::cast(ctl, true);
            auto pyfiles = transform(std::span{files, size}, [](auto const &x) { return std::string{x.data, x.size}; });
            app.main(pyctl, pyfiles);
        }
        CLINGO_CATCH;
    }

    static auto print_model_(clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data,
                             void *data) -> bool {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            auto acquire = py::gil_scoped_acquire{};
            // NOTE: Python seems to directly write large buffers
            std::cout.flush();
            app.print_model(Model{model}, [printer, printer_data]() { handle_error(printer(printer_data)); });
            py::module_::import("sys").attr("stdout").attr("flush")();
        }
        CLINGO_CATCH;
    }

    static auto register_options_(clingo_options_t *options, void *data) -> bool {
        auto &app = *static_cast<App *>(data);
        CLINGO_TRY {
            app.register_options(Options{options, app.list_});
        }
        CLINGO_CATCH;
    }

    static auto validate_options_(void *data) -> bool {
        auto &app = *static_cast<App *>(data);
        try {
            app.validate_options();
        } catch (py::error_already_set &e) {
            if (e.matches(PyExc_ValueError)) {
                auto str = app.program_name();
                std::string msg = py::str(e.value());
                fprintf(stderr, "*** ERROR: (%.*s): %s\n", (int)str.size(), str.data(), msg.c_str());
            }
            return store_error();
        } catch (...) {
            return store_error();
        }
        return true;
    }

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

auto pymain(Library &lib, std::span<std::string const> arguments, std::optional<App *> app) -> int {
    auto capp = std::optional<clingo_application_t>{};
    if (app) {
        capp.emplace(app.value()->prepare());
    }
    auto cargs = transform(arguments, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
    auto code = 0;
    auto ret = clingo_main(lib, cargs.data(), cargs.size(), capp ? &*capp : nullptr,
                           app ? static_cast<void *>(*app) : nullptr, &code);
    // NOTE: Clasp's main is noexcept, it will report whether an exception was
    // caught via the exit code. Whenever an exception is rethrown from within
    // the internal C++ API, the clingo error will be cleared and *only* be
    // reported by clasp. In this case, no clingo error is set but the exit
    // code indicates an error.
    try {
        handle_error_no_code(ret, code);
    } catch (py::error_already_set const &e) {
        auto name = app ? app.value()->program_name() : CLINGO_EXECUTABLE;
        fprintf(stderr, "*** ERROR: (%.*s): %s\n", (int)name.size(), name.data(), e.what());
    } catch (std::exception const &e) {
        auto name = app ? app.value()->program_name() : CLINGO_EXECUTABLE;
        fprintf(stderr, "*** ERROR: (%.*s): %s\n", (int)name.size(), name.data(), e.what());
    }
    return code;
}

} // namespace

auto convert_options(py::handle hnd) -> clingo_options_t * {
    return hnd.cast<Options &>().c_ptr();
}

void register_app(pybind11::module &m) {
    using namespace PyClingo;

    auto app = m.def_submodule("app", R"(
Module to create custom clingo-based applications.

This module provides application-level functionality for working with clingo,
including an application class, options definitions, and helper routines.

# Examples

The following example shows how to run clingo without customization:

```python
import sys
from clingo.app import clingo_main
from clingo.core import Library
from clingo.script import enable_python

with Library() as lib:
    enable_python(lib)
    clingo_main(lib, sys.argv[1:])
```

The next example shows how to write a simple clingo-based application that adds
an option to print atoms in models in order:

```python
from typing import Callable, Sequence
import sys
from clingo.app import App, AppOptions, Flag, clingo_main
from clingo.core import Library
from clingo.control import Control
from clingo.solve import Model
from clingo.symbol import Symbol

Parts = Sequence[Sequence[tuple[str, Sequence[Symbol]]]]

class MyApp(App):
    def __init__(self) -> None:
        super().__init__("my-app", "1.0.0")
        self._order = Flag()

    def print_model(self, model: Model, default_printer: Callable[[], None]) -> None:
        if self._order.value:
            print(" ".join(str(sym) for sym in sorted(model.symbols(shown=True))))
        else:
            default_printer()

    def register_options(self, options: AppOptions) -> None:
        options.add_flag(
            "MyApp", "order", "Print atoms in models in order.", self._order
        )

    def main(self, control: Control, files: Sequence[str], parts: Parts) -> None:
        control.parse_files(files)
        control.main(parts)

with Library() as lib:
    sys.exit(clingo_main(lib, sys.argv[1:], MyApp()))
```
)"_d);

    py::class_<Flag>(app, "Flag", R"(
Boolean flag with value management.

Represents command-line toggle options.
)"_d)
        .def(py::init<bool>(), py::arg("value") = false, R"(
Initializes the flag with the provided value.

Args:
    value:
        The initial boolean value of the flag (default is False).
)"_d)
        .def_readwrite("value", &Flag::value, "Get/set the value of the flag.");

    py::class_<Options>(app, "AppOptions", R"(
Manager for application options and their definitions.

Provides interface to add/configures various option types:
- argument options,
- flag options, and
- multi-value options.
)"_d)
        .def("add", &Options::add, py::arg("group"), py::arg("option"), py::arg("description"), py::arg("parser"),
             py::arg("multi") = false, py::arg("argument") = std::nullopt, R"(
Adds an option with a custom parser.

An option's group name acts like a section header; all options with the same
group name are displayed under it in the help output. The option name is the
identifier following the two dashes on the command line and it's value is
parsed by the given parser.

Args:
    group:
        The option group or category.
    option:
        The option name (after --).
    description:
        A brief description of the option.
    parser:
        A callable to process the string input for this option.
    multi:
        Whether the option can accept multiple values (default is False).
    argument:
        An optional string indicating the argument type or format.
)"_d)
        .def("add_flag", &Options::add_flag, py::arg("group"), py::arg("option"), py::arg("description"),
             py::arg("flag"), R"(
Add a Boolean flag option.

Similar to `add_option` but used for Boolean options that can be toggled on or
off.

Args:
    group:
        The option group or category.
    option:
        The option name or flag identifier.
    description:
        A brief description of the flag option.
    flag:
        A Flag object that holds the value of the flag.
)"_d)
        .def("set_default_value", &Options::set_default_value, py::arg("option"), py::arg("value"), R"(
Set the default value for an existing clingo option.

This function can be used to adjust the default value of a clingo option, which
will be used if no value is given on the command-line.

Args:
    option:
        The name of the option for which a default value should be set.
    value:
        The new default value to set.
)"_d);

    py::class_<App>(app, "App", py::custom_type_setup(&App::setup), R"(
Interface to implement a custom Clingo-based application.

This class encapsulates the main execution flow of a Clingo-based application.
It provides methods for executing the program, printing models, registering
application options, and validating the configuration.
)"_d)
        .def(py::init<std::optional<std::string>, std::optional<std::string>>(), py::arg("program_name") = std::nullopt,
             py::arg("version") = std::nullopt, R"(
Initializes the application with a program name and its version.

If no name is given, `"clingo"` is used. If no version is given, the current
clingo version is used.

Args:
    program_name:
        The name of the application.
    version:
        The version string of the application.
)"_d)
        .def("register_options", &App::register_options, py::arg("options"), R"(
Register command-line options for the application.

Args:
	options:
		An instance of AppOptions to add new options.
)"_d)
        .def("validate_options", &App::validate_options, R"(
Validate the options passed to the application.

Once the application options have been set, this method confirms that they are
valid. If an error is detected, a ValueError should be raised.
)"_d)
        .def("print_model", &App::print_model, py::arg("model"), py::arg("default_printer"), R"(
Print the given model in a custom format.

The default printer can be used to print the model as clingo would. A possible
use case would be to use the printer and then print additional information
below the model.

Args:
	model:
		The current model held by the solver.
	default_printer:
		A callable that prints the model in default format.
)"_d)
        .def("main", &App::main, py::arg("control"), py::arg("files"), R"(
Run the main execution flow of the application.

This method is invoked after Clingo's control object has been configured. It
processes input files, executes the control flow, and handles output.

Args:
    control:
        The Clingo control object for managing grounding and solving.
    files:
        A list of filenames representing the input logic programs.
)"_d);

    app.def("clingo_main", &pymain, py::arg("lib"), py::arg("arguments"), py::arg("app") = std::nullopt,
            R"(
Entry point for running the Clingo application.

This function initializes necessary components, processes input arguments, and
then executes the main functionality of the Clingo application. It can
optionally use a provided App instance to customize this behavior.

Args:
	lib:
		The Clingo core library interface.
	arguments:
		A list of command-line arguments.
	app:
		An optional App instance containing application-specific logic.

Returns:
    An integer exit code.
)"_d)
        .def("_pyclingo", &pyentry);
}

} // namespace PyClingo
