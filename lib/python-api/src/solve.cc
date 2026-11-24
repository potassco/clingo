#define PY_SSIZE_T_CLEAN

#include "solve.hh"

#include "base.hh"
#include "core.hh"
#include "stats.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/solve.h>

#include <array>
#include <cstdint>
#include <utility>

namespace PyClingo {

namespace {

struct SolveModuleState {
    PyObject *symbol_type{nullptr};
    PyObject *base_type{nullptr};
    PyObject *stats_type{nullptr};
    PyObject *model_type_enum{nullptr};
    PyObject *control_type{nullptr};
    PyObject *model_type{nullptr};
    PyObject *result_type{nullptr};
    PyObject *handle_type{nullptr};
    PyObject *async_handle_type{nullptr};
    PyObject *async_step_type{nullptr};
};

struct PyResultObject {
    PyObject_HEAD
    clingo_solve_result_bitset_t res;
};

struct PyModelObject {
    PyObject_HEAD
    clingo_model_t *mdl;
    PyObject *control;
    bool is_mutable;
};

struct PyControlObject {
    PyObject_HEAD
    clingo_solve_control_t *ctl;
};

struct PyHandleObject {
    PyObject_HEAD
    clingo_solve_handle_t *hnd;
    //! The statistics passed to on_statistics (null outside of the callback).
    clingo_stats_t *stats;
    PyObject *owner;
    PyObject *on_model;
    PyObject *on_unsat;
    PyObject *on_stats;
    PyObject *on_finish;
    PyObject *cb_model;
    PyObject *cur_model;
};

// casts and module state

auto result_cast(PyObject *obj) -> PyResultObject * {
    return reinterpret_cast<PyResultObject *>(obj);
}

auto model_cast(PyObject *obj) -> PyModelObject * {
    return reinterpret_cast<PyModelObject *>(obj);
}

auto control_cast(PyObject *obj) -> PyControlObject * {
    return reinterpret_cast<PyControlObject *>(obj);
}

auto handle_cast(PyObject *obj) -> PyHandleObject * {
    return reinterpret_cast<PyHandleObject *>(obj);
}

auto get_state(PyObject *module) -> SolveModuleState * {
    auto *state = static_cast<SolveModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "solve module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_type(PyObject *type) -> SolveModuleState * {
    auto *state = static_cast<SolveModuleState *>(PyType_GetModuleState(reinterpret_cast<PyTypeObject *>(type)));
    if (state == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "solve module state is unavailable");
        }
        return nullptr;
    }
    return state;
}

auto state_of(PyObject *self) -> SolveModuleState * {
    return get_state_from_type(reinterpret_cast<PyObject *>(Py_TYPE(self)));
}

template <class T> auto alloc_object(PyObject *cls) -> T * {
    return reinterpret_cast<T *>(PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(cls), 0));
}

template <class T> auto int_list(T const *values, size_t size) -> PyObject * {
    auto list = steal(PyList_New(static_cast<Py_ssize_t>(size)));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < size; ++i) {
        auto *value = PyLong_FromLongLong(static_cast<long long>(values[i]));
        if (value == nullptr) {
            return nullptr;
        }
        if (PyList_SetItem(list.get(), static_cast<Py_ssize_t>(i), value) < 0) {
            return nullptr;
        }
    }
    return list.release();
}

auto check_callback(PyObject *&cb, char const *what) -> bool {
    if (cb == Py_None) {
        cb = nullptr;
    }
    if (cb != nullptr && PyCallable_Check(cb) == 0) {
        PyErr_Format(PyExc_TypeError, "%s must be callable", what);
        return false;
    }
    return true;
}

// SolveResult

auto result_new(SolveModuleState *state, clingo_solve_result_bitset_t res) -> PyObject * {
    auto *obj = alloc_object<PyResultObject>(state->result_type);
    if (obj == nullptr) {
        return nullptr;
    }
    obj->res = res;
    return reinterpret_cast<PyObject *>(obj);
}

auto result_flag(PyObject *self, clingo_solve_result_bitset_t mask) -> PyObject * {
    return PyBool_FromLong((result_cast(self)->res & mask) != 0 ? 1 : 0);
}

auto py_result_satisfiable(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return result_flag(self, clingo_solve_result_satisfiable);
}

auto py_result_unsatisfiable(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return result_flag(self, clingo_solve_result_unsatisfiable);
}

auto py_result_unknown(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto mask = clingo_solve_result_bitset_t{clingo_solve_result_satisfiable | clingo_solve_result_unsatisfiable};
    return PyBool_FromLong((result_cast(self)->res & mask) == 0 ? 1 : 0);
}

auto py_result_exhausted(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return result_flag(self, clingo_solve_result_exhausted);
}

auto py_result_interrupted(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return result_flag(self, clingo_solve_result_interrupted);
}

auto py_result_str(PyObject *self) -> PyObject * {
    auto res = result_cast(self)->res;
    if ((res & clingo_solve_result_satisfiable) != 0) {
        return PyUnicode_FromString("SAT");
    }
    if ((res & clingo_solve_result_unsatisfiable) != 0) {
        return PyUnicode_FromString("UNSAT");
    }
    return PyUnicode_FromString("UNKNOWN");
}

auto py_result_getset = std::array{
    PyGetSetDef{
        "satisfiable",
        py_result_satisfiable,
        nullptr,
        PyDoc_STR("Whether at least one model was found."),
        nullptr,
    },
    PyGetSetDef{
        "unsatisfiable",
        py_result_unsatisfiable,
        nullptr,
        PyDoc_STR("Whether there was no model."),
        nullptr,
    },
    PyGetSetDef{
        "unknown",
        py_result_unknown,
        nullptr,
        PyDoc_STR("Whether the satisfiability is unknown."),
        nullptr,
    },
    PyGetSetDef{
        "exhausted",
        py_result_exhausted,
        nullptr,
        PyDoc_STR("Whether all models have been enumerated."),
        nullptr,
    },
    PyGetSetDef{
        "interrupted",
        py_result_interrupted,
        nullptr,
        PyDoc_STR("Whether the search was interrupted."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_result_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A solve result captures information about a solve call.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_result_str)},
    PyType_Slot{Py_tp_getset, py_result_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_result_spec = PyType_Spec{
    "clingo.solve.SolveResult", sizeof(PyResultObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_result_slots.data(),
};

auto control_get(PyObject *self) -> clingo_solve_control_t * {
    auto *ctl = control_cast(self)->ctl;
    if (ctl == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "control is no longer valid");
    }
    return ctl;
}

// Model

auto model_new(PyObject *cls, clingo_model_t const *mdl, bool is_mutable) -> PyObject * {
    auto *obj = alloc_object<PyModelObject>(cls);
    if (obj == nullptr) {
        return nullptr;
    }
    obj->mdl = const_cast<clingo_model_t *>(mdl);
    obj->is_mutable = is_mutable;
    return reinterpret_cast<PyObject *>(obj);
}

auto model_get(PyObject *self, bool mut) -> clingo_model_t * {
    auto *m = model_cast(self);
    auto *mdl = m->mdl;
    if (mdl == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "model is no longer valid");
        return nullptr;
    }
    if (mut && !m->is_mutable) {
        PyErr_SetString(PyExc_RuntimeError, "model is not mutable");
        return nullptr;
    }
    return mdl;
}

void model_clear(PyObject *obj) {
    if (obj != nullptr) {
        auto *m = model_cast(obj);
        m->mdl = nullptr;
        if (m->control != nullptr) {
            control_cast(m->control)->ctl = nullptr;
            Py_CLEAR(m->control);
        }
    }
}

auto py_model_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *m = model_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(m->control);
    return 0;
}

auto py_model_tp_clear(PyObject *self) -> int {
    model_clear(self);
    return 0;
}

auto model_set(PyObject *&slot, PyObject *cls, clingo_model_t const *mdl, bool is_mutable) -> PyObject * {
    if (slot == nullptr) {
        slot = model_new(cls, mdl, is_mutable);
        if (slot == nullptr) {
            return nullptr;
        }
    } else {
        auto *m = model_cast(slot);
        m->mdl = const_cast<clingo_model_t *>(mdl);
        m->is_mutable = is_mutable;
    }
    return slot;
}

struct SymbolsData {
    PyObject *symbol_type;
    PyObject *list;
};

auto symbols_cb(clingo_symbol_t const *symbols, size_t size, void *data) -> bool {
    auto *d = static_cast<SymbolsData *>(data);
    for (size_t i = 0; i < size; ++i) {
        auto sym = steal(py_symbol_new(d->symbol_type, symbols[i], true));
        if (sym.get() == nullptr) {
            return store_python_error("creating symbol failed");
        }
        if (PyList_Append(d->list, sym.get()) < 0) {
            return store_python_error("appending symbol to list failed");
        }
    }
    return true;
}

auto py_model_symbols(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *shown_obj{};
    PyObject *atoms_obj{};
    PyObject *terms_obj{};
    PyObject *theory_obj{};
    if (!parse_args("symbols", {}, {"shown", "atoms", "terms", "theory"}, args, nargs, kwnames,
                    {&shown_obj, &atoms_obj, &terms_obj, &theory_obj})) {
        return nullptr;
    }
    auto shown = false;
    auto atoms = false;
    auto terms = false;
    auto theory = false;
    if (!parse_bool(shown_obj, &shown) || !parse_bool(atoms_obj, &atoms) || !parse_bool(terms_obj, &terms) ||
        !parse_bool(theory_obj, &theory)) {
        return nullptr;
    }
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_show_type_bitset_t show = 0;
    if (shown) {
        show |= clingo_show_type_shown;
    }
    if (atoms) {
        show |= clingo_show_type_atoms;
    }
    if (terms) {
        show |= clingo_show_type_terms;
    }
    if (theory) {
        show |= clingo_show_type_theory;
    }
    auto list = steal(PyList_New(0));
    if (list.get() == nullptr) {
        return nullptr;
    }
    auto data = SymbolsData{state->symbol_type, list.get()};
    if (!clingo_model_symbols(mdl, show, symbols_cb, &data)) {
        return py_handle_error();
    }
    return list.release();
}

auto py_model_contains(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *atom = unset_arg();
    if (!parse_args("contains", {"atom"}, {}, args, nargs, kwnames, {&atom})) {
        return nullptr;
    }
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_symbol_t sym = 0;
    auto check = py_symbol_check(state->symbol_type, atom, &sym);
    if (check < 0) {
        return nullptr;
    }
    if (check == 0) {
        PyErr_SetString(PyExc_TypeError, "atom must be a Symbol");
        return nullptr;
    }
    bool res = false;
    if (!clingo_model_contains(mdl, sym, &res)) {
        return py_handle_error();
    }
    return PyBool_FromLong(res ? 1 : 0);
}

auto py_model_is_true(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *lit_obj = unset_arg();
    if (!parse_args("is_true", {"literal"}, {}, args, nargs, kwnames, {&lit_obj})) {
        return nullptr;
    }
    clingo_literal_t lit = 0;
    if (!parse_num(lit_obj, &lit, "literal")) {
        return nullptr;
    }
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    bool res = false;
    if (!clingo_model_is_true(mdl, lit, &res)) {
        return py_handle_error();
    }
    return PyBool_FromLong(res ? 1 : 0);
}

auto py_model_is_consequence(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *lit_obj = unset_arg();
    if (!parse_args("is_consequence", {"literal"}, {}, args, nargs, kwnames, {&lit_obj})) {
        return nullptr;
    }
    clingo_literal_t lit = 0;
    if (!parse_num(lit_obj, &lit, "literal")) {
        return nullptr;
    }
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    clingo_consequence_t res = 0;
    if (!clingo_model_is_consequence(mdl, lit, &res)) {
        return py_handle_error();
    }
    if (res == clingo_consequence_unknown) {
        Py_RETURN_NONE;
    }
    return PyBool_FromLong(res == clingo_consequence_true ? 1 : 0);
}

auto parse_symbols(SolveModuleState *state, PyObject *obj, PyGuard &keep, PyBuffer<clingo_symbol_t> &out) -> bool {
    if (PyUnicode_Check(obj) != 0 || PyBytes_Check(obj) != 0) {
        PyErr_SetString(PyExc_TypeError, "symbols must be a sequence of Symbols");
        return false;
    }
    keep = steal(PySequence_Tuple(obj));
    if (keep.get() == nullptr) {
        return false;
    }
    auto n = PyTuple_Size(keep.get());
    if (n < 0) {
        return false;
    }
    if (!out.alloc(static_cast<size_t>(n))) {
        return false;
    }
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *item = PyTuple_GetItem(keep.get(), i);
        if (item == nullptr) {
            return false;
        }
        auto check = py_symbol_check(state->symbol_type, item, &out[static_cast<size_t>(i)]);
        if (check < 0) {
            return false;
        }
        if (check == 0) {
            PyErr_SetString(PyExc_TypeError, "symbols must be a sequence of Symbols");
            return false;
        }
    }
    return true;
}

auto py_model_extend(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *symbols = unset_arg();
    if (!parse_args("extend", {"symbols"}, {}, args, nargs, kwnames, {&symbols})) {
        return nullptr;
    }
    auto *mdl = model_get(self, true);
    if (mdl == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto keep = PyGuard{nullptr};
    PyBuffer<clingo_symbol_t> syms;
    if (!parse_symbols(state, symbols, keep, syms)) {
        return nullptr;
    }
    if (!clingo_model_extend(mdl, syms.data(), syms.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

struct StrData {
    clingo_string_builder_t *bld;
    PyObject *list;
};

auto str_cb(clingo_symbol_t const *symbols, size_t size, void *data) -> bool {
    auto *d = static_cast<StrData *>(data);
    for (size_t i = 0; i < size; ++i) {
        clingo_string_builder_clear(d->bld);
        if (!clingo_symbol_to_string(symbols[i], d->bld)) {
            return false;
        }
        clingo_string_t str;
        if (!clingo_string_builder_string(d->bld, &str)) {
            return false;
        }
        auto py_str = steal(PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size)));
        if (py_str.get() == nullptr) {
            return store_python_error("creating string failed");
        }
        if (PyList_Append(d->list, py_str.get()) < 0) {
            return store_python_error("appending string to list failed");
        }
    }
    return true;
}

auto py_model_str(PyObject *self) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    auto list = steal(PyList_New(0));
    if (list.get() == nullptr) {
        return nullptr;
    }
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    auto data = StrData{bld.get(), list.get()};
    if (!clingo_model_symbols(mdl, clingo_show_type_shown, str_cb, &data)) {
        return py_handle_error();
    }
    auto sep = steal(PyUnicode_FromString(", "));
    if (sep.get() == nullptr) {
        return nullptr;
    }
    return PyUnicode_Join(sep.get(), list.get());
}

auto py_model_control(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *mdl = model_get(self, true);
    if (mdl == nullptr) {
        return nullptr;
    }
    auto *m = model_cast(self);
    if (m->control == nullptr) {
        auto *state = state_of(self);
        if (state == nullptr) {
            return nullptr;
        }
        clingo_solve_control_t *ctl = nullptr;
        if (!clingo_model_control(mdl, &ctl)) {
            return py_handle_error();
        }
        auto *obj = alloc_object<PyControlObject>(state->control_type);
        if (obj == nullptr) {
            return nullptr;
        }
        obj->ctl = ctl;
        m->control = reinterpret_cast<PyObject *>(obj);
    }
    return Py_NewRef(m->control);
}

auto py_get_model_type(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_model_type_t type = 0;
    if (!clingo_model_type(mdl, &type)) {
        return py_handle_error();
    }
    return PyObject_CallFunction(state->model_type_enum, "i", static_cast<int>(type));
}

auto py_model_number(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    uint64_t num = 0;
    if (!clingo_model_number(mdl, &num)) {
        return py_handle_error();
    }
    return PyLong_FromUnsignedLongLong(num);
}

auto py_model_cost(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    int64_t const *costs = nullptr;
    size_t size = 0;
    if (!clingo_model_cost(mdl, &costs, &size)) {
        return py_handle_error();
    }
    return int_list(costs, size);
}

auto py_model_priorities(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    clingo_weight_t const *prios = nullptr;
    size_t size = 0;
    if (!clingo_model_priority(mdl, &prios, &size)) {
        return py_handle_error();
    }
    return int_list(prios, size);
}

auto py_model_optimality_proven(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    bool res = false;
    if (!clingo_model_optimality_proven(mdl, &res)) {
        return py_handle_error();
    }
    return PyBool_FromLong(res ? 1 : 0);
}

auto py_model_thread_id(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    clingo_id_t id = 0;
    if (!clingo_model_thread_id(mdl, &id)) {
        return py_handle_error();
    }
    return PyLong_FromUnsignedLong(id);
}

auto py_model_methods = std::array{
    PyMethodDef{
        "symbols",
        method_cast(py_model_symbols),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the symbols in the model."),
    },
    PyMethodDef{
        "contains",
        method_cast(py_model_contains),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if the model contains the given atom."),
    },
    PyMethodDef{
        "is_true",
        method_cast(py_model_is_true),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if the given program literal is true."),
    },
    PyMethodDef{
        "is_consequence",
        method_cast(py_model_is_consequence),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if the given program literal is a consequence."),
    },
    PyMethodDef{
        "extend",
        method_cast(py_model_extend),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Extend a model with the given symbols."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_model_getset = std::array{
    PyGetSetDef{
        "control",
        py_model_control,
        nullptr,
        PyDoc_STR("Get the associated solve control object."),
        nullptr,
    },
    PyGetSetDef{
        "type",
        py_get_model_type,
        nullptr,
        PyDoc_STR("Get the type of a model."),
        nullptr,
    },
    PyGetSetDef{
        "number",
        py_model_number,
        nullptr,
        PyDoc_STR("Get the running number of a model."),
        nullptr,
    },
    PyGetSetDef{
        "cost",
        py_model_cost,
        nullptr,
        PyDoc_STR("Get the integer cost values of the model."),
        nullptr,
    },
    PyGetSetDef{
        "priorities",
        py_model_priorities,
        nullptr,
        PyDoc_STR("Get the priorities of the cost values."),
        nullptr,
    },
    PyGetSetDef{
        "optimality_proven",
        py_model_optimality_proven,
        nullptr,
        PyDoc_STR("Whether the optimality of the model has been proven."),
        nullptr,
    },
    PyGetSetDef{
        "thread_id",
        py_model_thread_id,
        nullptr,
        PyDoc_STR("Get the thread id the model was found in."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_model_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A view on the solver's current solution.")}, // NOLINT
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_model_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_model_tp_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_model_str)},
    PyType_Slot{Py_tp_methods, py_model_methods.data()},
    PyType_Slot{Py_tp_getset, py_model_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_model_spec = PyType_Spec{
    "clingo.solve.Model",
    sizeof(PyModelObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_model_slots.data(),
};

// SolveControl

auto py_control_base(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_base_t const *base = nullptr;
    if (!clingo_solve_control_base(ctl, &base)) {
        return py_handle_error();
    }
    return py_base_new(self, pointer_slot(&control_cast(self)->ctl), state->base_type, base);
}

auto control_add(PyObject *self, PyObject *lits, bool negate) -> PyObject * {
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto base = steal(py_control_base(self, nullptr));
    if (base.get() == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    PyBuffer<clingo_literal_t> buf;
    if (!convert(state->base_type, base.get(), lits, negate, true, buf)) {
        return nullptr;
    }
    if (!clingo_solve_control_add_clause(ctl, buf.data(), buf.size())) {
        py_handle_error();
        return nullptr;
    }
    Py_RETURN_NONE;
}

auto py_control_add_clause(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *lits = unset_arg();
    if (!parse_args("add_clause", {"clause"}, {}, args, nargs, kwnames, {&lits})) {
        return nullptr;
    }
    return control_add(self, lits, false);
}

auto py_control_add_nogood(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *lits = unset_arg();
    if (!parse_args("add_nogood", {"nogood"}, {}, args, nargs, kwnames, {&lits})) {
        return nullptr;
    }
    return control_add(self, lits, true);
}

auto py_control_methods = std::array{
    PyMethodDef{
        "add_clause",
        method_cast(py_control_add_clause),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a clause that applies to the current solving step."),
    },
    PyMethodDef{
        "add_nogood",
        method_cast(py_control_add_nogood),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a nogood that applies to the current solving step."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_control_getset = std::array{
    PyGetSetDef{"base", py_control_base, nullptr, PyDoc_STR("Get the atom/term bases of the program."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_control_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A control object to add clauses while solving.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_methods, py_control_methods.data()},
    PyType_Slot{Py_tp_getset, py_control_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_control_spec = PyType_Spec{
    "clingo.solve.SolveControl", sizeof(PyControlObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_control_slots.data(),
};

auto on_model_cb([[maybe_unused]] clingo_model_t *model, void *data, bool *goon) -> bool {
    auto gil = GILGuard{};
    auto *self = static_cast<PyObject *>(data);
    auto *h = handle_cast(self);
    auto *state = state_of(self);
    if (state == nullptr) {
        return store_python_error("solve module state is unavailable");
    }
    auto *mdl = model_set(h->cb_model, state->model_type, model, true);
    if (mdl == nullptr) {
        return store_python_error("creating model failed");
    }
    auto res = steal(PyObject_CallFunctionObjArgs(h->on_model, mdl, nullptr));
    // NOTE: the model is only valid during the callback
    model_clear(h->cb_model);
    if (res.get() == nullptr) {
        return store_python_error("on_model failed");
    }
    if (res.get() == Py_None) {
        *goon = true;
        return true;
    }
    if (PyBool_Check(res.get()) == 0) {
        PyErr_SetString(PyExc_TypeError, "on_model must return a bool or None");
        return store_python_error("on_model failed");
    }
    *goon = res.get() == Py_True;
    return true;
}

auto on_unsat_cb(int64_t const *values, size_t size, void *data) -> bool {
    auto gil = GILGuard{};
    auto *h = handle_cast(static_cast<PyObject *>(data));
    auto bound = steal(int_list(values, size));
    if (bound.get() == nullptr) {
        return store_python_error("creating lower bound failed");
    }
    auto res = steal(PyObject_CallFunctionObjArgs(h->on_unsat, bound.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("on_unsat failed");
    }
    return true;
}

auto on_stats_cb(clingo_stats_t *stats, void *data) -> bool {
    auto gil = GILGuard{};
    auto *h = handle_cast(static_cast<PyObject *>(data));
    auto *state = state_of(static_cast<PyObject *>(data));
    if (state == nullptr) {
        return store_python_error("solve module state is unavailable");
    }
    uint64_t root = 0;
    if (!clingo_stats_root(stats, &root)) {
        return false;
    }
    uint64_t step = 0;
    if (!clingo_stats_map_add_subkey(stats, root, clingo_user_stats_step.data, clingo_user_stats_step.size,
                                     clingo_stats_type_map, &step)) {
        return false;
    }
    uint64_t accu = 0;
    if (!clingo_stats_map_add_subkey(stats, root, clingo_user_stats_accu.data, clingo_user_stats_accu.size,
                                     clingo_stats_type_map, &accu)) {
        return false;
    }
    assert(h->stats == nullptr);
    h->stats = stats;
    auto ok = [&] {
        auto *self = static_cast<PyObject *>(data);
        auto step_stats = steal(py_stats_new(self, pointer_slot(&h->stats), state->stats_type, stats, step));
        if (step_stats.get() == nullptr) {
            return store_python_error("creating step statistics failed");
        }
        auto accu_stats = steal(py_stats_new(self, pointer_slot(&h->stats), state->stats_type, stats, accu));
        if (accu_stats.get() == nullptr) {
            return store_python_error("creating accumulated statistics failed");
        }
        auto res = steal(PyObject_CallFunctionObjArgs(h->on_stats, step_stats.get(), accu_stats.get(), nullptr));
        if (res.get() == nullptr) {
            return store_python_error("on_stats failed");
        }
        return true;
    }();
    h->stats = nullptr;
    return ok;
}

void on_finish_cb(clingo_solve_result_bitset_t result, void *data) {
    auto gil = GILGuard{};
    auto *self = static_cast<PyObject *>(data);
    auto *h = handle_cast(self);
    auto *state = state_of(self);
    if (state != nullptr) {
        auto res = steal(result_new(state, result));
        if (res.get() != nullptr) {
            auto ret = steal(PyObject_CallFunctionObjArgs(h->on_finish, res.get(), nullptr));
            if (ret.get() != nullptr) {
                return;
            }
        }
    }
    // NOTE: the callback is meant for synchronization and errors cannot be propagated
    PyErr_WriteUnraisable(self);
}

// SolveHandle: object

void close_handle(PyHandleObject *h) {
    model_clear(h->cur_model);
    auto *hnd = std::exchange(h->hnd, nullptr);
    if (hnd != nullptr) {
        with_gil_released([&] {
            clingo_solve_handle_close(hnd);
            return true;
        });
    }
}

auto ensure_handle_open(PyObject *self) -> clingo_solve_handle_t * {
    auto *h = handle_cast(self);
    if (h->hnd == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "solve handle is closed");
        return nullptr;
    }
    return h->hnd;
}

auto handle_new(SolveModuleState *state, PyObject *owner, PyObject *on_model, PyObject *on_unsat, PyObject *on_stats,
                PyObject *on_finish) -> PyObject * {
    auto *h = alloc_object<PyHandleObject>(state->handle_type);
    if (h == nullptr) {
        return nullptr;
    }
    h->owner = Py_XNewRef(owner);
    h->on_model = Py_XNewRef(on_model);
    h->on_unsat = Py_XNewRef(on_unsat);
    h->on_stats = Py_XNewRef(on_stats);
    h->on_finish = Py_XNewRef(on_finish);
    return reinterpret_cast<PyObject *>(h);
}

auto py_handle_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *h = handle_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(h->owner);
    Py_VISIT(h->on_model);
    Py_VISIT(h->on_unsat);
    Py_VISIT(h->on_stats);
    Py_VISIT(h->on_finish);
    Py_VISIT(h->cb_model);
    Py_VISIT(h->cur_model);
    return 0;
}

auto py_handle_clear(PyObject *self) -> int {
    auto *h = handle_cast(self);
    // NOTE: close first because running callbacks still use the references
    close_handle(h);
    Py_CLEAR(h->owner);
    Py_CLEAR(h->on_model);
    Py_CLEAR(h->on_unsat);
    Py_CLEAR(h->on_stats);
    Py_CLEAR(h->on_finish);
    Py_CLEAR(h->cb_model);
    Py_CLEAR(h->cur_model);
    return 0;
}

auto fetch_model(clingo_solve_handle_t *hnd, clingo_model_t **mdl) -> bool {
    if (!with_gil_released([&] { return clingo_solve_handle_model(hnd, mdl); })) {
        py_handle_error();
        return false;
    }
    return true;
}

auto py_handle_get(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    clingo_solve_result_bitset_t res = 0;
    if (!with_gil_released([&] { return clingo_solve_handle_get(hnd, &res); })) {
        return py_handle_error();
    }
    return result_new(state, res);
}

auto py_handle_cancel(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    if (!with_gil_released([&] { return clingo_solve_handle_cancel(hnd); })) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_handle_interrupt(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    if (!clingo_solve_handle_interrupt(hnd)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_handle_resume(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    if (!with_gil_released([&] { return clingo_solve_handle_resume(hnd); })) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_handle_model(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_model_t *mdl = nullptr;
    if (!fetch_model(hnd, &mdl)) {
        return nullptr;
    }
    if (mdl == nullptr) {
        Py_RETURN_NONE;
    }
    auto *obj = model_set(handle_cast(self)->cur_model, state->model_type, mdl, true);
    return Py_XNewRef(obj);
}

auto py_handle_last(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    clingo_model_t const *mdl = nullptr;
    if (!with_gil_released([&] { return clingo_solve_handle_last(hnd, &mdl); })) {
        return py_handle_error();
    }
    if (mdl == nullptr) {
        Py_RETURN_NONE;
    }
    auto *obj = model_set(handle_cast(self)->cur_model, state->model_type, mdl, false);
    return Py_XNewRef(obj);
}

auto py_handle_core(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    clingo_literal_t const *lits = nullptr;
    size_t size = 0;
    if (!with_gil_released([&] { return clingo_solve_handle_core(hnd, &lits, &size); })) {
        return py_handle_error();
    }
    return int_list(lits, size);
}

auto py_handle_wait(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *timeout_obj = Py_None;
    if (!parse_args("wait", {}, {"timeout"}, args, nargs, kwnames, {&timeout_obj})) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    double timeout = -1;
    if (timeout_obj != Py_None) {
        timeout = PyFloat_AsDouble(timeout_obj);
        if (timeout == -1.0 && PyErr_Occurred() != nullptr) {
            return nullptr;
        }
    }
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    bool result = false;
    if (!with_gil_released([&] { return clingo_solve_handle_wait(hnd, timeout, &result); })) {
        return py_handle_error();
    }
    return PyBool_FromLong(result ? 1 : 0);
}

auto py_handle_close(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    close_handle(handle_cast(self));
    Py_RETURN_NONE;
}

auto py_handle_enter(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    if (ensure_handle_open(self) == nullptr) {
        return nullptr;
    }
    return Py_NewRef(self);
}

auto py_handle_exit(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> PyObject * {
    PyObject *type = unset_arg();
    PyObject *value = unset_arg();
    PyObject *traceback = unset_arg();
    if (!parse_args("__exit__", {"type", "value", "traceback"}, {}, args, nargs, {&type, &value, &traceback})) {
        return nullptr;
    }
    close_handle(handle_cast(self));
    Py_RETURN_NONE;
}

auto py_handle_iter(PyObject *self) -> PyObject * {
    if (ensure_handle_open(self) == nullptr) {
        return nullptr;
    }
    return Py_NewRef(self);
}

auto py_handle_iternext(PyObject *self) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    if (!with_gil_released([&] { return clingo_solve_handle_resume(hnd); })) {
        return py_handle_error();
    }
    clingo_model_t *mdl = nullptr;
    if (!fetch_model(hnd, &mdl)) {
        return nullptr;
    }
    if (mdl == nullptr) {
        // NOTE: returning nullptr without an error raises StopIteration
        return nullptr;
    }
    auto *obj = model_set(handle_cast(self)->cur_model, state->model_type, mdl, true);
    return Py_XNewRef(obj);
}

auto py_handle_methods = std::array{
    PyMethodDef{
        "get",
        py_handle_get,
        METH_NOARGS,
        PyDoc_STR("Get the solve result."),
    },
    PyMethodDef{
        "core",
        py_handle_core,
        METH_NOARGS,
        PyDoc_STR("Get the subset of assumptions that made the problem unsatisfiable."),
    },
    PyMethodDef{
        "model",
        py_handle_model,
        METH_NOARGS,
        PyDoc_STR("Get the current model if there is any."),
    },
    PyMethodDef{
        "last",
        py_handle_last,
        METH_NOARGS,
        PyDoc_STR("Get the last computed model, if any."),
    },
    PyMethodDef{
        "resume",
        py_handle_resume,
        METH_NOARGS,
        PyDoc_STR("Discard the last model and start searching for the next one."),
    },
    PyMethodDef{
        "wait",
        method_cast(py_handle_wait),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Wait for the solve call to finish or the next result with an optional timeout."),
    },
    PyMethodDef{
        "cancel",
        py_handle_cancel,
        METH_NOARGS,
        PyDoc_STR("Cancel the running search."),
    },
    PyMethodDef{
        "interrupt",
        py_handle_interrupt,
        METH_NOARGS,
        PyDoc_STR("Request the search to stop without waiting for it."),
    },
    PyMethodDef{
        "close",
        py_handle_close,
        METH_NOARGS,
        PyDoc_STR("Stop the search and close the handle."),
    },
    PyMethodDef{"__enter__", py_handle_enter, METH_NOARGS, PyDoc_STR("Return self.")},
    PyMethodDef{
        "__exit__",
        method_cast(py_handle_exit),
        METH_FASTCALL,
        PyDoc_STR("Stop the search and close the handle."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_handle_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("An object to interact with a running search.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_handle_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_handle_clear)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_handle_iter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_handle_iternext)},
    PyType_Slot{Py_tp_methods, py_handle_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_handle_spec = PyType_Spec{
    "clingo.solve.SolveHandle",
    sizeof(PyHandleObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_handle_slots.data(),
};

// AsyncSolveHandle
//
// The solver thread only posts notifications to the event loop. Everything
// else, i.e., handing out models, resuming the search, and resolving futures,
// happens on the thread of the event loop.

struct PyAsyncHandleObject {
    PyHandleObject base;
    //! The event loop.
    PyObject *loop;
    //! The call_soon_threadsafe method of the event loop.
    PyObject *post;
    //! The future resolved with the solve result.
    PyObject *result;
    //! The future waiting for the next model (yield mode only).
    PyObject *next;
    //! The future resolved once __aexit__ closed the handle.
    PyObject *closed;
    //! Whether models are yielded.
    bool yield;
    //! Whether the handle is being destroyed and must not be notified anymore.
    bool dying;
    //! Whether the search was cancelled (models are no longer handed out).
    bool cancelled;
    //! Whether the search finished.
    bool finished;
    //! Whether a model arrived that was not handed out yet.
    bool model_ready;
    //! Whether a model was handed out and the search has to be resumed.
    bool need_resume;
    //! Whether the search is resumed automatically until it finished.
    bool draining;
};

auto async_cast(PyObject *obj) -> PyAsyncHandleObject * {
    return reinterpret_cast<PyAsyncHandleObject *>(obj);
}

auto async_self(PyAsyncHandleObject *a) -> PyObject * {
    return reinterpret_cast<PyObject *>(a);
}

//! Cancel the search unless it already finished.
//!
//! This does not wait for the search to stop. The result is passed via the
//! finish notification.
auto async_cancel(PyAsyncHandleObject *a) -> bool {
    auto *hnd = a->base.hnd;
    if (a->cancelled || a->finished || hnd == nullptr) {
        return true;
    }
    // NOTE: The handle keeps itself alive until the finish notification. Otherwise, dropping the handle after
    // cancelling would block until the search stopped and a pending __aexit__ would never finish. The reference leaks
    // if the loop is closed before.
    Py_INCREF(async_self(a));
    a->cancelled = true;
    a->model_ready = false;
    a->need_resume = false;
    model_clear(a->base.cur_model);
    if (!clingo_solve_handle_interrupt(hnd)) {
        py_handle_error();
        return false;
    }
    return true;
}

//! Resume the search after a model has been handed out.
auto async_resume(PyAsyncHandleObject *a) -> bool {
    model_clear(a->base.cur_model);
    a->need_resume = false;
    auto *hnd = a->base.hnd;
    if (!with_gil_released([&] { return clingo_solve_handle_resume(hnd); })) {
        py_handle_error();
        return false;
    }
    return true;
}

//! Resume the search past a model that arrived but was not handed out.
auto async_skip(PyAsyncHandleObject *a) -> bool {
    a->model_ready = false;
    // NOTE: wait until the model is available to not resume too early
    clingo_model_t *mdl = nullptr;
    return fetch_model(a->base.hnd, &mdl) && async_resume(a);
}

//! Resume the search until it finished passing models to the callbacks only.
auto async_drain(PyAsyncHandleObject *a) -> bool {
    if (a->draining || a->cancelled || a->finished || a->base.hnd == nullptr) {
        return true;
    }
    a->draining = true;
    if (a->model_ready) {
        return async_skip(a);
    }
    if (a->need_resume) {
        return async_resume(a);
    }
    return true;
}

//! Hand out the model that arrived to the given future.
auto async_deliver(PyAsyncHandleObject *a, PyObject *fut) -> bool {
    a->model_ready = false;
    auto *state = state_of(async_self(a));
    if (state == nullptr) {
        return false;
    }
    clingo_model_t *mdl = nullptr;
    if (!fetch_model(a->base.hnd, &mdl)) {
        return false;
    }
    if (mdl == nullptr) {
        return future_stop(fut);
    }
    auto *obj = model_set(a->base.cur_model, state->model_type, mdl, true);
    if (obj == nullptr) {
        return false;
    }
    a->need_resume = true;
    return future_set_result(fut, obj);
}

//! Pass the current error to the given future (or the result) and stop the search.
void async_fail(PyAsyncHandleObject *a, PyObject *fut) {
    auto exc = steal(take_error());
    if (exc.get() == nullptr || !future_set_exception(fut != nullptr ? fut : a->result, exc.get())) {
        PyErr_WriteUnraisable(async_self(a));
    }
    if (!async_cancel(a)) {
        PyErr_WriteUnraisable(async_self(a));
    }
}

auto py_async_notify_model(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *a = async_cast(self);
    if (a->base.hnd == nullptr || a->cancelled || a->finished) {
        Py_RETURN_NONE;
    }
    if (a->draining) {
        if (!async_skip(a)) {
            async_fail(a, nullptr);
        }
        Py_RETURN_NONE;
    }
    auto next = steal(std::exchange(a->next, nullptr));
    auto done = next.get() != nullptr ? future_done(next.get()) : 1;
    if (done < 0 || (done == 0 && !async_deliver(a, next.get()))) {
        async_fail(a, next.get());
    } else if (done > 0) {
        a->model_ready = true;
    }
    Py_RETURN_NONE;
}

auto py_async_notify_finish(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *a = async_cast(self);
    // release the reference taken when cancelling (the notification still holds one)
    auto keep = steal(a->cancelled ? self : nullptr);
    a->finished = true;
    a->model_ready = false;
    model_clear(a->base.cur_model);
    auto next = steal(std::exchange(a->next, nullptr));
    auto *state = state_of(self);
    auto *hnd = a->base.hnd;
    clingo_solve_result_bitset_t res = 0;
    auto ok = state != nullptr && with_gil_released([&] { return clingo_solve_handle_get(hnd, &res); });
    if (!ok && state != nullptr) {
        py_handle_error();
    }
    auto obj = steal(ok ? result_new(state, res) : nullptr);
    if (obj.get() != nullptr) {
        ok = future_set_result(a->result, obj.get()) && (next.get() == nullptr || future_stop(next.get()));
    } else {
        auto exc = steal(take_error());
        ok = exc.get() != nullptr && future_set_exception(a->result, exc.get()) &&
             (next.get() == nullptr || future_set_exception(next.get(), exc.get()));
    }
    if (!ok) {
        PyErr_WriteUnraisable(self);
    }
    if (a->closed != nullptr) {
        close_handle(&a->base);
        auto closed = steal(std::exchange(a->closed, nullptr));
        if (!future_set_result(closed.get(), Py_None)) {
            PyErr_WriteUnraisable(self);
        }
    }
    Py_RETURN_NONE;
}

// NOTE: the definitions are bound to handles to post them to the event loop
PyMethodDef async_notify_model_def = {"_notify_model", py_async_notify_model, METH_NOARGS, nullptr};
PyMethodDef async_notify_finish_def = {"_notify_finish", py_async_notify_finish, METH_NOARGS, nullptr};

auto async_model_cb(clingo_model_t *model, void *data, bool *goon) -> bool {
    auto *self = static_cast<PyObject *>(data);
    auto *a = async_cast(self);
    if (a->base.on_model != nullptr && !on_model_cb(model, data, goon)) {
        return false;
    }
    if (a->yield) {
        auto gil = GILGuard{};
        if (!a->dying && !loop_post(a->post, &async_notify_model_def, self)) {
            return store_python_error("notifying the event loop failed");
        }
    }
    return true;
}

void async_finish_cb([[maybe_unused]] clingo_solve_result_bitset_t result, void *data) {
    auto gil = GILGuard{};
    auto *self = static_cast<PyObject *>(data);
    if (!async_cast(self)->dying && !loop_post(async_cast(self)->post, &async_notify_finish_def, self)) {
        PyErr_WriteUnraisable(self);
    }
}

auto py_async_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    if (auto ret = py_handle_traverse(self, visit, arg); ret != 0) {
        return ret;
    }
    auto *a = async_cast(self);
    Py_VISIT(a->loop);
    Py_VISIT(a->post);
    Py_VISIT(a->result);
    Py_VISIT(a->next);
    Py_VISIT(a->closed);
    return 0;
}

auto py_async_clear(PyObject *self) -> int {
    auto *a = async_cast(self);
    // NOTE: closing releases the GIL and the solver thread must not post anymore
    a->dying = true;
    py_handle_clear(self);
    Py_CLEAR(a->loop);
    Py_CLEAR(a->post);
    Py_CLEAR(a->result);
    Py_CLEAR(a->next);
    Py_CLEAR(a->closed);
    return 0;
}

// AsyncStep
//
// Awaiting a future of the event loop directly would hide cancellations of
// the awaiting task if the future is already done. The step forwards to the
// iterator of the future and interrupts the search if an exception is thrown
// into the awaiting coroutine.

struct PyAsyncStepObject {
    PyObject_HEAD
    //! The handle whose search is interrupted.
    PyObject *handle;
    //! The iterator of the awaited future.
    PyObject *iter;
};

auto step_cast(PyObject *obj) -> PyAsyncStepObject * {
    return reinterpret_cast<PyAsyncStepObject *>(obj);
}

//! Create a step awaiting the given future on behalf of the handle.
auto step_new(PyObject *handle, PyObject *fut) -> PyObject * {
    auto *state = state_of(handle);
    if (state == nullptr) {
        return nullptr;
    }
    auto iter = steal(PyObject_CallMethod(fut, "__await__", nullptr));
    if (iter.get() == nullptr) {
        return nullptr;
    }
    auto *step = alloc_object<PyAsyncStepObject>(state->async_step_type);
    if (step == nullptr) {
        return nullptr;
    }
    step->handle = Py_NewRef(handle);
    step->iter = iter.release();
    return reinterpret_cast<PyObject *>(step);
}

//! Call a method of the iterator of the future with the given arguments.
auto step_call(PyObject *self, char const *name, PyObject *const *args, Py_ssize_t nargs) -> PyObject * {
    auto meth = steal(PyObject_GetAttrString(step_cast(self)->iter, name));
    auto tuple = steal(meth.get() != nullptr ? PyTuple_New(nargs) : nullptr);
    if (tuple.get() == nullptr) {
        return nullptr;
    }
    for (Py_ssize_t i = 0; i < nargs; ++i) {
        auto *arg = std::span{args, static_cast<size_t>(nargs)}[static_cast<size_t>(i)];
        PyTuple_SetItem(tuple.get(), i, Py_NewRef(arg));
    }
    return PyObject_Call(meth.get(), tuple.get(), nullptr);
}

auto py_step_send(PyObject *self, PyObject *value) -> PyObject * {
    return step_call(self, "send", &value, 1);
}

auto py_step_throw(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> PyObject * {
    if (!async_cancel(async_cast(step_cast(self)->handle))) {
        PyErr_WriteUnraisable(step_cast(self)->handle);
    }
    return step_call(self, "throw", args, nargs);
}

auto py_step_close(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return step_call(self, "close", nullptr, 0);
}

auto py_step_iternext(PyObject *self) -> PyObject * {
    return py_step_send(self, Py_None);
}

auto py_step_await(PyObject *self) -> PyObject * {
    return Py_NewRef(self);
}

auto py_step_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(step_cast(self)->handle);
    Py_VISIT(step_cast(self)->iter);
    return 0;
}

auto py_step_clear(PyObject *self) -> int {
    Py_CLEAR(step_cast(self)->handle);
    Py_CLEAR(step_cast(self)->iter);
    return 0;
}

auto py_step_methods = std::array{
    PyMethodDef{"send", py_step_send, METH_O, PyDoc_STR("Send a value into the awaited future.")},
    PyMethodDef{
        "throw",
        method_cast(py_step_throw),
        METH_FASTCALL,
        PyDoc_STR("Interrupt the search and throw an exception into the awaited future."),
    },
    PyMethodDef{"close", py_step_close, METH_NOARGS, PyDoc_STR("Close the iterator of the awaited future.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_step_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("An awaitable step of an asynchronous search.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_step_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_step_clear)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_step_await)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_step_iternext)},
    PyType_Slot{Py_am_await, reinterpret_cast<void *>(py_step_await)},
    PyType_Slot{Py_tp_methods, py_step_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_step_spec = PyType_Spec{
    "clingo.solve._AsyncStep",
    sizeof(PyAsyncStepObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_step_slots.data(),
};

auto py_async_await(PyObject *self) -> PyObject * {
    // NOTE: Each await uses its own future so that cancelling an awaiting task does not cancel the result, which can
    // then still be awaited to wait until the search stopped.
    auto *a = async_cast(self);
    if (!async_drain(a)) {
        return nullptr;
    }
    auto fut = steal(future_new(a->loop));
    if (fut.get() == nullptr || !future_chain(a->result, fut.get())) {
        return nullptr;
    }
    return step_new(self, fut.get());
}

auto py_async_aiter(PyObject *self) -> PyObject * {
    if (!async_cast(self)->yield) {
        PyErr_SetString(PyExc_TypeError, "models can only be iterated with yield_=True");
        return nullptr;
    }
    return Py_NewRef(self);
}

auto py_async_anext(PyObject *self) -> PyObject * {
    auto *a = async_cast(self);
    if (!a->yield) {
        PyErr_SetString(PyExc_TypeError, "models can only be iterated with yield_=True");
        return nullptr;
    }
    if (ensure_handle_open(self) == nullptr) {
        return nullptr;
    }
    if (a->next != nullptr) {
        auto done = future_done(a->next);
        if (done < 0) {
            return nullptr;
        }
        if (done == 0) {
            PyErr_SetString(PyExc_RuntimeError, "the next model is already awaited");
            return nullptr;
        }
        Py_CLEAR(a->next);
    }
    auto fut = steal(future_new(a->loop));
    if (fut.get() == nullptr) {
        return nullptr;
    }
    auto ok = true;
    if (a->finished || a->cancelled || a->draining) {
        ok = future_stop(fut.get());
    } else if (a->need_resume && !async_resume(a)) {
        ok = false;
    } else if (a->model_ready) {
        ok = async_deliver(a, fut.get());
    } else {
        a->next = Py_NewRef(fut.get());
    }
    return ok ? step_new(self, fut.get()) : nullptr;
}

auto py_async_cancel(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    if (!async_cancel(async_cast(self))) {
        return nullptr;
    }
    Py_RETURN_NONE;
}

auto py_async_last(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    if (ensure_handle_open(self) == nullptr) {
        return nullptr;
    }
    if (!async_cast(self)->finished) {
        Py_RETURN_NONE;
    }
    return py_handle_last(self, nullptr);
}

auto py_async_core(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    if (ensure_handle_open(self) == nullptr) {
        return nullptr;
    }
    if (!async_cast(self)->finished) {
        return PyList_New(0);
    }
    return py_handle_core(self, nullptr);
}

auto py_async_aenter(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto fut = steal(future_new(async_cast(self)->loop));
    if (fut.get() == nullptr || !future_set_result(fut.get(), self)) {
        return nullptr;
    }
    return fut.release();
}

auto py_async_aexit(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> PyObject * {
    PyObject *type = unset_arg();
    PyObject *value = unset_arg();
    PyObject *traceback = unset_arg();
    if (!parse_args("__aexit__", {"type", "value", "traceback"}, {}, args, nargs, {&type, &value, &traceback})) {
        return nullptr;
    }
    auto *a = async_cast(self);
    auto fut = steal(future_new(a->loop));
    if (fut.get() == nullptr) {
        return nullptr;
    }
    if (a->base.hnd == nullptr || a->finished) {
        close_handle(&a->base);
        return future_set_result(fut.get(), Py_None) ? fut.release() : nullptr;
    }
    if (a->closed != nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "the solve handle is already being closed");
        return nullptr;
    }
    if (!async_cancel(a)) {
        return nullptr;
    }
    a->closed = Py_NewRef(fut.get());
    return fut.release();
}

auto py_async_methods = std::array{
    PyMethodDef{
        "cancel",
        py_async_cancel,
        METH_NOARGS,
        PyDoc_STR("Cancel the running search."),
    },
    PyMethodDef{
        "last",
        py_async_last,
        METH_NOARGS,
        PyDoc_STR("Get the last computed model once the search finished, if any."),
    },
    PyMethodDef{
        "core",
        py_async_core,
        METH_NOARGS,
        PyDoc_STR("Get the subset of assumptions that made the problem unsatisfiable."),
    },
    PyMethodDef{"__aenter__", py_async_aenter, METH_NOARGS, PyDoc_STR("Return self.")},
    PyMethodDef{
        "__aexit__",
        method_cast(py_async_aexit),
        METH_FASTCALL,
        PyDoc_STR("Stop the search and close the handle."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_async_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("An object to await a search running in the background.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_async_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_async_clear)},
    PyType_Slot{Py_am_await, reinterpret_cast<void *>(py_async_await)},
    PyType_Slot{Py_am_aiter, reinterpret_cast<void *>(py_async_aiter)},
    PyType_Slot{Py_am_anext, reinterpret_cast<void *>(py_async_anext)},
    PyType_Slot{Py_tp_methods, py_async_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_async_spec = PyType_Spec{
    "clingo.solve.AsyncSolveHandle",
    sizeof(PyAsyncHandleObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_async_slots.data(),
};

// module

constexpr auto model_type_members = std::array<IntEnumMember, 3>{
    IntEnumMember{"StableModel", static_cast<int>(clingo_model_type_stable_model)},
    IntEnumMember{"CautiousConsequences", static_cast<int>(clingo_model_type_cautious_consequences)},
    IntEnumMember{"BraveConsequences", static_cast<int>(clingo_model_type_brave_consequences)},
};

auto solve_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<SolveModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->symbol_type);
    Py_VISIT(state->stats_type);
    Py_VISIT(state->base_type);
    Py_VISIT(state->model_type_enum);
    Py_VISIT(state->control_type);
    Py_VISIT(state->model_type);
    Py_VISIT(state->result_type);
    Py_VISIT(state->handle_type);
    Py_VISIT(state->async_handle_type);
    Py_VISIT(state->async_step_type);
    return 0;
}

auto solve_module_clear(PyObject *module) -> int {
    auto *state = static_cast<SolveModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->symbol_type);
    Py_CLEAR(state->stats_type);
    Py_CLEAR(state->base_type);
    Py_CLEAR(state->model_type_enum);
    Py_CLEAR(state->control_type);
    Py_CLEAR(state->model_type);
    Py_CLEAR(state->result_type);
    Py_CLEAR(state->handle_type);
    Py_CLEAR(state->async_handle_type);
    Py_CLEAR(state->async_step_type);
    return 0;
}

void solve_module_free(void *module) {
    solve_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef solve_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.solve",
    PyDoc_STR("Functions and classes related to solving."),
    static_cast<Py_ssize_t>(sizeof(SolveModuleState)),
    nullptr,
    nullptr,
    solve_module_traverse,
    solve_module_clear,
    solve_module_free,
};

} // namespace

auto py_solve_handle_type() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.solve"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    if (state->handle_type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.solve module is not initialized");
        return nullptr;
    }
    return Py_NewRef(state->handle_type);
}

auto py_model_type() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.solve"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    if (state->model_type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.solve module is not initialized");
        return nullptr;
    }
    return Py_NewRef(state->model_type);
}

auto py_model_new(PyObject *type, clingo_model_t const *model, bool is_mutable) -> PyObject * {
    return model_new(type, model, is_mutable);
}

auto py_model_get(PyObject *type, PyObject *obj, bool mut) -> clingo_model_t * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    if (PyObject_TypeCheck(obj, to) == 0) {
        PyErr_SetString(PyExc_TypeError, "object is not a Model");
        return nullptr;
    }
    return model_get(obj, mut);
}

void py_model_clear(PyObject *obj) {
    model_clear(obj);
}

auto py_solve_start(PyObject *type, clingo_control_t *ctl, PyObject *owner,
                    std::span<clingo_literal_t const> assumptions, PyObject *on_model, PyObject *on_unsat,
                    PyObject *on_stats, PyObject *on_finish, bool yield, bool async) -> PyObject * {
    if (!check_callback(on_model, "on_model") || !check_callback(on_unsat, "on_unsat") ||
        !check_callback(on_stats, "on_stats") || !check_callback(on_finish, "on_finish")) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    auto obj = steal(handle_new(state, owner, on_model, on_unsat, on_stats, on_finish));
    if (obj.get() == nullptr) {
        return nullptr;
    }
    auto *h = handle_cast(obj.get());

    auto mode = clingo_solve_mode_bitset_t{0};
    if (yield) {
        mode |= clingo_solve_mode_yield;
    }
    if (async) {
        mode |= clingo_solve_mode_async;
    }
    auto handler = clingo_solve_event_handler_t{
        on_model != nullptr ? on_model_cb : nullptr,
        on_unsat != nullptr ? on_unsat_cb : nullptr,
        on_stats != nullptr ? on_stats_cb : nullptr,
        on_finish != nullptr ? on_finish_cb : nullptr,
        nullptr,
    };
    auto has_handler = on_model != nullptr || on_unsat != nullptr || on_stats != nullptr || on_finish != nullptr;
    auto ok = with_gil_released([&] {
        return clingo_control_solve(ctl, mode, assumptions.data(), assumptions.size(), has_handler ? &handler : nullptr,
                                    static_cast<void *>(obj.get()), &h->hnd);
    });
    if (!ok) {
        py_handle_error();
        return nullptr;
    }
    return obj.release();
}

auto py_solve(PyObject *type, clingo_control_t *ctl, PyObject *owner, std::span<clingo_literal_t const> assumptions,
              PyObject *on_model, PyObject *on_unsat, PyObject *on_stats, PyObject *on_finish) -> PyObject * {
    auto obj =
        steal(py_solve_start(type, ctl, owner, assumptions, on_model, on_unsat, on_stats, on_finish, false, false));
    if (obj.get() == nullptr) {
        return nullptr;
    }
    auto *h = handle_cast(obj.get());
    auto *state = state_of(obj.get());
    if (state == nullptr) {
        return nullptr;
    }
    clingo_solve_result_bitset_t res = 0;
    if (!with_gil_released([&] { return clingo_solve_handle_get(h->hnd, &res); })) {
        return py_handle_error();
    }
    close_handle(h);
    return result_new(state, res);
}

auto py_solve_async(PyObject *type, clingo_control_t *ctl, PyObject *owner,
                    std::span<clingo_literal_t const> assumptions, PyObject *on_model, PyObject *on_unsat,
                    PyObject *on_stats, PyObject *loop, bool yield) -> PyObject * {
    if (!check_callback(on_model, "on_model") || !check_callback(on_unsat, "on_unsat") ||
        !check_callback(on_stats, "on_stats")) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    auto post = steal(PyObject_GetAttrString(loop, "call_soon_threadsafe"));
    auto result = steal(post.get() != nullptr ? future_new(loop) : nullptr);
    if (result.get() == nullptr) {
        return nullptr;
    }
    auto obj = steal(reinterpret_cast<PyObject *>(alloc_object<PyAsyncHandleObject>(state->async_handle_type)));
    if (obj.get() == nullptr) {
        return nullptr;
    }
    auto *a = async_cast(obj.get());
    a->base.owner = Py_NewRef(owner);
    a->base.on_model = Py_XNewRef(on_model);
    a->base.on_unsat = Py_XNewRef(on_unsat);
    a->base.on_stats = Py_XNewRef(on_stats);
    a->loop = Py_NewRef(loop);
    a->post = post.release();
    a->result = result.release();
    a->yield = yield;

    auto mode = clingo_solve_mode_bitset_t{clingo_solve_mode_async};
    if (yield) {
        mode |= clingo_solve_mode_yield;
    }
    auto handler = clingo_solve_event_handler_t{
        on_model != nullptr || yield ? async_model_cb : nullptr,
        on_unsat != nullptr ? on_unsat_cb : nullptr,
        on_stats != nullptr ? on_stats_cb : nullptr,
        async_finish_cb,
        nullptr,
    };
    auto ok = with_gil_released([&] {
        return clingo_control_solve(ctl, mode, assumptions.data(), assumptions.size(), &handler,
                                    static_cast<void *>(obj.get()), &a->base.hnd);
    });
    if (!ok) {
        py_handle_error();
        return nullptr;
    }
    return obj.release();
}

auto create_solve(PyObject *parent) -> PyObject * {
    auto solve_mod = PyGuard{nullptr};
    if (!add_submodule(parent, &solve_def, &solve_mod.get())) {
        return nullptr;
    }
    auto *state = get_state(solve_mod.get());
    if (state == nullptr ||
        !add_int_enum(solve_mod.get(), "ModelType", model_type_members, &state->model_type_enum, {}) ||
        !add_type(solve_mod.get(), "SolveControl", &py_control_spec, &state->control_type) ||
        !add_type(solve_mod.get(), "Model", &py_model_spec, &state->model_type) ||
        !add_type(solve_mod.get(), "SolveResult", &py_result_spec, &state->result_type) ||
        !add_type(solve_mod.get(), "SolveHandle", &py_handle_spec, &state->handle_type) ||
        !add_type(solve_mod.get(), "AsyncSolveHandle", &py_async_spec, &state->async_handle_type)) {
        return nullptr;
    }
    // NOTE: the step type is an implementation detail and not added to the module
    state->async_step_type = PyType_FromModuleAndSpec(solve_mod.get(), &py_step_spec, nullptr);
    if (state->async_step_type == nullptr) {
        return nullptr;
    }
    return solve_mod.release();
}

auto link_solve(PyObject *mod) -> bool {
    static constexpr auto imports = std::array{
        TypeImport<SolveModuleState>{&SolveModuleState::symbol_type, &py_symbol_type},
        TypeImport<SolveModuleState>{&SolveModuleState::base_type, &py_base_type},
        TypeImport<SolveModuleState>{&SolveModuleState::stats_type, &py_stats_type},
    };
    auto *state = get_state(mod);
    return state != nullptr && import_types(state, imports);
}

} // namespace PyClingo
