#include "script.hh"
#include "control.hh"
#include "symbol.hh"
#include "util.hh"

namespace Clingo::Python {

class Script {
  public:
    void execute(char const *code) { PYBIND11_OVERRIDE_PURE(void, Script, execute, code); }

    static auto c_execute(char const *code, void *data) -> clingo_result_t {
        auto *self = static_cast<py::object *>(data)->cast<Script *>();
        CLINGO_TRY { self->execute(code); }
        CLINGO_CATCH(self->lib_);
    }

    auto call(Library lib, char const *name, SymbolVec const &args) -> std::variant<SymbolVec, Symbol> {
        PYBIND11_OVERRIDE_PURE(SymbolVec, Script, call, &lib, name, args);
    }

    static auto c_call(clingo_lib_t *lib, clingo_location_t const *loc, char const *name,
                       clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data,
                       void *data) -> clingo_result_t {
        // Note that the location could in principle be used for better error reporting.
        static_cast<void>(loc);
        auto *self = static_cast<py::object *>(data)->cast<Script *>();
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            auto syms = self->call(lib, name, args);
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
        CLINGO_CATCH(self->lib_);
    }

    auto callable(char const *name, size_t arguments) -> bool {
        PYBIND11_OVERRIDE_PURE(bool, Script, callable, name, arguments);
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        auto *self = static_cast<py::object *>(data)->cast<Script *>();
        CLINGO_TRY {
            auto *self = static_cast<py::object *>(data)->cast<Script *>();
            *result = self->callable(name, arguments);
        }
        CLINGO_CATCH(self->lib_);
    }

    void main(Library lib, Control ctl) { PYBIND11_OVERRIDE_PURE(void, Script, main, &lib, &ctl); }

    static auto c_main(clingo_lib_t *lib, clingo_control_t *control, void *data) -> clingo_result_t {
        auto *self = static_cast<py::object *>(data)->cast<Script *>();
        CLINGO_TRY {
            auto *self = static_cast<py::object *>(data)->cast<Script *>();
            self->main(lib, control);
        }
        CLINGO_CATCH(self->lib_);
    }

    auto name() -> std::string { PYBIND11_OVERRIDE_PURE(std::string, Script, name); }

    static auto c_name(void *data) -> char const * {
        try {
            auto *self = static_cast<py::object *>(data)->cast<Script *>();
            if (self->name_.empty()) {
                self->name_ = self->name();
            }
            return self->name_.c_str();
        } catch (...) {
            return "<error>";
        }
    }

    auto version() -> std::string { PYBIND11_OVERRIDE_PURE(std::string, Script, version); }

    static auto c_version(void *data) -> char const * {
        try {
            auto *self = static_cast<py::object *>(data)->cast<Script *>();
            if (self->version_.empty()) {
                self->version_ = self->version();
            }
            return self->version_.c_str();
        } catch (...) {
            return "<error>";
        }
    }

    static void c_free(void *data) {
        // NOLINTNEXTLINE
        delete static_cast<py::object *>(data);
    }

    void lib(clingo_lib_t *lib) { lib_ = lib; }

  private:
    clingo_lib_t *lib_{};
    std::string name_;
    std::string version_;
};

void reg_script(Library const &lib, Script &script) {
    script.lib(lib);
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::c_main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    handle_error(clingo_script_register(lib, &c_script, std::make_unique<py::object>(py::cast(script)).release()));
}

void register_script(pybind11::module &m) {
    auto script = m.def_submodule("script", doc(R"(
Module containing functions to add custom scripts, which can be embedded into logic programs.
)"));
    py::class_<Script>(script, "Script", R"(ABC for custom scripts.)")
        .def(py::init<>(), R"(Construct a script object.)")
        .def("execute", &Script::execute, py::arg("code"), doc(R"(
Execute the given code.

Args:
    code: The code to execute.
)"))
        .def("call", &Script::call, py::arg("lib"), py::arg("name"), py::arg("arguments"), doc(R"(
Call the function with the given name and arguments.

Args:
    lib: The library object to store symbols.
    name: The name of the function.
    arguments: The arguments of the function.

Returns:
    A list of symbols.
)"))
        .def("callable", &Script::callable, py::arg("name"), py::arg("arguments"), doc(R"(
Check if the function with the given signature is callable.

Args:
    name: The name of the function.
    arguments: The number of arguments of the function.

Returns:
    Whether the function is callable.
)"))
        .def("main", &Script::main, py::arg("lib"), py::arg("control"), doc(R"(
Run the main function.

Args:
    lib: The (main) library object.
    control: The (main) control object.
)"))
        .def("name", &Script::name, R"(Get the name of the script.)")
        .def("version", &Script::version, R"(Get the version of the script.)");

    script.def("register", &reg_script, py::arg("lib"), py::arg("script"), doc(R"(
Registers a script language which can then be embedded into a logic program.

Args:
    lib: The library to register the script with.
    script: The script to register.
)"));
}

} // namespace Clingo::Python
