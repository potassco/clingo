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

#include <utility>

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

    void exec(char const *code) { py::exec(code, scope_); }

    auto callable(char const *name) -> bool { return scope_.contains(name) && callable_(scope_[name]).cast<bool>(); }

    auto call(const Annotation<Library> &lib, char const *name, SymbolVec args) -> std::variant<SymbolVec, Symbol> {
        return scope_[name](lib, *py::cast(args)).cast<std::variant<SymbolVec, Symbol>>();
    }

    auto main(const Annotation<Library> &lib, Control ctl, PartsSpan parts) { scope_["main"](lib, &ctl, parts); }

    auto version() -> char const * { return version_.c_str(); }

  private:
    std::unique_ptr<py::scoped_interpreter> py_;
    py::object scope_;
    py::object callable_;
    std::string version_;
};

class MainScript {
  public:
    MainScript(clingo_lib_t *lib) : clib_{lib} {}
    MainScript(Annotation<Library> lib) : clib_{nullptr}, lib_{std::move(lib)} { init_(); }

    static auto cast(void *data) -> MainScript * { return static_cast<MainScript *>(data); }

    static auto c_execute(char const *code, void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY {
            self->init_();
            self->py_->exec(code);
        }
        CLINGO_CATCH(get_exception_ptr());
    }

    static auto c_call([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc,
                       char const *name, clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data)
        -> clingo_result_t {
        // NOTE: the Python wrapped library is used here instead.
        auto *self = cast(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            if (self->py_) {
                auto gil = py::gil_scoped_acquire{};
                auto syms = self->py_->call(self->get_lib(), name, args);
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
        CLINGO_CATCH(get_exception_ptr());
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        // Note: that python cannot check the number of arguments
        static_cast<void>(arguments);
        auto *self = cast(data);
        CLINGO_TRY {
            auto gil = py::gil_scoped_acquire{};
            *result = self->py_ && self->py_->callable(name);
        }
        CLINGO_CATCH(get_exception_ptr());
    }

    static auto main([[maybe_unused]] clingo_lib_t *lib, clingo_control_t *control, clingo_parts_array_t const *parts,
                     size_t size, void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY {
            if (self->py_) {
                auto gil = py::gil_scoped_acquire{};
                self->py_->main(self->get_lib(), control, std::span{parts, size});
            }
        }
        CLINGO_CATCH(get_exception_ptr());
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
            if (!lib_) {
                plib_ = py::cast(Library{clib_});
                lib_ = plib_;
            }
        }
    }

    auto get_lib() -> Annotation<Library> { return py::reinterpret_borrow<Annotation<Library>>(lib_); }

    std::unique_ptr<Interpreter> py_;
    //! NOTE: only for embedding
    Annotation<Library> plib_;
    //! NOTE: only for embedding
    clingo_lib_t *clib_;
    // FIXME: this is too messy -> needs design change to just store the above libs for embedding here
    py::handle lib_;
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
        CLINGO_CATCH(get_exception_ptr());
    }

    auto call(const Annotation<Library> &lib, char const *name, SymbolVec const &args)
        -> std::variant<SymbolVec, Symbol> {
        PYBIND11_OVERRIDE_PURE(SymbolVec, Script, call, lib, name, args);
    }

    static auto c_call([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc,
                       char const *name, clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data)
        -> clingo_result_t {
        // NOTE: the Python wrapped library is used here instead.
        auto &self = get_self(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            auto gil = py::gil_scoped_acquire{};
            auto syms = self.call(self.get_lib(), name, args);
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
        CLINGO_CATCH(get_exception_ptr());
    }

    auto callable(char const *name, size_t arguments) -> bool {
        PYBIND11_OVERRIDE_PURE(bool, Script, callable, name, arguments);
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        CLINGO_TRY {
            auto gil = py::gil_scoped_acquire{};
            auto &self = get_self(data);
            *result = self.callable(name, arguments);
        }
        CLINGO_CATCH(get_exception_ptr());
    }

    void main(const Annotation<Library> &lib, Control &ctl, PartsSpan parts) {
        PYBIND11_OVERRIDE_PURE(void, Script, main, lib, &ctl, &parts);
    }

    static auto c_main([[maybe_unused]] clingo_lib_t *lib, clingo_control_t *control, clingo_parts_array_t const *parts,
                       size_t size, void *data) -> clingo_result_t {
        CLINGO_TRY {
            auto &self = get_self(data);
            auto py_ctl = Control{control};
            auto gil = py::gil_scoped_acquire{};
            self.main(self.get_lib(), py_ctl, std::span{parts, size});
        }
        CLINGO_CATCH(get_exception_ptr());
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

    void set_lib(Annotation<Library> lib) { lib_ = lib.release(); }
    auto get_lib() -> Annotation<Library> { return py::reinterpret_borrow<Annotation<Library>>(lib_); }

  private:
    // FIXME: this is too messy -> needs design change to just store the above libs for embedding here
    py::handle lib_;
    std::string name_;
    std::string version_;
};

void reg_script(Annotation<Library> lib, Annotation<Script> script) {
    auto &clib = py::cast<Library &>(lib);
    py::cast<Script &>(script).set_lib(std::move(lib));
    auto *ptr = clib.add_object(std::move(script));
    auto c_script =
        clingo_script_t{Script::c_execute, Script::c_call, Script::c_callable, Script::c_main, Script::c_name,
                        Script::c_version, nullptr};
    handle_error(clingo_script_register(clib, &c_script, ptr));
}

} // namespace

auto register_python(clingo_lib_t *lib) -> clingo_result_t {
    using Script = Clingo::Python::MainScript;
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    // NOTE: the script is going to manage an internal library
    auto script = std::make_unique<Script>(lib);
    return clingo_script_register(lib, &c_script, script.release());
}

auto reg_python(Annotation<Library> lib) -> clingo_result_t {
    using Script = Clingo::Python::MainScript;
    auto &c_lib = py::cast<Library &>(lib);
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    // NOTE: the library takes ownership of the script
    auto *script = c_lib.add_object(py::cast(Script{std::move(lib)}));
    return clingo_script_register(c_lib, &c_script, py::cast<Script *>(script));
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
        .def("enable_python", &reg_python, py::arg("lib"), R"(
Enable embedded python scripts.

Args:
    lib:
        The library to register the script with.
)"_d);
}

} // namespace Clingo::Python
