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
    using ParserList = std::forward_list<std::pair<Annotation<Parser>, clingo_lib_t *>>;

    Options(clingo_options_t *opts, ParserList &parsers, clingo_lib_t *lib)
        : opts_{opts}, parsers_{&parsers}, lib_{lib} {}

    void add(char const *group, char const *option, char const *description, Annotation<Parser> parser, bool multi,
             std::optional<char const *> argument) {
        parsers_->emplace_front(std::move(parser), lib_);
        static constexpr auto cparser = [](char const *value, void *data, bool *result) -> clingo_result_t {
            auto &[parser, ptr] = *static_cast<ParserList::value_type *>(data);
            CLINGO_TRY {
                *result = py::cast<Parser>(parser)(value);
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

    auto program_name() -> char const * {
        assert(program_name_);
        return program_name_->c_str();
    }

    auto version() -> char const * {
        assert(version_);
        return version_->c_str();
    }

    auto lib() -> clingo_lib_t * { return lib_; }

    auto prepare(clingo_lib_t *lib) -> clingo_application_t {
        lib_ = lib;
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
            for (auto const &[parser, lib] : self.parsers_) {
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
    std::optional<std::string> program_name_;
    //! The applications version.
    std::optional<std::string> version_;
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
on option to print atoms in models in order:


```python
from typing import Callable, Sequence
import sys

from clingo.app import App, AppOptions, Flag, clingo_main
from clingo.core import Library
from clingo.control import Control
from clingo.solve import Model


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

    def main(self, control: Control, files: Sequence[str]) -> None:
        control.parse_files(files)
        control.main()


with Library() as lib:
    sys.exit(clingo_main(lib, sys.argv[1:], MyApp()))
```

)"_d);

    py::class_<Flag>(app, "Flag", R"(
Encapsulates a Boolean flag value with getter/setter functionality.

A flag is used to represent a binary command-line option in the application.
)"_d)
        .def(py::init<bool>(), py::arg("value") = false, R"(
Initializes the flag with the provided value.

Args:
    value:
        The initial boolean value of the flag (default is False).
)"_d)
        .def_readwrite("value", &Flag::value, "Get/set the value of the flag.");

    py::class_<Options>(app, "AppOptions", R"(
Manage application options and their definitions.

This class provides an interface to add various types of options (e.g., with
arguments, flags) which the application can use for configuration.
)"_d)
        .def("add", &Options::add, py::arg("group"), py::arg("option"), py::arg("description"), py::arg("parser"),
             py::arg("multi") = false, py::arg("argument") = std::nullopt, R"(
Adds an option with a corresponding parser to the options set.

An option's group name acts like a section header; all options with the same
group name are displayed under it in the help output. The option name is the
identifier following the two dashes on the command line and it's value is
parsed by the given parser.

Args:
    group:
        The option group.
    option:
        The option name.
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
Adds a flag option to the options set.

Similar to add_option but used for Boolean options that can be toggled on or
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
valid. If an error is detected, an exception may be raised.
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

    app.def("clingo_main", &pymain, py::arg("lib"), py::arg("arguments"), py::arg("app") = std::nullopt, R"(
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
)"_d);
}

} // namespace Clingo::Python
