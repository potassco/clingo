#include "collection.hh"

#include "core.hh"
#include "util.hh"

#include <algorithm>
#include <array>

namespace PyClingo {

namespace {

// mapping views

struct PyViewObject {
    PyObject_HEAD
    //! The mapping (nullptr after the object has been cleared).
    PyObject *mapping;
    //! The function to iterate the mapping.
    ViewIter iter;
};

auto view_cast(PyObject *self) -> PyViewObject * {
    return reinterpret_cast<PyViewObject *>(self);
}

auto view_mapping(PyObject *self) -> PyObject * {
    auto *mapping = view_cast(self)->mapping;
    if (mapping == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "the view is no longer valid");
    }
    return mapping;
}

auto view_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(view_cast(self)->mapping);
    return 0;
}

auto view_clear(PyObject *self) -> int {
    Py_CLEAR(view_cast(self)->mapping);
    return 0;
}

auto view_len(PyObject *self) -> Py_ssize_t {
    auto *mapping = view_mapping(self);
    return mapping != nullptr ? PyObject_Size(mapping) : -1;
}

auto view_iter(PyObject *self) -> PyObject * {
    auto *mapping = view_mapping(self);
    return mapping != nullptr ? view_cast(self)->iter(mapping) : nullptr;
}

auto view_repr(PyObject *self) -> PyObject * {
    auto *mapping = view_mapping(self);
    if (mapping == nullptr) {
        return nullptr;
    }
    auto name = steal(PyType_GetName(Py_TYPE(self)));
    return name.get() != nullptr ? PyUnicode_FromFormat("%U(%R)", name.get(), mapping) : nullptr;
}

auto keys_contains(PyObject *self, PyObject *key) -> int {
    auto *mapping = view_mapping(self);
    return mapping != nullptr ? PySequence_Contains(mapping, key) : -1;
}

auto items_contains(PyObject *self, PyObject *item) -> int {
    auto *mapping = view_mapping(self);
    if (mapping == nullptr) {
        return -1;
    }
    if (PyTuple_Check(item) == 0 || PyTuple_Size(item) != 2) {
        return 0;
    }
    auto value = steal(PyObject_GetItem(mapping, PyTuple_GetItem(item, 0)));
    if (value.get() == nullptr) {
        if (PyErr_ExceptionMatches(PyExc_KeyError) != 0) {
            PyErr_Clear();
            return 0;
        }
        return -1;
    }
    return PyObject_RichCompareBool(value.get(), PyTuple_GetItem(item, 1), Py_EQ);
}

auto values_contains(PyObject *self, PyObject *value) -> int {
    auto it = steal(view_iter(self));
    if (it.get() == nullptr) {
        return -1;
    }
    for (auto item = steal(PyIter_Next(it.get())); item.get() != nullptr; item = steal(PyIter_Next(it.get()))) {
        if (auto res = PyObject_RichCompareBool(item.get(), value, Py_EQ); res != 0) {
            return res;
        }
    }
    return PyErr_Occurred() != nullptr ? -1 : 0;
}

// set operations of keys and items views
//
// The operations follow the mixin methods of collections.abc.Set: the operand
// that is not a view can be any iterable and the result is a set.

auto view_and(PyObject *a, PyObject *b) -> PyObject *;

//! Check whether the object is a keys or items view.
auto is_set_view(PyObject *obj) -> bool {
    return PyType_GetSlot(Py_TYPE(obj), Py_nb_and) == reinterpret_cast<void *>(view_and);
}

//! Return NotImplemented if getting an iterator failed with a type error.
auto not_iterable() -> PyObject * {
    if (PyErr_ExceptionMatches(PyExc_TypeError) != 0) {
        PyErr_Clear();
        Py_RETURN_NOTIMPLEMENTED;
    }
    return nullptr;
}

//! Add the elements of the iterable to the set that satisfy the predicate.
//!
//! The predicate returns 1 to add an element, 0 to skip it, and -1 on error.
template <class F> auto add_if(PyObject *set, PyObject *it, F const &pred) -> bool {
    for (auto item = steal(PyIter_Next(it)); item.get() != nullptr; item = steal(PyIter_Next(it))) {
        auto res = pred(item.get());
        if (res < 0 || (res == 1 && PySet_Add(set, item.get()) < 0)) {
            return false;
        }
    }
    return PyErr_Occurred() == nullptr;
}

auto view_and(PyObject *a, PyObject *b) -> PyObject * {
    // NOTE: the other operand is iterated so that intersecting a large view
    // with a small iterable is cheap
    auto *view = is_set_view(a) ? a : b;
    auto it = steal(PyObject_GetIter(view == a ? b : a));
    if (it.get() == nullptr) {
        return not_iterable();
    }
    auto res = steal(PySet_New(nullptr));
    if (res.get() == nullptr ||
        !add_if(res.get(), it.get(), [view](PyObject *item) { return PySequence_Contains(view, item); })) {
        return nullptr;
    }
    return res.release();
}

auto view_or(PyObject *a, PyObject *b) -> PyObject * {
    auto res = steal(PySet_New(nullptr));
    if (res.get() == nullptr) {
        return nullptr;
    }
    for (auto *obj : {a, b}) {
        auto it = steal(PyObject_GetIter(obj));
        if (it.get() == nullptr) {
            return not_iterable();
        }
        if (!add_if(res.get(), it.get(), [](PyObject *) { return 1; })) {
            return nullptr;
        }
    }
    return res.release();
}

auto view_sub(PyObject *a, PyObject *b) -> PyObject * {
    // NOTE: the right operand is converted to a set if it is not a set
    // already so that membership tests are cheap
    auto rhs = PyGuard{nullptr};
    if (is_set_view(b) || PyAnySet_Check(b)) {
        rhs = borrow(b);
    } else {
        auto it = steal(PyObject_GetIter(b));
        if (it.get() == nullptr) {
            return not_iterable();
        }
        rhs = steal(PySet_New(it.get()));
        if (rhs.get() == nullptr) {
            return nullptr;
        }
    }
    auto it = steal(PyObject_GetIter(a));
    if (it.get() == nullptr) {
        return not_iterable();
    }
    auto res = steal(PySet_New(nullptr));
    if (res.get() == nullptr || !add_if(res.get(), it.get(), [&rhs](PyObject *item) {
            auto contained = PySequence_Contains(rhs.get(), item);
            return contained < 0 ? -1 : 1 - contained;
        })) {
        return nullptr;
    }
    return res.release();
}

auto view_xor(PyObject *a, PyObject *b) -> PyObject * {
    auto lhs = steal(PySet_New(nullptr));
    auto rhs = steal(PySet_New(nullptr));
    if (lhs.get() == nullptr || rhs.get() == nullptr) {
        return nullptr;
    }
    for (auto [obj, set] : {std::pair{a, lhs.get()}, std::pair{b, rhs.get()}}) {
        auto it = steal(PyObject_GetIter(obj));
        if (it.get() == nullptr) {
            return not_iterable();
        }
        if (!add_if(set, it.get(), [](PyObject *) { return 1; })) {
            return nullptr;
        }
    }
    return PyNumber_Xor(lhs.get(), rhs.get());
}

auto view_richcompare(PyObject *self, PyObject *other, int op) -> PyObject * {
    auto *set_abc = py_set_abc(Py_TYPE(self));
    if (set_abc == nullptr) {
        return nullptr;
    }
    auto is_set = PyObject_IsInstance(other, set_abc);
    if (is_set < 0) {
        return nullptr;
    }
    if (is_set == 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto lhs = steal(PyFrozenSet_New(self));
    auto rhs = PyAnySet_Check(other) ? borrow(other) : steal(PyFrozenSet_New(other));
    if (lhs.get() == nullptr || rhs.get() == nullptr) {
        return nullptr;
    }
    return PyObject_RichCompare(lhs.get(), rhs.get(), op);
}

auto view_isdisjoint(PyObject *self, PyObject *other) -> PyObject * {
    auto it = steal(PyObject_GetIter(other));
    if (it.get() == nullptr) {
        return nullptr;
    }
    for (auto item = steal(PyIter_Next(it.get())); item.get() != nullptr; item = steal(PyIter_Next(it.get()))) {
        auto res = PySequence_Contains(self, item.get());
        if (res < 0) {
            return nullptr;
        }
        if (res == 1) {
            Py_RETURN_FALSE;
        }
    }
    if (PyErr_Occurred() != nullptr) {
        return nullptr;
    }
    Py_RETURN_TRUE;
}

auto set_view_methods = std::array{
    PyMethodDef{
        "isdisjoint",
        view_isdisjoint,
        METH_O,
        PyDoc_STR("Check whether the view has no elements in common with the iterable."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto keys_view_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A view on the keys of a mapping.")}, // NOLINT
    PyType_Slot{Py_tp_repr, reinterpret_cast<void *>(view_repr)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(view_iter)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(view_richcompare)},
    PyType_Slot{Py_tp_methods, set_view_methods.data()},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(view_len)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(keys_contains)},
    PyType_Slot{Py_nb_and, reinterpret_cast<void *>(view_and)},
    PyType_Slot{Py_nb_or, reinterpret_cast<void *>(view_or)},
    PyType_Slot{Py_nb_subtract, reinterpret_cast<void *>(view_sub)},
    PyType_Slot{Py_nb_xor, reinterpret_cast<void *>(view_xor)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(view_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(view_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{0, nullptr},
};

auto items_view_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A view on the items of a mapping.")}, // NOLINT
    PyType_Slot{Py_tp_repr, reinterpret_cast<void *>(view_repr)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(view_iter)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(view_richcompare)},
    PyType_Slot{Py_tp_methods, set_view_methods.data()},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(view_len)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(items_contains)},
    PyType_Slot{Py_nb_and, reinterpret_cast<void *>(view_and)},
    PyType_Slot{Py_nb_or, reinterpret_cast<void *>(view_or)},
    PyType_Slot{Py_nb_subtract, reinterpret_cast<void *>(view_sub)},
    PyType_Slot{Py_nb_xor, reinterpret_cast<void *>(view_xor)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(view_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(view_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{0, nullptr},
};

auto values_view_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A view on the values of a mapping.")}, // NOLINT
    PyType_Slot{Py_tp_repr, reinterpret_cast<void *>(view_repr)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(view_iter)},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(view_len)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(values_contains)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(view_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(view_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{0, nullptr},
};

constexpr auto view_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC;

auto keys_view_spec = PyType_Spec{
    "clingo.core._KeysView", sizeof(PyViewObject), 0, view_flags, keys_view_slots.data(),
};

auto values_view_spec = PyType_Spec{
    "clingo.core._ValuesView", sizeof(PyViewObject), 0, view_flags, values_view_slots.data(),
};

auto items_view_spec = PyType_Spec{
    "clingo.core._ItemsView", sizeof(PyViewObject), 0, view_flags, items_view_slots.data(),
};

//! Convert an optional bound of the index method like collections.abc.Sequence.index.
auto index_bound(PyObject *obj, Py_ssize_t size, Py_ssize_t *bound) -> bool {
    if (obj == Py_None) {
        return true;
    }
    auto val = PyNumber_AsSsize_t(obj, nullptr);
    if (val == -1 && PyErr_Occurred() != nullptr) {
        return false;
    }
    *bound = std::clamp<Py_ssize_t>(val < 0 ? val + size : val, 0, size);
    return true;
}

} // namespace

auto add_view_types(PyObject *module, PyObject **keys, PyObject **values, PyObject **items) -> bool {
    return add_type(module, "_KeysView", &keys_view_spec, keys) &&
           add_type(module, "_ValuesView", &values_view_spec, values) &&
           add_type(module, "_ItemsView", &items_view_spec, items) && register_abc(*keys, "KeysView") &&
           register_abc(*values, "ValuesView") && register_abc(*items, "ItemsView");
}

auto py_view_new(PyObject *type, PyObject *mapping, ViewIter iter) -> PyObject * {
    auto *obj = PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(type), 0);
    if (obj == nullptr) {
        return nullptr;
    }
    Py_INCREF(mapping);
    view_cast(obj)->mapping = mapping;
    view_cast(obj)->iter = iter;
    return obj;
}

auto py_mapping_richcompare(PyObject *view_type, PyObject *self, PyObject *other, int op, ViewIter items)
    -> PyObject * {
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *mapping_abc = py_mapping_abc(reinterpret_cast<PyTypeObject *>(view_type));
    if (mapping_abc == nullptr) {
        return nullptr;
    }
    auto is_mapping = PyObject_IsInstance(other, mapping_abc);
    if (is_mapping < 0) {
        return nullptr;
    }
    if (is_mapping == 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    // NOTE: like collections.abc.Mapping, the mappings are compared as dictionaries
    auto lhs = steal(PyDict_New());
    auto lhs_items = steal(items(self));
    if (lhs.get() == nullptr || lhs_items.get() == nullptr || PyDict_MergeFromSeq2(lhs.get(), lhs_items.get(), 1) < 0) {
        return nullptr;
    }
    auto rhs = steal(PyDict_New());
    auto rhs_items = steal(PyObject_CallMethod(other, "items", nullptr));
    if (rhs.get() == nullptr || rhs_items.get() == nullptr || PyDict_MergeFromSeq2(rhs.get(), rhs_items.get(), 1) < 0) {
        return nullptr;
    }
    return PyObject_RichCompare(lhs.get(), rhs.get(), op);
}

auto py_sequence_subscript(PyObject *self, PyObject *key) -> PyObject * {
    if (PyIndex_Check(key) != 0) {
        auto index = PyNumber_AsSsize_t(key, PyExc_IndexError);
        if (index == -1 && PyErr_Occurred() != nullptr) {
            return nullptr;
        }
        // NOTE: negative indices are adjusted using the length of the sequence
        return PySequence_GetItem(self, index);
    }
    if (PySlice_Check(key) != 0) {
        Py_ssize_t start = 0;
        Py_ssize_t stop = 0;
        Py_ssize_t step = 0;
        if (PySlice_Unpack(key, &start, &stop, &step) < 0) {
            return nullptr;
        }
        auto size = PyObject_Size(self);
        if (size < 0) {
            return nullptr;
        }
        auto n = PySlice_AdjustIndices(size, &start, &stop, step);
        auto res = steal(PyList_New(n));
        if (res.get() == nullptr) {
            return nullptr;
        }
        for (Py_ssize_t i = 0; i < n; ++i) {
            auto *item = PySequence_GetItem(self, start + (i * step));
            if (item == nullptr) {
                return nullptr;
            }
            PyList_SetItem(res.get(), i, item);
        }
        return res.release();
    }
    auto name = steal(PyType_GetName(Py_TYPE(self)));
    if (name.get() != nullptr) {
        PyErr_Format(PyExc_TypeError, "%U indices must be integers or slices", name.get());
    }
    return nullptr;
}

auto py_sequence_index(PyObject *self, PyObject *args) -> PyObject * {
    PyObject *value = nullptr;
    PyObject *start_obj = Py_None;
    PyObject *stop_obj = Py_None;
    if (PyArg_ParseTuple(args, "O|OO:index", &value, &start_obj, &stop_obj) == 0) {
        return nullptr;
    }
    auto size = PyObject_Size(self);
    if (size < 0) {
        return nullptr;
    }
    Py_ssize_t start = 0;
    Py_ssize_t stop = size;
    if (!index_bound(start_obj, size, &start) || !index_bound(stop_obj, size, &stop)) {
        return nullptr;
    }
    for (auto i = start; i < stop; ++i) {
        auto item = steal(PySequence_GetItem(self, i));
        if (item.get() == nullptr) {
            return nullptr;
        }
        auto res = PyObject_RichCompareBool(item.get(), value, Py_EQ);
        if (res < 0) {
            return nullptr;
        }
        if (res == 1) {
            return PyLong_FromSsize_t(i);
        }
    }
    PyErr_SetString(PyExc_ValueError, "value is not in the sequence");
    return nullptr;
}

auto py_sequence_count(PyObject *self, PyObject *value) -> PyObject * {
    auto it = steal(PyObject_GetIter(self));
    if (it.get() == nullptr) {
        return nullptr;
    }
    Py_ssize_t count = 0;
    for (auto item = steal(PyIter_Next(it.get())); item.get() != nullptr; item = steal(PyIter_Next(it.get()))) {
        auto res = PyObject_RichCompareBool(item.get(), value, Py_EQ);
        if (res < 0) {
            return nullptr;
        }
        count += res;
    }
    if (PyErr_Occurred() != nullptr) {
        return nullptr;
    }
    return PyLong_FromSsize_t(count);
}

} // namespace PyClingo
