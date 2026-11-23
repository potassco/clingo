#pragma once

#include <Python.h>

#include <cstdint>

namespace PyClingo {

// Mapping views
//
// The views implement the protocols of collections.abc.KeysView, ValuesView,
// and ItemsView. They hold a reference to their mapping and iterate it with
// the iterator function of the mapping. The view types are owned by the
// clingo.core module.

//! A function returning a new iterator over the keys, values, or items of a mapping.
using ViewIter = PyObject *(*)(PyObject *mapping);

//! Add the keys, values, and items view types to the module and register them with collections.abc.
auto add_view_types(PyObject *module, PyObject **keys, PyObject **values, PyObject **items) -> bool;

//! Create a view of the given type over the mapping (new reference).
auto py_view_new(PyObject *type, PyObject *mapping, ViewIter iter) -> PyObject *;

//! Compare a mapping with another object like collections.abc.Mapping.
//!
//! The items of the mapping are iterated with the given function. The view type
//! gives access to the state of the clingo.core module.
auto py_mapping_richcompare(PyObject *view_type, PyObject *self, PyObject *other, int op, ViewIter items) -> PyObject *;

//! The kinds of mapping views.
enum class ViewKind : uint8_t { keys, values, items };

//! A function returning the view type of the given kind for a mapping (borrowed reference).
//!
//! The view types are typically stored in the state of the module of the mapping.
using ViewTypeGetter = PyObject *(*)(PyObject *mapping, ViewKind kind);

//! The keys, values, and items methods and the comparison of a mapping.
//!
//! They are implemented in terms of the iterator functions of the mapping.
template <ViewTypeGetter Type, ViewIter Keys, ViewIter Values, ViewIter Items> struct MappingProtocol {
    //! Get a view on the keys (keys method, METH_NOARGS).
    static auto keys(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
        return view(self, ViewKind::keys, Keys);
    }
    //! Get a view on the values (values method, METH_NOARGS).
    static auto values(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
        return view(self, ViewKind::values, Values);
    }
    //! Get a view on the items (items method, METH_NOARGS).
    static auto items(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
        return view(self, ViewKind::items, Items);
    }
    //! Compare the mapping with another object (Py_tp_richcompare slot).
    static auto richcompare(PyObject *self, PyObject *other, int op) -> PyObject * {
        auto *type = Type(self, ViewKind::keys);
        return type != nullptr ? py_mapping_richcompare(type, self, other, op, Items) : nullptr;
    }

  private:
    static auto view(PyObject *self, ViewKind kind, ViewIter iter) -> PyObject * {
        auto *type = Type(self, kind);
        return type != nullptr ? py_view_new(type, self, iter) : nullptr;
    }
};

// Sequences
//
// The functions implement the parts of collections.abc.Sequence not covered
// by the sequence slots of a type via its length and item functions.

//! Get an item or a list of items for a slice (Py_mp_subscript slot).
auto py_sequence_subscript(PyObject *self, PyObject *key) -> PyObject *;

//! Get the first index of a value (index method, METH_FASTCALL).
auto py_sequence_index(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> PyObject *;

//! Count the occurrences of a value (count method, METH_O).
auto py_sequence_count(PyObject *self, PyObject *value) -> PyObject *;

} // namespace PyClingo
