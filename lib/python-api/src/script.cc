// Note that the scripts in this class report errors via the libraries logger.
// This design is somewhat unfortunate but proper forwarding of errors here
// would be tough. In practice, it is best to avoid scripts and instead use
// the context object of the ground call that supports error forwarding.
#include "script.hh"
#include "control.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/script.h>

#include <pybind11/embed.h>
#include <pybind11/eval.h>

namespace Clingo::Python {

namespace {

class Interpreter {
  public:
    Interpreter() {
        if (Py_IsInitialized() == 0) {
            py_ = std::make_unique<py::scoped_interpreter>();
            py::module::import("clingo");
        }
        auto gil = py::gil_scoped_acquire{};
        scope_ = py::module_::import("__main__").attr("__dict__");
        callable_ = py::module_::import("builtins").attr("callable");
        version_ = py::module_::import("sys").attr("version").cast<std::string>();
    }

    void exec(char const *code) {
        auto gil = py::gil_scoped_acquire{};
        py::exec(code, scope_);
    }

    auto callable(char const *name) -> bool {
        auto gil = py::gil_scoped_acquire{};
        return scope_.contains(name) && callable_(scope_[name]).cast<bool>();
    }

    auto call(Library lib, char const *name, SymbolVec args) -> std::variant<SymbolVec, Symbol> {
        auto gil = py::gil_scoped_acquire{};
        return scope_[name](&lib, *py::cast(args)).cast<std::variant<SymbolVec, Symbol>>();
    }

    auto main(Library lib, Control ctl, PartsSpan parts) {
        auto gil = py::gil_scoped_acquire{};
        scope_["main"](&lib, &ctl, parts);
    }

    auto version() -> char const * { return version_.c_str(); }

  private:
    std::unique_ptr<py::scoped_interpreter> py_;
    py::object scope_;
    py::object callable_;
    std::string version_;
};

class MainScript {
  public:
    MainScript(clingo_lib_t *lib) : lib_{lib} {
        // NOTE: Initialize right away if the interpreter is already running.
        // If the interpreter is not yet running, it will be started as soon as
        // Python code is executed. If the intepreter is running, Python
        // functions must be callable even if no code is executed.
        if (Py_IsInitialized() != 0) {
            init_();
        }
    }

    static auto cast(void *data) -> MainScript * { return static_cast<MainScript *>(data); }

    static auto c_execute(char const *code, void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY {
            self->init_();
            self->py_->exec(code);
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_call(clingo_lib_t *lib, clingo_location_t const *loc, char const *name,
                       clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data)
        -> clingo_result_t {
        // Note that the location could in principle be used for better error reporting.
        static_cast<void>(loc);
        auto *self = cast(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            if (self->py_) {
                auto syms = self->py_->call(lib, name, args);
                return std::visit(
                    [&]<class T>(T const &res) {
                        if constexpr (std::is_same_v<T, Symbol>) {
                            // NOLINTNEXTLINE
                            auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(&res);
                            return symbol_callback(c_syms, 1, symbol_callback_data);
                        } else {
                            // NOLINTNEXTLINE
                            auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(res.data());
                            return symbol_callback(c_syms, res.size(), symbol_callback_data);
                        }
                    },
                    syms);
            }
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        // Note: that python cannot check the number of arguments
        static_cast<void>(arguments);
        auto *self = cast(data);
        CLINGO_TRY {
            *result = self->py_ && self->py_->callable(name);
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto main(clingo_lib_t *lib, clingo_control_t *control, clingo_parts_array_t const *parts, size_t size,
                     void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY {
            if (self->py_) {
                self->py_->main(lib, control, std::span{parts, size});
            }
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_name(void *data) -> char const * {
        static_cast<void>(data);
        return "python";
    }

    static auto c_version(void *data) -> char const * {
        static_cast<void>(data);
        return CLINGO_PYTHON_VERSION;
    }

    static void c_free(void *data) {
        // NOLINTNEXTLINE
        delete cast(data);
    }

  private:
    void init_() {
        if (!py_) {
            py_ = std::make_unique<Interpreter>();
        }
    }

    std::unique_ptr<Interpreter> py_;
    clingo_lib_t *lib_;
};

class Script {
  public:
    void execute(char const *code) { PYBIND11_OVERRIDE_PURE(void, Script, execute, code); }

    static auto get_self(void *data) -> Script & { return py::handle{static_cast<PyObject *>(data)}.cast<Script &>(); }

    static auto c_execute(char const *code, void *data) -> clingo_result_t {
        auto &self = get_self(data);
        CLINGO_TRY {
            self.execute(code);
        }
        CLINGO_CATCH(self.lib_);
    }

    auto call(Library lib, char const *name, SymbolVec const &args) -> std::variant<SymbolVec, Symbol> {
        PYBIND11_OVERRIDE_PURE(SymbolVec, Script, call, &lib, name, args);
    }

    static auto c_call(clingo_lib_t *lib, clingo_location_t const *loc, char const *name,
                       clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data)
        -> clingo_result_t {
        // Note that the location could in principle be used for better error reporting.
        static_cast<void>(loc);
        auto &self = get_self(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            auto syms = self.call(lib, name, args);
            return std::visit(
                [&]<class T>(T const &res) {
                    if constexpr (std::is_same_v<T, Symbol>) {
                        // NOLINTNEXTLINE
                        auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(&res);
                        return symbol_callback(c_syms, 1, symbol_callback_data);
                    } else {
                        // NOLINTNEXTLINE
                        auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(res.data());
                        return symbol_callback(c_syms, res.size(), symbol_callback_data);
                    }
                },
                syms);
        }
        CLINGO_CATCH(self.lib_);
    }

    auto callable(char const *name, size_t arguments) -> bool {
        PYBIND11_OVERRIDE_PURE(bool, Script, callable, name, arguments);
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        auto &self = get_self(data);
        CLINGO_TRY {
            auto &self = get_self(data);
            *result = self.callable(name, arguments);
        }
        CLINGO_CATCH(self.lib_);
    }

    void main(Library lib, Control &ctl, PartsSpan parts) {
        PYBIND11_OVERRIDE_PURE(void, Script, main, &lib, &ctl, &parts);
    }

    static auto c_main(clingo_lib_t *lib, clingo_control_t *control, clingo_parts_array_t const *parts, size_t size,
                       void *data) -> clingo_result_t {
        auto &self = get_self(data);
        CLINGO_TRY {
            auto py_ctl = Control{control};
            self.main(lib, py_ctl, std::span{parts, size});
        }
        CLINGO_CATCH(self.lib_);
    }

    auto name() -> std::string { PYBIND11_OVERRIDE_PURE(std::string, Script, name); }

    static auto c_name(void *data) -> char const * {
        try {
            auto &self = get_self(data);
            if (self.name_.empty()) {
                self.name_ = self.name();
            }
            return self.name_.c_str();
        } catch (...) {
            return "<error>";
        }
    }

    auto version() -> std::string { PYBIND11_OVERRIDE_PURE(std::string, Script, version); }

    static auto c_version(void *data) -> char const * {
        try {
            auto &self = get_self(data);
            if (self.version_.empty()) {
                self.version_ = self.version();
            }
            return self.version_.c_str();
        } catch (...) {
            return "<error>";
        }
    }

    void lib(clingo_lib_t *lib) { lib_ = lib; }

  private:
    clingo_lib_t *lib_{};
    std::string name_;
    std::string version_;
};

void reg_script(Library &lib, Annotation<Script> script) {
    py::cast<Script &>(script).lib(lib);
    auto *ptr = lib.add_object(std::move(script));
    auto c_script =
        clingo_script_t{Script::c_execute, Script::c_call, Script::c_callable, Script::c_main, Script::c_name,
                        Script::c_version, nullptr};
    handle_error(clingo_script_register(lib, &c_script, ptr));
}

} // namespace

auto register_python(clingo_lib_t *lib) -> clingo_result_t {
    using Script = Clingo::Python::MainScript;
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    auto script = std::make_unique<Script>(lib);
    return clingo_script_register(lib, &c_script, script.release());
}

void register_script(pybind11::module &m) {
    auto script = m.def_submodule("script", R"(
Module containing functions to add custom scripts, which can be embedded into
logic programs.

# Examples

The following example shows how to register a custom python script that executes functions
from the main context (just like the embedded one in the standalone clingo).

```python
>>> import __main__
>>> from clingo.control import Control
>>> from clingo.core import Library
>>> from clingo.script import Script, register
>>> from clingo.symbol import Number, Symbol
...
>>> class PyScript(Script):
...     def execute(self, code: str) -> None:
...         exec(code, __main__.__dict__, __main__.__dict__)
...     def call(self, lib: Library, name: str, arguments: list[Symbol]) -> list[Symbol]:
...         return [getattr(__main__, name)(lib, *arguments)]
...     def callable(self, name: str, args: int) -> bool:
...         return name in __main__.__dict__ and callable(__main__.__dict__[name])
...     def main(self, lib: Library, control: Control) -> None:
...         __main__.main(lib, control)
...     def name(self):
...         return "python"
...
>>> def f(lib: Library, x: Symbol) -> Symbol:
...     return Number(lib, x.number * 3)
...
>>> lib = Library()
>>> register(lib, PyScript())
>>> ctl = Control(lib, ["--mode=ground"])
>>> ctl.parse_string("""\
... #script (python)
...
... from clingo.symbol import Number, Symbol
...
... def g(lib: Library, x: Symbol) -> Symbol:
...     return Number(lib, x.number * 4)
...
... #end.
... p(@f(1)).
... q(@g(2)).
... """)
>>> ctl.ground()
>>> ctl.buffer
"""\
p(3).
q(8).
#show p/1.
#show q/1.
#show.
"""
```
)");
    py::class_<Script>(script, "Script", R"(ABC for custom scripts.)")
        .def(py::init<>(), R"(Construct a script object.)")
        .def("execute", &Script::execute, py::arg("code"), R"(
Execute the given code.

Args:
    code:
        The code to execute.
)")
        .def("call", &Script::call, py::arg("lib"), py::arg("name"), py::arg("arguments"), R"(
Call the function with the given name and arguments.

Args:
    lib:
        The library object to store symbols.
    name:
        The name of the function.
    arguments:
        The arguments of the function.

Returns:
    A list of symbols.
)")
        .def("callable", &Script::callable, py::arg("name"), py::arg("arguments"), R"(
Check if the function with the given signature is callable.

Args:
    name:
        The name of the function.
    arguments:
        The number of arguments of the function.

Returns:
    Whether the function is callable.
)")
        .def("main", &Script::main, py::arg("lib"), py::arg("control"), py::arg("parts"), R"(
Run the main function.

Args:
    lib:
        The (main) library object.
    control:
        The (main) control object.
    parts:
        The parts to ground and solve.
)")
        .def("name", &Script::name, R"(Get the name of the script.)")
        .def("version", &Script::version, R"(Get the version of the script.)");

    script
        .def("register", &reg_script, py::arg("lib"), py::arg("script"), R"(
Registers a script language which can then be embedded into a logic program.

Args:
    lib:
        The library to register the script with.
    script:
        The script to register.
)")
        .def("enable_python", [](Library &lib) { handle_error(register_python(lib)); }, py::arg("lib"), R"(
Enable embedded python scripts.

Args:
    lib:
        The library to register the script with.
)"_d);
}

} // namespace Clingo::Python
