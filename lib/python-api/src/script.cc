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

class Scope {
  public:
    Scope() {
        if (Py_IsInitialized() == 0) {
            // make sure that the python interpreter is finalized after the main code
            static auto si = std::make_unique<py::scoped_interpreter>();
            py::module::import("clingo");
        }
        auto gil = py::gil_scoped_acquire{};
        scope_ = py::module_::import("__main__").attr("__dict__");
        callable_ = py::module_::import("builtins").attr("callable");
        version_ = py::module_::import("sys").attr("version").cast<std::string>();
    }

    void exec(std::string_view code) { py::exec(code, scope_); }

    auto callable(std::string_view name) -> bool {
        return scope_.contains(name) && callable_(scope_[py::str{name}]).cast<bool>();
    }

    auto call(PyLibrary const &lib, std::string_view name, SymbolVec const &args) -> std::variant<SymbolVec, Symbol> {
        return scope_[py::str{name}](lib, *py::cast(args)).cast<std::variant<SymbolVec, Symbol>>();
    }

    auto main(PyLibrary const &lib, PyControl const &ctl) { scope_["main"](lib, ctl); }

    auto version() -> std::string_view { return version_; }

  private:
    py::object scope_;
    py::object callable_;
    std::string version_;
};

class MainScript {
  public:
    //! Construct the main python script.
    //!
    //! The flag indicates whether the class has been registered on the C or
    //! python level. Scripts registered externally report errors right away
    //! and are in charge of managing library and control object.
    MainScript(bool external) : external_{external} {
        // ensure python functions can be executed
        if (!external_) {
            init_();
        }
    }

    static auto cast(void *data) -> MainScript * { return static_cast<MainScript *>(data); }

    static auto c_execute(char const *code, size_t size, void *data) -> bool {
        auto *self = cast(data);
        CLINGO_TRY {
            self->init_();
            self->py_->exec({code, size});
        }
        CLINGO_CATCH;
    }

    static auto c_call(clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc, char const *name,
                       size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data) -> bool {
        auto *self = cast(data);
        CLINGO_TRY {
            if (self->py_) {
                auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                      [](auto sym) { return Symbol{sym, true}; });
                auto gil = py::gil_scoped_acquire{};
                auto syms = self->py_->call(self->get_lib(lib), {name, name_size}, args);
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
        CLINGO_CATCH;
    }

    static auto c_callable(char const *name, size_t size, [[maybe_unused]] size_t arguments, bool *result, void *data)
        -> bool {
        // NOTE: python cannot check the number of arguments
        auto *self = cast(data);
        CLINGO_TRY {
            if (self->py_) {
                auto gil = py::gil_scoped_acquire{};
                *result = self->py_->callable({name, size});
            } else {
                *result = false;
            }
        }
        CLINGO_CATCH;
    }

    static auto main(clingo_lib_t *lib, clingo_control_t *control, void *data) -> bool {
        auto *self = cast(data);
        CLINGO_TRY {
            if (self->py_) {
                auto gil = py::gil_scoped_acquire{};
                self->py_->main(self->get_lib(lib), get_ctl(control));
            }
        }
        CLINGO_CATCH;
    }

    static void c_name([[maybe_unused]] void *data, clingo_string_t *name) {
        name->data = "python";
        name->size = strlen(name->data);
    }

    static void c_version([[maybe_unused]] void *data, clingo_string_t *version) {
        version->data = CLINGO_PYTHON_VERSION;
        version->size = strlen(version->data);
    }

    static void c_free(void *data) { std::ignore = std::make_unique<MainScript>(cast(data)); }

  private:
    void init_() {
        if (!py_) {
            py_ = std::make_unique<Scope>();
        }
    }

    auto get_lib(clingo_lib_t *lib) -> PyLibrary {
        if (external_) {
            if (lib_.ptr() == nullptr) {
                lib_ = Library::cast(lib, true);
            }
            return lib_;
        }
        return Library::cast(lib);
    }

    static auto get_ctl(clingo_control_t *ctl) -> PyControl { return Control::cast(ctl, true); }

    std::unique_ptr<Scope> py_;
    //! Stores lib pointer when embedded.
    PyLibrary lib_;
    //! Whether to store or report errors.
    bool external_;
};

class Script {
  public:
    void execute(std::string_view code) { PYBIND11_OVERRIDE_PURE(void, Script, execute, code); }

    static auto get_self(void *data) -> Script & { return *static_cast<Script *>(data); }

    static auto c_execute(char const *code, size_t size, void *data) -> bool {
        CLINGO_TRY {
            auto &self = get_self(data);
            self.execute({code, size});
        }
        CLINGO_CATCH;
    }

    auto call(const PyLibrary &lib, std::string_view name, SymbolSpan args)
        -> TypeHint<"Sequence[clingo.symbol.Symbol]"> {
        PYBIND11_OVERRIDE_PURE(TypeHint<"Sequence[clingo.symbol.Symbol]">, Script, call, lib, name, args);
    }

    static auto c_call(clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc, char const *name,
                       size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data) -> bool {
        auto &self = get_self(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            auto gil = py::gil_scoped_acquire{};
            auto syms = self.call(get_lib(lib), {name, name_size}, args).cast<std::variant<SymbolVec, Symbol>>();
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
        CLINGO_CATCH;
    }

    auto callable(std::string_view name, size_t arguments) -> bool {
        PYBIND11_OVERRIDE_PURE(bool, Script, callable, name, arguments);
    }

    static auto c_callable(char const *name, size_t size, size_t arguments, bool *result, void *data) -> bool {
        CLINGO_TRY {
            auto gil = py::gil_scoped_acquire{};
            auto &self = get_self(data);
            *result = self.callable({name, size}, arguments);
        }
        CLINGO_CATCH;
    }

    void main(const PyLibrary &lib, const PyControl &ctl) { PYBIND11_OVERRIDE_PURE(void, Script, main, lib, ctl); }

    static auto c_main(clingo_lib_t *lib, clingo_control_t *control, void *data) -> bool {
        CLINGO_TRY {
            auto gil = py::gil_scoped_acquire{};
            auto &self = get_self(data);
            self.main(get_lib(lib), get_ctl(control));
        }
        CLINGO_CATCH;
    }

    auto name() -> std::string { PYBIND11_OVERRIDE_PURE(std::string, Script, name); }

    static void c_name(void *data, clingo_string_t *name) {
        try {
            auto &self = get_self(data);
            if (self.name_.empty()) {
                self.name_ = self.name();
            }
            name->data = self.name_.data();
            name->size = self.name_.size();
        } catch (...) {
            name->data = "<error>";
            name->size = strlen(name->data);
        }
    }

    auto version() -> std::string { PYBIND11_OVERRIDE_PURE(std::string, Script, version); }

    static void c_version(void *data, clingo_string_t *version) {
        try {
            auto &self = get_self(data);
            if (self.version_.empty()) {
                self.version_ = self.version();
            }
            version->data = self.version_.data();
            version->size = self.version_.size();
        } catch (...) {
            version->data = "<error>";
            version->size = strlen(version->data);
        }
    }

    static auto get_lib(clingo_lib_t *lib) -> PyLibrary { return Library::cast(lib); }
    static auto get_ctl(clingo_control_t *ctl) -> PyControl { return Control::cast(ctl, true); }

  private:
    std::string name_;
    std::string version_;
};

void reg_script(Annotation<Library> const &lib, Annotation<Script> const &script) {
    auto &c_lib = py::cast<Library &>(lib);
    auto c_script =
        clingo_script_t{Script::c_execute, Script::c_call, Script::c_callable, Script::c_main, Script::c_name,
                        Script::c_version, nullptr};
    c_lib.tie(script);
    handle_error(clingo_script_register(c_lib, &c_script, py::cast<Script *>(script)));
}

void reg_python(Annotation<Library> const &lib) {
    using Script = Clingo::Python::MainScript;
    auto &c_lib = py::cast<Library &>(lib);
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call, Script::c_callable, Script::main, Script::c_name,
                                    Script::c_version, nullptr};
    auto py_script = py::cast(Script{false});
    c_lib.tie(py_script);
    handle_error(clingo_script_register(c_lib, &c_script, py::cast<Script *>(py_script)));
}

} // namespace

auto register_python(clingo_lib_t *lib) -> bool {
    using Script = Clingo::Python::MainScript;
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    auto script = std::make_unique<Script>(true);
    return clingo_script_register(lib, &c_script, script.release());
}

void register_script(pybind11::module &m) {
    auto script = m.def_submodule("script", R"(
Module to add custom scripts that can be embedded into logic programs.

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
...     def execute(self, code) -> None:
...         exec(code, __main__.__dict__, __main__.__dict__)
...     def call(self, lib, name, arguments):
...         return [getattr(__main__, name)(lib, *arguments)]
...     def callable(self, name, arguments) -> bool:
...         return name in __main__.__dict__ and callable(__main__.__dict__[name])
...     def main(self, lib, control) -> None:
...         __main__.main(lib, control)
...     def name(self):
...         return "python"
...
>>> def f(lib, x):
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
... def g(lib, x):
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
)"_d);
    std::ignore = py::class_<MainScript>(script, "_MainScript");

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
Check if a function with the given signature is callable.

Args:
    name:
        The name of the function.
    arguments:
        The number of arguments of the function.

Returns:
    Whether the function is callable.
)")
        .def("main", &Script::main, py::arg("lib"), py::arg("control"), R"(
Run the main function.

Args:
    lib:
        The (main) library object.
    control:
        The (main) control object.
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
