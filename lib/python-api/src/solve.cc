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
#include <cstdio>
#include <cstdlib>
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

auto py_model_symbols(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    static constexpr auto kwlist = std::array<char const *, 5>{"shown", "atoms", "terms", "theory", nullptr};
    int shown = 0;
    int atoms = 0;
    int terms = 0;
    int theory = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|pppp:symbols", cast_kw(kwlist), &shown, &atoms, &terms, &theory) ==
        0) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_show_type_bitset_t show = 0;
    if (shown != 0) {
        show |= clingo_show_type_shown;
    }
    if (atoms != 0) {
        show |= clingo_show_type_atoms;
    }
    if (terms != 0) {
        show |= clingo_show_type_terms;
    }
    if (theory != 0) {
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

auto py_model_contains(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    static constexpr auto kwlist = std::array<char const *, 2>{"atom", nullptr};
    PyObject *atom = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:contains", cast_kw(kwlist), &atom) == 0) {
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

auto py_model_is_true(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    static constexpr auto kwlist = std::array<char const *, 2>{"literal", nullptr};
    int lit = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "i:is_true", cast_kw(kwlist), &lit) == 0) {
        return nullptr;
    }
    bool res = false;
    if (!clingo_model_is_true(mdl, static_cast<clingo_literal_t>(lit), &res)) {
        return py_handle_error();
    }
    return PyBool_FromLong(res ? 1 : 0);
}

auto py_model_is_consequence(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto const *mdl = model_get(self, false);
    if (mdl == nullptr) {
        return nullptr;
    }
    static constexpr auto kwlist = std::array<char const *, 2>{"literal", nullptr};
    int lit = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "i:is_consequence", cast_kw(kwlist), &lit) == 0) {
        return nullptr;
    }
    clingo_consequence_t res = 0;
    if (!clingo_model_is_consequence(mdl, static_cast<clingo_literal_t>(lit), &res)) {
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

auto py_model_extend(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *mdl = model_get(self, true);
    if (mdl == nullptr) {
        return nullptr;
    }
    static constexpr auto kwlist = std::array<char const *, 2>{"symbols", nullptr};
    PyObject *symbols = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:extend", cast_kw(kwlist), &symbols) == 0) {
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
    Py_INCREF(m->control);
    return m->control;
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
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Get the symbols in the model."),
    },
    PyMethodDef{
        "contains",
        method_cast(py_model_contains),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check if the model contains the given atom."),
    },
    PyMethodDef{
        "is_true",
        method_cast(py_model_is_true),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check if the given program literal is true."),
    },
    PyMethodDef{
        "is_consequence",
        method_cast(py_model_is_consequence),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check if the given program literal is a consequence."),
    },
    PyMethodDef{
        "extend",
        method_cast(py_model_extend),
        METH_VARARGS | METH_KEYWORDS,
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

auto py_control_add_clause(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"clause", nullptr};
    PyObject *lits = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:add_clause", cast_kw(kwlist), &lits) == 0) {
        return nullptr;
    }
    return control_add(self, lits, false);
}

auto py_control_add_nogood(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"nogood", nullptr};
    PyObject *lits = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:add_nogood", cast_kw(kwlist), &lits) == 0) {
        return nullptr;
    }
    return control_add(self, lits, true);
}

auto py_control_methods = std::array{
    PyMethodDef{
        "add_clause",
        method_cast(py_control_add_clause),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Add a clause that applies to the current solving step."),
    },
    PyMethodDef{
        "add_nogood",
        method_cast(py_control_add_nogood),
        METH_VARARGS | METH_KEYWORDS,
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
    PyErr_Print();
    fprintf(stderr, "panic: solve finish callback failed\n");
    std::abort();
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
    h->owner = owner;
    h->on_model = on_model;
    h->on_unsat = on_unsat;
    h->on_stats = on_stats;
    h->on_finish = on_finish;
    Py_XINCREF(owner);
    Py_XINCREF(on_model);
    Py_XINCREF(on_unsat);
    Py_XINCREF(on_stats);
    Py_XINCREF(on_finish);
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

auto py_handle_resume(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    model_clear(handle_cast(self)->cur_model);
    if (!clingo_solve_handle_resume(hnd)) {
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
    Py_XINCREF(obj);
    return obj;
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
    if (!clingo_solve_handle_last(hnd, &mdl)) {
        return py_handle_error();
    }
    if (mdl == nullptr) {
        Py_RETURN_NONE;
    }
    auto *obj = model_set(handle_cast(self)->cur_model, state->model_type, mdl, false);
    Py_XINCREF(obj);
    return obj;
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

auto py_handle_wait(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"timeout", nullptr};
    PyObject *timeout_obj = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O:wait", cast_kw(kwlist), &timeout_obj) == 0) {
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
    Py_INCREF(self);
    return self;
}

auto py_handle_exit(PyObject *self, PyObject *args) -> PyObject * {
    PyObject *type = nullptr;
    PyObject *value = nullptr;
    PyObject *traceback = nullptr;
    if (PyArg_ParseTuple(args, "OOO:__exit__", &type, &value, &traceback) == 0) {
        return nullptr;
    }
    close_handle(handle_cast(self));
    Py_RETURN_NONE;
}

auto py_handle_iter(PyObject *self) -> PyObject * {
    if (ensure_handle_open(self) == nullptr) {
        return nullptr;
    }
    Py_INCREF(self);
    return self;
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
    if (!clingo_solve_handle_resume(hnd)) {
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
    Py_XINCREF(obj);
    return obj;
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
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Wait for the solve call to finish or the next result with an optional timeout."),
    },
    PyMethodDef{
        "cancel",
        py_handle_cancel,
        METH_NOARGS,
        PyDoc_STR("Cancel the running search."),
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
        py_handle_exit,
        METH_VARARGS,
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
    Py_INCREF(state->handle_type);
    return state->handle_type;
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
    Py_INCREF(state->model_type);
    return state->model_type;
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
        !add_type(solve_mod.get(), "SolveHandle", &py_handle_spec, &state->handle_type)) {
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
