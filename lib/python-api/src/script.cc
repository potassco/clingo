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

    auto call(Library &lib, char const *name, SymbolVec const &args) -> SymbolVec {
        PYBIND11_OVERRIDE_PURE(SymbolVec, Script, call, lib, name, args);
    }

    static auto c_call(clingo_lib_t *lib, char const *name, clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data,
                       void *data) -> clingo_result_t {
        auto *self = static_cast<py::object *>(data)->cast<Script *>();
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            Library l(lib);
            auto syms = self->call(l, name, args);
            // NOLINTNEXTLINE
            auto const *c_syms = reinterpret_cast<clingo_symbol_t *>(syms.data());
            handle_error(symbol_callback(c_syms, syms.size(), symbol_callback_data));
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

    void main(Library lib, Control ctl) { PYBIND11_OVERRIDE_PURE(void, Script, main, lib, ctl); }

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

void reg_script(Library &lib, Script &script) {
    script.lib(lib);
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::c_main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    handle_error(clingo_script_register(lib, &c_script, std::make_unique<py::object>(py::cast(script)).release()));
}

void register_script(pybind11::module &m) {
    auto script = m.def_submodule("script", doc(R"(
Module containing functions to add custom scripts, which can be embedded into logic programs.
)"));
    py::class_<Script>(script, "Script")
        .def(py::init<>())
        .def("execute", &Script::execute)
        .def("call", &Script::call)
        .def("callable", &Script::callable)
        //.def("main", &Script::main)
        .def("name", &Script::name)
        .def("version", &Script::version);

    script.def("register", &reg_script, "Registers a script language which can then be embedded into a logic program.");
}

} // namespace Clingo::Python
