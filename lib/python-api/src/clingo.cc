#include "clingo.hh"
#include "app.hh"
#include "ast.hh"
#include "backend.hh"
#include "base.hh"
#include "config.hh"
#include "control.hh"
#include "core.hh"
#include "ground.hh"
#include "propagate.hh"
#include "script.hh"
#include "solve.hh"
#include "stats.hh"
#include "symbol.hh"
#include "theory.hh"
#include "util.hh"

#include <array>

namespace PyClingo {

namespace {

//! The registration functions of a submodule.
//!
//! Modules are registered in two phases. First, all modules are created with
//! their own types. Then, the types imported from other modules are stored in
//! the module states. This way, modules can import each other's types
//! independently of the order in which they are created.
struct Submodule {
    //! Create the module (returns a new reference).
    PyObject *(*create)(PyObject *parent);
    //! Import types of other modules (nullptr if the module imports no types).
    bool (*link)(PyObject *mod);
};

constexpr auto submodules = std::array{
    Submodule{create_core, nullptr},         Submodule{create_symbol, link_symbol},
    Submodule{create_ast, link_ast},         Submodule{create_base, link_base},
    Submodule{create_backend, link_backend}, Submodule{create_ground, link_ground},
    Submodule{create_stats, link_stats},     Submodule{create_solve, link_solve},
    Submodule{create_config, nullptr},       Submodule{create_propagate, link_propagate},
    Submodule{create_control, link_control}, Submodule{create_script, link_script},
    Submodule{create_app, link_app},         Submodule{create_theory, link_theory},
};

//! Remove the clingo submodules from sys.modules keeping the current error.
//!
//! This makes sure that a failed import does not leave behind partially
//! initialized modules.
void remove_submodules() {
    PyObject *type = nullptr;
    PyObject *value = nullptr;
    PyObject *trace = nullptr;
    PyErr_Fetch(&type, &value, &trace);
    auto *modules = PyImport_GetModuleDict();
    auto prefix = steal(PyUnicode_FromString("clingo."));
    auto keys = steal(modules != nullptr ? PyDict_Keys(modules) : nullptr);
    if (prefix.get() != nullptr && keys.get() != nullptr) {
        auto size = PyList_Size(keys.get());
        for (Py_ssize_t i = 0; i < size; ++i) {
            auto *key = PyList_GetItem(keys.get(), i);
            if (PyUnicode_Check(key) != 0 && PyUnicode_Tailmatch(key, prefix.get(), 0, PY_SSIZE_T_MAX, -1) == 1 &&
                PyDict_DelItem(modules, key) < 0) {
                break;
            }
        }
    }
    PyErr_Clear();
    PyErr_Restore(type, value, trace);
}

//! Create and link the submodules of the clingo module.
//!
//! The module uses multi-phase initialization so that it is already in
//! sys.modules when this function runs. Otherwise, importing a submodule while
//! linking would import the clingo module again.
auto clingo_exec(PyObject *clingo) -> int {
    std::array<PyGuard, submodules.size()> mods;
    for (size_t i = 0; i < submodules.size(); ++i) {
        mods.at(i) = steal(submodules.at(i).create(clingo));
        if (mods.at(i).get() == nullptr) {
            remove_submodules();
            return -1;
        }
    }
    for (size_t i = 0; i < submodules.size(); ++i) {
        if (auto *link = submodules.at(i).link; link != nullptr && !link(mods.at(i).get())) {
            remove_submodules();
            return -1;
        }
    }
    // TODO: The modules could be loaded lazily by installing a finder in
    // sys.meta_path. Its loader would create a module in create_module() and
    // link it in exec_module().
    return 0;
}

auto clingo_slots = std::array{
    PyModuleDef_Slot{Py_mod_exec, reinterpret_cast<void *>(clingo_exec)},
    PyModuleDef_Slot{0, nullptr},
};

// NOTE: the documentation of the package is maintained in the stubs
PyModuleDef clingo_def = {
    PyModuleDef_HEAD_INIT,
    "clingo",
    PyDoc_STR("Functions and classes to control the grounding and solving process of Answer Set Programming."),
    0,
    nullptr,
    clingo_slots.data(),
    nullptr,
    nullptr,
    nullptr,
};

} // namespace

auto init_clingo() -> PyObject * {
    return PyModuleDef_Init(&clingo_def);
}

} // namespace PyClingo
