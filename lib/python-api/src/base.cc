#define PY_SSIZE_T_CLEAN

#include "base.hh"

#include "symbol.hh"
#include "util.hh"

#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <utility>

namespace PyClingo {

namespace {

using PyAtomObject = struct {
    PyObject_HEAD const clingo_atom_base_t *base;
    size_t index;
};

using PyAtomBaseObject = struct {
    PyObject_HEAD const clingo_atom_base_t *base;
};

using PyTermObject = struct {
    PyObject_HEAD const clingo_term_base_t *base;
    size_t index;
};

using PyTermBaseObject = struct {
    PyObject_HEAD const clingo_term_base_t *base;
};

using PyTheoryTermObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
    size_t index;
};

using PyTheoryElementObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
    size_t index;
};

using PyTheoryAtomObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
    size_t index;
};

using PyTheoryBaseObject = struct {
    PyObject_HEAD const clingo_theory_base_t *base;
};

using PyBaseObject = struct {
    PyObject_HEAD const clingo_base_t *base;
};

enum class IterType : uint8_t { keys, values, items };

using PyBaseIter = struct {
    PyObject_HEAD clingo_base_t *base;
    Py_ssize_t index;
    Py_ssize_t size;
    IterType kind;
};

using PyAtomBaseIter = struct {
    PyObject_HEAD const clingo_atom_base_t *base;
    Py_ssize_t index;
    Py_ssize_t size;
    IterType kind;
};

using PyTermBaseIter = struct {
    PyObject_HEAD const clingo_term_base_t *base;
    Py_ssize_t index;
    Py_ssize_t size;
    IterType kind;
};

struct BaseModuleState {
    PyObject *atom_type{nullptr};
    PyObject *atom_base_type{nullptr};
    PyObject *atom_base_iter_type{nullptr};
    PyObject *term_type{nullptr};
    PyObject *term_base_type{nullptr};
    PyObject *term_base_iter_type{nullptr};
    PyObject *theory_term_type{nullptr};
    PyObject *theory_element_type{nullptr};
    PyObject *theory_atom_type{nullptr};
    PyObject *theory_base_type{nullptr};
    PyObject *base_type{nullptr};
    PyObject *base_iter_type{nullptr};
    PyObject *theory_term_type_enum{nullptr};
};

auto get_state(PyObject *module) -> BaseModuleState * {
    auto *state = static_cast<BaseModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "base module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_self(PyObject *self) -> BaseModuleState * {
    auto *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve base module");
        return nullptr;
    }
    return get_state(mod);
}

// Atom

auto py_atom_new(PyTypeObject *type, const clingo_atom_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = reinterpret_cast<PyAtomObject *>(obj);
    a->base = base;
    a->index = index;
    return obj;
}

auto py_atom_hash(PyObject *self) -> Py_hash_t {
    auto *a = reinterpret_cast<PyAtomObject *>(self);
    return static_cast<Py_hash_t>(hash_combine(a->index, std::hash<void const *>{}(a->base)));
}

auto py_atom_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyAtomObject *>(a);
    auto *y = reinterpret_cast<PyAtomObject *>(b);
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_atom_get_literal(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *a = reinterpret_cast<PyAtomObject *>(self);
    clingo_literal_t lit = 0;
    if (!clingo_atom_base_literal(a->base, a->index, &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

auto py_atom_get_symbol(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *a = reinterpret_cast<PyAtomObject *>(self);
    clingo_symbol_t sym = 0;
    if (!clingo_atom_base_symbol(a->base, a->index, &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo_object(sym, true);
}

auto py_atom_getset = std::array{
    PyGetSetDef{"literal", py_atom_get_literal, nullptr, PyDoc_STR("Program literal."), nullptr},
    PyGetSetDef{"symbol", py_atom_get_symbol, nullptr, PyDoc_STR("Atom symbol."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_atom_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a symbolic atom."))},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_atom_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_atom_richcompare)},
    PyType_Slot{Py_tp_getset, py_atom_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_atom_spec = PyType_Spec{
    "clingo.base.Atom",   sizeof(PyAtomObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_atom_slots.data(),
};

// AtomBase Iterator

auto py_atom_base_iter_new(PyObject *owner, IterType kind) -> PyObject * {
    auto *state = get_state_from_self(owner);
    if (state == nullptr || state->atom_base_iter_type == nullptr) {
        return nullptr;
    }
    auto *type = reinterpret_cast<PyTypeObject *>(state->atom_base_iter_type);
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }

    auto *it = reinterpret_cast<PyAtomBaseIter *>(obj);
    auto *a = reinterpret_cast<PyAtomBaseObject *>(owner);
    it->base = a->base;
    it->index = 0;
    it->kind = kind;

    size_t n = 0;
    if (!clingo_atom_base_size(it->base, &n)) {
        Py_DECREF(obj);
        return py_handle_error();
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        Py_DECREF(obj);
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return nullptr;
    }
    it->size = static_cast<Py_ssize_t>(n);
    return obj;
}

auto py_atom_base_iter_next(PyObject *self) -> PyObject * {
    auto *it = reinterpret_cast<PyAtomBaseIter *>(self);
    if (it->index >= it->size) {
        return nullptr;
    }

    auto idx = static_cast<size_t>(it->index++);
    if (it->kind == IterType::keys) {
        clingo_symbol_t sym = 0;
        if (!clingo_atom_base_symbol(it->base, idx, &sym)) {
            return py_handle_error();
        }
        return py_symbol_from_clingo_object(sym, true);
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    if (it->kind == IterType::values) {
        return py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), it->base, idx);
    }

    clingo_symbol_t sym = 0;
    if (!clingo_atom_base_symbol(it->base, idx, &sym)) {
        return py_handle_error();
    }
    auto key = Guard<PyObject *, Py_DECREF>(py_symbol_from_clingo_object(sym, true));
    auto val =
        Guard<PyObject *, Py_DECREF>(py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), it->base, idx));
    if (key.get() == nullptr || val.get() == nullptr) {
        return nullptr;
    }
    return PyTuple_Pack(2, key.get(), val.get());
}

auto py_atom_base_iter_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Lazy iterator over AtomBase."))},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(PyObject_SelfIter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_atom_base_iter_next)},
    PyType_Slot{0, nullptr},
};

auto py_atom_base_iter_spec = PyType_Spec{
    "clingo.base._AtomBaseIter",    sizeof(PyAtomBaseIter), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_atom_base_iter_slots.data(),
};

// AtomBase

auto py_atom_base_new(PyTypeObject *type, const clingo_atom_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = reinterpret_cast<PyAtomBaseObject *>(obj);
    a->base = base;
    return obj;
}

auto py_atom_base_len(PyObject *self) -> Py_ssize_t {
    auto *a = reinterpret_cast<PyAtomBaseObject *>(self);
    size_t n = 0;
    if (!clingo_atom_base_size(a->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto atom_base_lookup(PyObject *self, PyObject *key, size_t *idx, clingo_symbol_t *sym_out = nullptr) -> int {
    clingo_symbol_t sym = 0;
    size_t index = 0;
    if (auto res = py_symbol_to_clingo_check(key, &sym); res != 1) {
        return res;
    }
    auto *a = reinterpret_cast<PyAtomBaseObject *>(self);
    if (!clingo_atom_base_find(a->base, sym, &index)) {
        py_handle_error();
        return -1;
    }
    if (idx != nullptr) {
        *idx = index;
    }
    if (sym_out != nullptr) {
        *sym_out = sym;
    }
    return 1;
}

auto py_atom_base_contains(PyObject *self, PyObject *key) -> int {
    size_t idx = 0;
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return -1;
    }
    auto res = atom_base_lookup(self, key, &idx);
    if (res < 0) {
        return -1;
    }
    return res == 1 && std::cmp_less(idx, n) ? 1 : 0;
}

auto py_atom_base_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    size_t idx = 0;
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto res = atom_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        PyErr_SetString(PyExc_KeyError, "key not found");
        return nullptr;
    }
    auto *a = reinterpret_cast<PyAtomBaseObject *>(self);
    return py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), a->base, idx);
}

auto py_atom_base_get(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"key", "default", nullptr};
    PyObject *key = nullptr;
    PyObject *def = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", cast_kw(kwlist), &key, &def) == 0) {
        return nullptr;
    }

    size_t idx = 0;
    auto n = py_atom_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto res = atom_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        Py_INCREF(def);
        return def;
    }
    return py_atom_base_subscript(self, key);
}

auto py_atom_base_keys(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_atom_base_iter_new(self, IterType::keys);
}

auto py_atom_base_iter(PyObject *self) -> PyObject * {
    return py_atom_base_iter_new(self, IterType::keys);
}

auto py_atom_base_values(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_atom_base_iter_new(self, IterType::values);
}

auto py_atom_base_items(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_atom_base_iter_new(self, IterType::items);
}

auto py_atom_base_methods = std::array{
    PyMethodDef{
        "get",
        py_c_function_cast(py_atom_base_get),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Get value or default."),
    },
    PyMethodDef{"keys", py_atom_base_keys, METH_NOARGS, PyDoc_STR("Iterate keys.")},
    PyMethodDef{"values", py_atom_base_values, METH_NOARGS, PyDoc_STR("Iterate values.")},
    PyMethodDef{"items", py_atom_base_items, METH_NOARGS, PyDoc_STR("Iterate items.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_atom_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Mapping Symbol -> Atom."))},
    PyType_Slot{Py_tp_methods, py_atom_base_methods.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_atom_base_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_atom_base_subscript)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_atom_base_contains)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_atom_base_iter)},
    PyType_Slot{0, nullptr},
};

auto py_atom_base_spec = PyType_Spec{
    "clingo.base.AtomBase",
    sizeof(PyAtomBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING,
    py_atom_base_slots.data(),
};

// Term

auto py_term_new(PyTypeObject *type, const clingo_term_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermObject *>(obj);
    t->base = base;
    t->index = index;
    return obj;
}

auto py_term_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTermObject *>(self);
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_term_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTermObject *>(a);
    auto *y = reinterpret_cast<PyTermObject *>(b);
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_term_get_symbol(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTermObject *>(self);
    clingo_symbol_t sym = 0;
    if (!clingo_term_base_symbol(t->base, t->index, &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo_object(sym, true);
}

auto py_term_get_condition(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTermObject *>(self);
    size_t const *sizes = nullptr;
    clingo_literal_t const *const *lits = nullptr;
    size_t n = 0;
    if (!clingo_term_base_condition(t->base, t->index, &sizes, &lits, &n)) {
        return py_handle_error();
    }

    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }

    for (size_t i = 0; i < n; ++i) {
        auto inner = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(sizes[i])));
        if (inner.get() == nullptr) {
            return nullptr;
        }
        for (size_t j = 0; j < sizes[i]; ++j) {
            auto *v = PyLong_FromLong(lits[i][j]);
            if (v == nullptr) {
                return nullptr;
            }
            PyList_SET_ITEM(inner.get(), static_cast<Py_ssize_t>(j), v);
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), inner.release());
    }
    return out.release();
}

auto py_term_getset = std::array{
    PyGetSetDef{"symbol", py_term_get_symbol, nullptr, PyDoc_STR("Term symbol."), nullptr},
    PyGetSetDef{"condition", py_term_get_condition, nullptr, PyDoc_STR("Term condition."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_term_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a shown term."))},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_term_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_term_richcompare)},
    PyType_Slot{Py_tp_getset, py_term_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_term_spec = PyType_Spec{
    "clingo.base.Term",   sizeof(PyTermObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_term_slots.data(),
};

// TermBase Iterator

auto py_term_base_iter_new(PyObject *owner, IterType kind) -> PyObject * {
    auto *state = get_state_from_self(owner);
    if (state == nullptr || state->term_base_iter_type == nullptr) {
        return nullptr;
    }
    auto *type = reinterpret_cast<PyTypeObject *>(state->term_base_iter_type);
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }

    auto *it = reinterpret_cast<PyTermBaseIter *>(obj);
    auto *t = reinterpret_cast<PyTermBaseObject *>(owner);
    it->base = t->base;
    it->index = 0;
    it->kind = kind;

    size_t n = 0;
    if (!clingo_term_base_size(it->base, &n)) {
        Py_DECREF(obj);
        return py_handle_error();
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        Py_DECREF(obj);
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return nullptr;
    }
    it->size = static_cast<Py_ssize_t>(n);
    return obj;
}

auto py_term_base_iter_next(PyObject *self) -> PyObject * {
    auto *it = reinterpret_cast<PyTermBaseIter *>(self);
    if (it->index >= it->size) {
        return nullptr;
    }

    auto idx = static_cast<size_t>(it->index++);
    if (it->kind == IterType::keys) {
        clingo_symbol_t sym = 0;
        if (!clingo_term_base_symbol(it->base, idx, &sym)) {
            return py_handle_error();
        }
        return py_symbol_from_clingo_object(sym, true);
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    if (it->kind == IterType::values) {
        return py_term_new(reinterpret_cast<PyTypeObject *>(state->term_type), it->base, idx);
    }

    clingo_symbol_t sym = 0;
    if (!clingo_term_base_symbol(it->base, idx, &sym)) {
        return py_handle_error();
    }
    auto key = Guard<PyObject *, Py_DECREF>(py_symbol_from_clingo_object(sym, true));
    auto val =
        Guard<PyObject *, Py_DECREF>(py_term_new(reinterpret_cast<PyTypeObject *>(state->term_type), it->base, idx));
    if (key.get() == nullptr || val.get() == nullptr) {
        return nullptr;
    }
    return PyTuple_Pack(2, key.get(), val.get());
}

auto py_term_base_iter_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Lazy iterator over TermBase."))},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(PyObject_SelfIter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_term_base_iter_next)},
    PyType_Slot{0, nullptr},
};

auto py_term_base_iter_spec = PyType_Spec{
    "clingo.base._TermBaseIter",    sizeof(PyTermBaseIter), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_term_base_iter_slots.data(),
};

// TermBase

auto py_term_base_new(PyTypeObject *type, const clingo_term_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(obj);
    t->base = base;
    return obj;
}

auto py_term_base_len(PyObject *self) -> Py_ssize_t {
    auto *t = reinterpret_cast<PyTermBaseObject *>(self);
    size_t n = 0;
    if (!clingo_term_base_size(t->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto term_base_lookup(PyObject *self, PyObject *key, size_t *idx) -> int {
    clingo_symbol_t sym = 0;
    if (auto res = py_symbol_to_clingo_check(key, &sym); res != 1) {
        return res;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(self);
    size_t index = 0;
    if (!clingo_term_base_find(t->base, sym, &index)) {
        py_handle_error();
        return -1;
    }
    *idx = index;
    return 1;
}

auto py_term_base_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    size_t idx = 0;
    auto n = py_term_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    auto res = term_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        PyErr_SetString(PyExc_KeyError, "key not found");
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTermBaseObject *>(self);
    return py_term_new(reinterpret_cast<PyTypeObject *>(state->term_type), t->base, idx);
}

auto py_term_base_contains(PyObject *self, PyObject *key) -> int {
    size_t idx = 0;
    auto n = py_term_base_len(self);
    if (n < 0) {
        return -1;
    }
    auto res = term_base_lookup(self, key, &idx);
    if (res < 0) {
        return -1;
    }
    return res == 1 && std::cmp_less(idx, n) ? 1 : 0;
}

auto py_term_base_get(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"key", "default", nullptr};
    PyObject *key = nullptr;
    PyObject *def = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", cast_kw(kwlist), &key, &def) == 0) {
        return nullptr;
    }
    auto n = py_term_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    size_t idx = 0;
    auto res = term_base_lookup(self, key, &idx);
    if (res < 0) {
        return nullptr;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a Symbol");
        return nullptr;
    }
    if (std::cmp_greater_equal(idx, n)) {
        Py_INCREF(def);
        return def;
    }
    return py_term_base_subscript(self, key);
}

auto py_term_base_keys(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return py_term_base_iter_new(self, IterType::keys);
}

auto py_term_base_iter(PyObject *self) -> PyObject * {
    return py_term_base_iter_new(self, IterType::keys);
}

auto py_term_base_values(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return py_term_base_iter_new(self, IterType::values);
}

auto py_term_base_items(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return py_term_base_iter_new(self, IterType::items);
}

auto py_term_base_methods = std::array{
    PyMethodDef{
        "get",
        py_c_function_cast(py_term_base_get),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Get value or default."),
    },
    PyMethodDef{"keys", py_term_base_keys, METH_NOARGS, PyDoc_STR("Iterate keys.")},
    PyMethodDef{"values", py_term_base_values, METH_NOARGS, PyDoc_STR("Iterate values.")},
    PyMethodDef{"items", py_term_base_items, METH_NOARGS, PyDoc_STR("Iterate items.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_term_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Mapping Symbol -> Term."))},
    PyType_Slot{Py_tp_methods, py_term_base_methods.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_term_base_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_term_base_subscript)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_term_base_contains)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_term_base_iter)},
    PyType_Slot{0, nullptr},
};

auto py_term_base_spec = PyType_Spec{
    "clingo.base.TermBase",
    sizeof(PyTermBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING,
    py_term_base_slots.data(),
};

// TheoryTerm

auto py_theory_term_new(PyTypeObject *type, const clingo_theory_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryTermObject *>(obj);
    t->base = base;
    t->index = index;
    return obj;
}

auto py_theory_term_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self);
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_theory_term_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTheoryTermObject *>(a);
    auto *y = reinterpret_cast<PyTheoryTermObject *>(b);
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_term_str(PyObject *self) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_theory_base_term_to_string(t->base, t->index, bld)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_theory_term_get_type(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self);
    clingo_theory_term_type_t ty = 0;
    if (!clingo_theory_base_term_type(t->base, t->index, &ty)) {
        return py_handle_error();
    }
    return PyObject_CallFunction(state->theory_term_type_enum, "i", static_cast<int>(ty));
}

auto py_theory_term_get_number(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self);
    int num = 0;
    if (!clingo_theory_base_term_number(t->base, t->index, &num)) {
        return py_handle_error();
    }
    return PyLong_FromLong(num);
}

auto py_theory_term_get_name(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self);
    clingo_string_t name;
    if (!clingo_theory_base_term_name(t->base, t->index, &name)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size));
}

auto py_theory_term_get_arguments(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryTermObject *>(self);
    clingo_id_t const *args = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_term_arguments(t->base, t->index, &args, &n)) {
        return py_handle_error();
    }

    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *obj = py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, args[i]);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list.get(), static_cast<Py_ssize_t>(i), obj);
    }
    return list.release();
}

auto py_theory_term_getset = std::array{
    PyGetSetDef{"type", py_theory_term_get_type, nullptr, PyDoc_STR("Theory term type."), nullptr},
    PyGetSetDef{"number", py_theory_term_get_number, nullptr, PyDoc_STR("Numeric value."), nullptr},
    PyGetSetDef{"name", py_theory_term_get_name, nullptr, PyDoc_STR("Name."), nullptr},
    PyGetSetDef{"arguments", py_theory_term_get_arguments, nullptr, PyDoc_STR("Arguments."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_term_slots = std::array{
    PyType_Slot{Py_tp_doc, nullptr},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_theory_term_str)},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_theory_term_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_theory_term_richcompare)},
    PyType_Slot{Py_tp_getset, py_theory_term_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_term_spec = PyType_Spec{
    "clingo.base.TheoryTerm",    sizeof(PyTheoryTermObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_term_slots.data(),
};

// TheoryElement

auto py_theory_element_new(PyTypeObject *type, const clingo_theory_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryElementObject *>(obj);
    t->base = base;
    t->index = index;
    return obj;
}

auto py_theory_element_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self);
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_theory_element_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTheoryElementObject *>(a);
    auto *y = reinterpret_cast<PyTheoryElementObject *>(b);
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_element_str(PyObject *self) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_theory_base_element_to_string(t->base, t->index, bld)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_theory_element_get_tuple(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self);
    clingo_id_t const *vals = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_element_tuple(t->base, t->index, &vals, &n)) {
        return py_handle_error();
    }
    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *obj = py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, vals[i]);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), obj);
    }
    return out.release();
}

auto py_theory_element_get_condition(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self);
    clingo_literal_t const *vals = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_element_condition(t->base, t->index, &vals, &n)) {
        return py_handle_error();
    }
    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *v = PyLong_FromLong(vals[i]);
        if (v == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), v);
    }
    return out.release();
}

auto py_theory_element_get_condition_id(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryElementObject *>(self);
    clingo_literal_t id = 0;
    if (!clingo_theory_base_element_condition_id(t->base, t->index, &id)) {
        return py_handle_error();
    }
    if (id == 0) {
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(id);
}

auto py_theory_element_getset = std::array{
    PyGetSetDef{"tuple", py_theory_element_get_tuple, nullptr, PyDoc_STR("Term tuple."), nullptr},
    PyGetSetDef{"condition", py_theory_element_get_condition, nullptr, PyDoc_STR("Condition literals."), nullptr},
    PyGetSetDef{
        "condition_id",
        py_theory_element_get_condition_id,
        nullptr,
        PyDoc_STR("Condition id or None."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_element_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a theory element."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_theory_element_str)},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_theory_element_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_theory_element_richcompare)},
    PyType_Slot{Py_tp_getset, py_theory_element_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_element_spec = PyType_Spec{
    "clingo.base.TheoryElement",
    sizeof(PyTheoryElementObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_element_slots.data(),
};

// TheoryAtom

auto py_theory_atom_new(PyTypeObject *type, const clingo_theory_base_t *base, size_t index) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(obj);
    t->base = base;
    t->index = index;
    return obj;
}

auto py_theory_atom_hash(PyObject *self) -> Py_hash_t {
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self);
    return static_cast<Py_hash_t>(hash_combine(t->index, std::hash<void const *>{}(t->base)));
}

auto py_theory_atom_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = reinterpret_cast<PyTheoryAtomObject *>(a);
    auto *y = reinterpret_cast<PyTheoryAtomObject *>(b);
    bool eq = x->base == y->base && x->index == y->index;
    bool res = false;
    if (op == Py_EQ) {
        res = eq;
    } else if (op == Py_NE) {
        res = !eq;
    } else {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_atom_str(PyObject *self) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_theory_base_atom_to_string(t->base, t->index, bld)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_theory_atom_get_name(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self);
    clingo_id_t id = 0;
    if (!clingo_theory_base_atom_term(t->base, t->index, &id)) {
        return py_handle_error();
    }
    return py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, id);
}

auto py_theory_atom_get_elements(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self);
    clingo_id_t const *ids = nullptr;
    size_t n = 0;
    if (!clingo_theory_base_atom_elements(t->base, t->index, &ids, &n)) {
        return py_handle_error();
    }
    auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *obj =
            py_theory_element_new(reinterpret_cast<PyTypeObject *>(state->theory_element_type), t->base, ids[i]);
        if (obj == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), obj);
    }
    return out.release();
}

auto py_theory_atom_get_literal(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self);
    clingo_literal_t lit = 0;
    if (!clingo_theory_base_atom_literal(t->base, t->index, &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

auto py_theory_atom_get_guard(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryAtomObject *>(self);
    bool has_guard = false;
    if (!clingo_theory_base_atom_has_guard(t->base, t->index, &has_guard)) {
        return py_handle_error();
    }
    if (!has_guard) {
        Py_RETURN_NONE;
    }

    clingo_string_t op;
    clingo_id_t term = 0;
    if (!clingo_theory_base_atom_guard(t->base, t->index, &op, &term)) {
        return py_handle_error();
    }

    auto op_obj = Guard<PyObject *, Py_DECREF>(PyUnicode_FromStringAndSize(op.data, static_cast<Py_ssize_t>(op.size)));
    auto term_obj = Guard<PyObject *, Py_DECREF>(
        py_theory_term_new(reinterpret_cast<PyTypeObject *>(state->theory_term_type), t->base, term));
    if (op_obj.get() == nullptr || term_obj.get() == nullptr) {
        return nullptr;
    }
    return PyTuple_Pack(2, op_obj.get(), term_obj.get());
}

auto py_theory_atom_getset = std::array{
    PyGetSetDef{"name", py_theory_atom_get_name, nullptr, PyDoc_STR("Name term."), nullptr},
    PyGetSetDef{"elements", py_theory_atom_get_elements, nullptr, PyDoc_STR("Elements."), nullptr},
    PyGetSetDef{"literal", py_theory_atom_get_literal, nullptr, PyDoc_STR("Literal."), nullptr},
    PyGetSetDef{"guard", py_theory_atom_get_guard, nullptr, PyDoc_STR("Guard or None."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_atom_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("View over a theory atom."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_theory_atom_str)},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_theory_atom_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_theory_atom_richcompare)},
    PyType_Slot{Py_tp_getset, py_theory_atom_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_atom_spec = PyType_Spec{
    "clingo.base.TheoryAtom",    sizeof(PyTheoryAtomObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_atom_slots.data(),
};

// TheoryBase (sequence)

auto py_theory_base_new(PyTypeObject *type, const clingo_theory_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(obj);
    t->base = base;
    return obj;
}

auto py_theory_base_len(PyObject *self) -> Py_ssize_t {
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(self);
    size_t n = 0;
    if (!clingo_theory_base_size(t->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto py_theory_base_item(PyObject *self, Py_ssize_t i) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto n = py_theory_base_len(self);
    if (n < 0) {
        return nullptr;
    }
    if (i < 0 || i >= n) {
        PyErr_SetString(PyExc_IndexError, "atom index out of range");
        return nullptr;
    }
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(self);
    return py_theory_atom_new(reinterpret_cast<PyTypeObject *>(state->theory_atom_type), t->base,
                              static_cast<size_t>(i));
}

auto py_theory_base_contains(PyObject *self, PyObject *value) -> int {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return -1;
    }
    if (!PyObject_TypeCheck(value, reinterpret_cast<PyTypeObject *>(state->theory_atom_type))) {
        return 0;
    }
    auto *a = reinterpret_cast<PyTheoryAtomObject *>(value);
    auto *t = reinterpret_cast<PyTheoryBaseObject *>(self);
    size_t n = 0;
    if (!clingo_theory_base_size(t->base, &n)) {
        py_handle_error();
        return -1;
    }
    return (a->base == t->base && a->index < n) ? 1 : 0;
}

auto py_theory_base_iter(PyObject *self) -> PyObject * {
    return PySeqIter_New(self);
}

auto py_theory_base_methods = std::array{
    PyMethodDef{
        nullptr,
        nullptr,
        0,
        nullptr,
    },
};

auto py_theory_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Sequence of theory atoms."))},
    PyType_Slot{Py_tp_methods, py_theory_base_methods.data()},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(py_theory_base_len)},
    PyType_Slot{Py_sq_item, reinterpret_cast<void *>(py_theory_base_item)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_theory_base_contains)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_theory_base_iter)},
    PyType_Slot{0, nullptr},
};

auto py_theory_base_spec = PyType_Spec{
    "clingo.base.TheoryBase",
    sizeof(PyTheoryBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_SEQUENCE,
    py_theory_base_slots.data(),
};

// Base iterator

auto py_base_iter_new(PyObject *owner, IterType kind) -> PyObject * {
    auto *state = get_state_from_self(owner);
    if (state == nullptr || state->base_iter_type == nullptr) {
        return nullptr;
    }

    auto *type = reinterpret_cast<PyTypeObject *>(state->base_iter_type);
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }

    auto *it = reinterpret_cast<PyBaseIter *>(obj);
    auto *b = reinterpret_cast<PyBaseObject *>(owner);
    it->base = const_cast<clingo_base_t *>(b->base);

    size_t n = 0;
    if (!clingo_base_atoms_size(it->base, &n)) {
        py_handle_error();
        return nullptr;
    }

    it->index = 0;
    it->size = static_cast<Py_ssize_t>(n);
    it->kind = kind;
    return obj;
}

auto py_base_iter_next(PyObject *self) -> PyObject * {
    auto *it = reinterpret_cast<PyBaseIter *>(self);
    if (it->index >= it->size) {
        return nullptr;
    }

    clingo_signature_t sig{};
    clingo_atom_base_t const *atoms = nullptr;
    if (!clingo_base_atoms_at(it->base, static_cast<size_t>(it->index), &sig, &atoms)) {
        return py_handle_error();
    }
    ++it->index;

    if (it->kind == IterType::keys) {
        return Py_BuildValue("(s#nO)", sig.name, static_cast<Py_ssize_t>(sig.size), static_cast<Py_ssize_t>(sig.arity),
                             sig.is_positive ? Py_True : Py_False);
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    if (it->kind == IterType::values) {
        return py_atom_base_new(reinterpret_cast<PyTypeObject *>(state->atom_base_type), atoms);
    }

    auto key = Guard<PyObject *, Py_DECREF>(Py_BuildValue("(s#nO)", sig.name, static_cast<Py_ssize_t>(sig.size),
                                                          static_cast<Py_ssize_t>(sig.arity),
                                                          sig.is_positive ? Py_True : Py_False));
    auto val =
        Guard<PyObject *, Py_DECREF>(py_atom_base_new(reinterpret_cast<PyTypeObject *>(state->atom_base_type), atoms));
    if (key.get() == nullptr || val.get() == nullptr) {
        return nullptr;
    }
    return PyTuple_Pack(2, key.get(), val.get());
}

auto py_base_iter_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Lazy iterator over Base."))},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(PyObject_SelfIter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_base_iter_next)},
    PyType_Slot{0, nullptr},
};

auto py_base_iter_spec = PyType_Spec{
    "clingo.base._BaseIter",   sizeof(PyBaseIter), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_base_iter_slots.data(),
};

// Base

auto py_base_len(PyObject *self) -> Py_ssize_t {
    auto *b = reinterpret_cast<PyBaseObject *>(self);
    size_t n = 0;
    if (!clingo_base_atoms_size(b->base, &n)) {
        py_handle_error();
        return -1;
    }
    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto parse_signature_tuple(PyObject *tuple, clingo_signature_t *sig) -> int {
    auto n = PyTuple_Size(tuple);
    if (n != 2 && n != 3) {
        return 0;
    }

    auto *name_obj = PyTuple_GetItem(tuple, 0);                    // borrowed
    auto *arity_obj = PyTuple_GetItem(tuple, 1);                   // borrowed
    auto *sign_obj = n == 3 ? PyTuple_GetItem(tuple, 2) : Py_True; // borrowed

    if (!PyUnicode_Check(name_obj)) {
        return 0;
    }

    Py_ssize_t name_len = 0;
    const auto *name = PyUnicode_AsUTF8AndSize(name_obj, &name_len);
    if (name == nullptr) {
        return -1;
    }

    if (!PyLong_Check(arity_obj)) {
        return 0;
    }
    auto arity = PyLong_AsSize_t(arity_obj);
    if (PyErr_Occurred() != nullptr) {
        return -1;
    }

    int positive = PyObject_IsTrue(sign_obj);
    if (positive < 0) {
        return -1;
    }

    *sig = clingo_signature_t{name, static_cast<size_t>(name_len), arity, positive != 0};
    return 1;
}

auto parse_base_key(PyObject *key, clingo_signature_t *sig, clingo_symbol_t *sym_opt) -> int {
    if (PyTuple_Check(key) != 0) {
        return parse_signature_tuple(key, sig);
    }

    clingo_symbol_t sym = 0;
    auto is_sym = py_symbol_to_clingo_check(key, &sym);
    if (is_sym < 0) {
        return -1;
    }
    if (is_sym == 0) {
        return 0;
    }
    if (clingo_symbol_type(sym) != clingo_symbol_type_function) {
        return 0;
    }

    clingo_string_t name;
    clingo_symbol_t const *args = nullptr;
    size_t arity = 0;
    bool pos = false;
    if (!clingo_symbol_name(sym, &name) || !clingo_symbol_arguments(sym, &args, &arity) ||
        !clingo_symbol_is_positive(sym, &pos)) {
        py_handle_error();
        return -1;
    }

    *sig = clingo_signature_t{name.data, name.size, arity, pos};
    if (sym_opt != nullptr) {
        *sym_opt = sym;
    }
    return 1;
}

auto py_base_contains(PyObject *self, PyObject *key) -> int {
    auto *b = reinterpret_cast<PyBaseObject *>(self);
    clingo_signature_t sig{};
    clingo_symbol_t sym = 0;
    auto is_key = parse_base_key(key, &sig, &sym);
    if (is_key < 0) {
        return -1;
    }
    if (is_key == 0) {
        return 0;
    }

    clingo_atom_base_t const *atoms = nullptr;
    bool found = false;
    if (!clingo_base_atoms_find(b->base, &sig, &atoms, &found)) {
        py_handle_error();
        return -1;
    }
    if (!found || atoms == nullptr) {
        return 0;
    }

    if (PyTuple_Check(key) != 0) {
        return 1;
    }

    size_t idx = 0;
    if (!clingo_atom_base_find(atoms, sym, &idx)) {
        py_handle_error();
        return -1;
    }
    size_t n = 0;
    if (!clingo_atom_base_size(atoms, &n)) {
        py_handle_error();
        return -1;
    }
    return idx < n ? 1 : 0;
}

auto base_get(PyObject *self, PyObject *key, PyObject *def) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self);

    clingo_signature_t sig{};
    clingo_symbol_t sym = 0;
    auto is_key = parse_base_key(key, &sig, &sym);
    if (is_key < 0) {
        return nullptr;
    }
    if (is_key == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a signature tuple or a function symbol");
        return nullptr;
    }

    clingo_atom_base_t const *atoms = nullptr;
    bool found = false;
    if (!clingo_base_atoms_find(b->base, &sig, &atoms, &found)) {
        return py_handle_error();
    }
    if (!found || atoms == nullptr) {
        Py_INCREF(def);
        return def;
    }

    if (PyTuple_Check(key) != 0) {
        return py_atom_base_new(reinterpret_cast<PyTypeObject *>(state->atom_base_type), atoms);
    }

    size_t idx = 0;
    if (!clingo_atom_base_find(atoms, sym, &idx)) {
        return py_handle_error();
    }
    size_t n = 0;
    if (!clingo_atom_base_size(atoms, &n)) {
        return py_handle_error();
    }
    if (idx >= n) {
        Py_INCREF(def);
        return def;
    }
    return py_atom_new(reinterpret_cast<PyTypeObject *>(state->atom_type), atoms, idx);
}

auto py_base_get(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"key", "default", nullptr};
    PyObject *key = nullptr;
    PyObject *def = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:get", cast_kw(kwlist), &key, &def) == 0) {
        return nullptr;
    }
    return base_get(self, key, def);
}

auto py_base_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *res = base_get(self, key, Py_None);
    if (res == nullptr) {
        return nullptr;
    }
    if (res == Py_None) {
        Py_DECREF(res);
        PyErr_SetString(PyExc_KeyError, "key not found");
        return nullptr;
    }
    return res;
}

auto py_base_keys(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return py_base_iter_new(self, IterType::keys);
}

auto py_base_iter(PyObject *self) -> PyObject * {
    return py_base_iter_new(self, IterType::keys);
}

auto py_base_values(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return py_base_iter_new(self, IterType::values);
}

auto py_base_items(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return py_base_iter_new(self, IterType::items);
}

auto base_predicate(PyObject *self, PyObject *args, PyObject *kwargs,
                    bool (*fun)(clingo_base_t const *, clingo_literal_t, bool *), char const *name) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"literal", nullptr};
    clingo_literal_t lit = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "i", cast_kw(kwlist), &lit) == 0) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self);
    bool val = false;
    if (!fun(b->base, lit, &val)) {
        return py_handle_error();
    }
    (void)name;
    if (val) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_base_is_external(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_external, "is_external");
}

auto py_base_is_fact(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_fact, "is_fact");
}

auto py_base_is_shown(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_shown, "is_shown");
}

auto py_base_is_projected(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_projected, "is_projected");
}

auto py_base_is_current(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    return base_predicate(self, args, kwargs, clingo_base_is_current, "is_current");
}

auto py_base_get_terms(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self);
    clingo_term_base_t const *terms = nullptr;
    if (!clingo_base_terms(b->base, &terms)) {
        return py_handle_error();
    }
    return py_term_base_new(reinterpret_cast<PyTypeObject *>(state->term_base_type), terms);
}

auto py_base_get_theory(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(self);
    clingo_theory_base_t const *theory = nullptr;
    if (!clingo_base_theory(b->base, &theory)) {
        return py_handle_error();
    }
    return py_theory_base_new(reinterpret_cast<PyTypeObject *>(state->theory_base_type), theory);
}

auto py_base_getset = std::array{
    PyGetSetDef{"terms", py_base_get_terms, nullptr, PyDoc_STR("Term base."), nullptr},
    PyGetSetDef{"theory", py_base_get_theory, nullptr, PyDoc_STR("Theory base."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_base_methods = std::array{
    PyMethodDef{
        "get",
        py_c_function_cast(py_base_get),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Get value or default."),
    },
    PyMethodDef{
        "keys",
        py_base_keys,
        METH_NOARGS,
        PyDoc_STR("Iterate keys."),
    },
    PyMethodDef{
        "values",
        py_base_values,
        METH_NOARGS,
        PyDoc_STR("Iterate values."),
    },
    PyMethodDef{
        "items",
        py_base_items,
        METH_NOARGS,
        PyDoc_STR("Iterate items."),
    },
    PyMethodDef{
        "is_external",
        py_c_function_cast(py_base_is_external),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check external."),
    },
    PyMethodDef{
        "is_fact",
        py_c_function_cast(py_base_is_fact),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check fact."),
    },
    PyMethodDef{
        "is_shown",
        py_c_function_cast(py_base_is_shown),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check shown."),
    },
    PyMethodDef{
        "is_projected",
        py_c_function_cast(py_base_is_projected),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check projected."),
    },
    PyMethodDef{
        "is_current",
        py_c_function_cast(py_base_is_current),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check current."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_base_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Base view."))},
    PyType_Slot{Py_tp_methods, py_base_methods.data()},
    PyType_Slot{Py_tp_getset, py_base_getset.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_base_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_base_subscript)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_base_contains)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_base_iter)},
    PyType_Slot{0, nullptr},
};

auto py_base_spec = PyType_Spec{
    "clingo.base.Base",
    sizeof(PyBaseObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING,
    py_base_slots.data(),
};

// TheoryTermType enum

constexpr auto theory_term_members = std::array<IntEnumMember, 6>{
    IntEnumMember{"Number", static_cast<int>(clingo_theory_term_type_number)},
    IntEnumMember{"Symbol", static_cast<int>(clingo_theory_term_type_symbol)},
    IntEnumMember{"Tuple", static_cast<int>(clingo_theory_term_type_tuple)},
    IntEnumMember{"List", static_cast<int>(clingo_theory_term_type_list)},
    IntEnumMember{"Set", static_cast<int>(clingo_theory_term_type_set)},
    IntEnumMember{"Function", static_cast<int>(clingo_theory_term_type_function)},
};

PyModuleDef base_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.base",
    PyDoc_STR("Functions and classes to inspect atom, term, and theory bases."),
    static_cast<Py_ssize_t>(sizeof(BaseModuleState)),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

} // namespace

void register_base(py::module &m) {
    auto base_mod = Guard<PyObject *, Py_DECREF>(PyModule_Create(&base_def));
    if (base_mod.get() == nullptr) {
        throw py::error_already_set();
    }

    auto *state = get_state(base_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    auto enum_obj =
        Guard<PyObject *, Py_DECREF>(create_int_enum("clingo.base", "TheoryTermType", theory_term_members, {}));
    if (enum_obj.get() == nullptr || PyModule_AddObjectRef(base_mod.get(), "TheoryTermType", enum_obj.get()) < 0) {
        throw py::error_already_set();
    }
    state->theory_term_type_enum = enum_obj.release();

    if (!add_type(base_mod.get(), "Atom", &py_atom_spec, &state->atom_type) ||
        !add_type(base_mod.get(), "_AtomBaseIter", &py_atom_base_iter_spec, &state->atom_base_iter_type) ||
        !add_type(base_mod.get(), "AtomBase", &py_atom_base_spec, &state->atom_base_type) ||
        !add_type(base_mod.get(), "Term", &py_term_spec, &state->term_type) ||
        !add_type(base_mod.get(), "_TermBaseIter", &py_term_base_iter_spec, &state->term_base_iter_type) ||
        !add_type(base_mod.get(), "TermBase", &py_term_base_spec, &state->term_base_type) ||
        !add_type(base_mod.get(), "TheoryTerm", &py_theory_term_spec, &state->theory_term_type) ||
        !add_type(base_mod.get(), "TheoryElement", &py_theory_element_spec, &state->theory_element_type) ||
        !add_type(base_mod.get(), "TheoryAtom", &py_theory_atom_spec, &state->theory_atom_type) ||
        !add_type(base_mod.get(), "TheoryBase", &py_theory_base_spec, &state->theory_base_type) ||
        !add_type(base_mod.get(), "_BaseIter", &py_base_iter_spec, &state->base_iter_type) ||
        !add_type(base_mod.get(), "Base", &py_base_spec, &state->base_type)) {
        throw py::error_already_set();
    }

    auto *modules = PyImport_GetModuleDict();
    if (modules == nullptr || PyModule_AddObjectRef(m.ptr(), "base", base_mod.get()) < 0 ||
        PyDict_SetItemString(modules, "clingo.base", base_mod.get()) < 0) {
        throw py::error_already_set();
    }
}

auto py_base_new(PyTypeObject *type, const clingo_base_t *base) -> PyObject * {
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *b = reinterpret_cast<PyBaseObject *>(obj);
    b->base = base;
    return obj;
}

auto py_base_type() -> PyTypeObject * {
    auto mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("clingo.base"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr || state->base_type == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<PyTypeObject *>(state->base_type);
}

auto convert(TypeHint<"clingo.base.Base"> base, MixedLitSpan const &lits, bool flip, bool disjunctive) -> LitVec {
    auto *base_type = py_base_type();
    if (base_type == nullptr) {
        throw py::error_already_set();
    }
    if (PyObject_TypeCheck(base.ptr(), base_type) == 0) {
        throw py::type_error{"base must be an instance of clingo.base.Base"};
    }
    auto *base_obj = reinterpret_cast<PyBaseObject *>(base.ptr());
    auto const *c_base = base_obj->base;

    auto res = LitVec{};
    res.reserve(lits.size());
    for (auto &&x : lits) {
        std::visit(
            [&]<typename T>(T const &x) {
                if constexpr (std::is_same_v<T, Lit_t>) {
                    res.emplace_back(flip ? -x : x);
                } else {
                    auto const &[sym, positive] = x;
                    auto pos = positive != flip;

                    auto const [name, arity, sign] = sym.signature().value();
                    auto csig = clingo_signature_t{name.data(), name.size(), arity, sign};

                    clingo_atom_base_t const *dom = nullptr;
                    bool found = false;
                    if (!clingo_base_atoms_find(c_base, &csig, &dom, &found)) {
                        py_handle_error();
                        throw py::error_already_set();
                    }

                    if (found && dom != nullptr) {
                        size_t index = 0;
                        if (!clingo_atom_base_find(dom, *c_cast(&sym), &index)) {
                            py_handle_error();
                            throw py::error_already_set();
                        }

                        size_t size = 0;
                        if (!clingo_atom_base_size(dom, &size)) {
                            py_handle_error();
                            throw py::error_already_set();
                        }

                        if (index < size) {
                            clingo_literal_t lit = 0;
                            if (!clingo_atom_base_literal(dom, index, &lit)) {
                                py_handle_error();
                                throw py::error_already_set();
                            }
                            res.emplace_back(pos ? lit : -lit);
                            return;
                        }
                    }

                    if (pos != disjunctive) {
                        res.emplace_back(1);
                        res.emplace_back(-1);
                    }
                }
            },
            x);
    }
    return res;
}

} // namespace PyClingo
