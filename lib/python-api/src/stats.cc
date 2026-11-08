#include "stats.hh"

#include "core.hh"

#include <array>
#include <cassert>
#include <limits>
#include <string_view>

namespace PyClingo {

namespace {

constexpr auto stats_type_members = std::array<IntEnumMember, 3>{
    IntEnumMember{"Value", static_cast<int>(StatsType::value)},
    IntEnumMember{"Array", static_cast<int>(StatsType::array)},
    IntEnumMember{"Map", static_cast<int>(StatsType::map)},
};

using PyStatsObject = struct {
    PyObject_HEAD clingo_stats_t *stats;
    uint64_t key;
};

enum class IterKind : uint8_t {
    map_keys,
    map_values_view,
    map_values,
    map_items_view,
    map_items,
    array_items_view,
    array_items,
};

using PyStatsIterObject = struct {
    PyObject_HEAD clingo_stats_t *stats;
    uint64_t key;
    size_t index;
    size_t size;
    IterKind kind;
};

struct StatsModuleState {
    PyObject *stats_type_enum{nullptr};
    std::array<PyObject *, 3> stats_type_vals{};
    PyObject *stats_view_type{nullptr};
    PyObject *stats_type{nullptr};
    PyObject *stats_array_view_type{nullptr};
    PyObject *stats_array_type{nullptr};
    PyObject *stats_map_view_type{nullptr};
    PyObject *stats_map_type{nullptr};
    PyObject *stats_iter_type{nullptr};
};

auto get_state(PyObject *module) -> StatsModuleState * {
    auto *state = static_cast<StatsModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "stats module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_self(PyObject *self) -> StatsModuleState * {
    auto *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve stats module");
        return nullptr;
    }
    return get_state(mod);
}

auto py_stats_cast(PyObject *obj) -> PyStatsObject * {
    return reinterpret_cast<PyStatsObject *>(obj);
}

auto py_stats_alloc(PyObject *type, clingo_stats_t *stats, uint64_t key) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    auto *obj = to->tp_alloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *s = reinterpret_cast<PyStatsObject *>(obj);
    s->stats = stats;
    s->key = key;
    return obj;
}

auto stats_type_of(clingo_stats_t *stats, uint64_t key, clingo_stats_type_t *type) -> bool {
    if (!clingo_stats_type(stats, key, type)) {
        py_handle_error();
        return false;
    }
    return true;
}

auto stats_map_len(clingo_stats_t *stats, uint64_t key, size_t *size) -> bool {
    if (!clingo_stats_map_size(stats, key, size)) {
        py_handle_error();
        return false;
    }
    return true;
}

auto stats_array_len(clingo_stats_t *stats, uint64_t key, size_t *size) -> bool {
    if (!clingo_stats_array_size(stats, key, size)) {
        py_handle_error();
        return false;
    }
    return true;
}

auto stats_key_to_size(PyObject *key, size_t *out) -> int {
    if (PyIndex_Check(key) == 0) {
        PyErr_SetString(PyExc_TypeError, "index must be an integer");
        return -1;
    }
    auto idx_obj = Guard<PyObject *, Py_DECREF>(PyNumber_Index(key));
    if (idx_obj.get() == nullptr) {
        return -1;
    }
    auto idx = PyLong_AsSsize_t(idx_obj.get());
    if (idx == -1 && PyErr_Occurred() != nullptr) {
        return -1;
    }
    if (idx < 0) {
        PyErr_SetString(PyExc_IndexError, "array index out of bounds");
        return -1;
    }
    *out = static_cast<size_t>(idx);
    return 0;
}

auto stats_key_to_str(PyObject *key, char const **data, Py_ssize_t *size) -> int {
    if (PyUnicode_Check(key) == 0) {
        PyErr_SetString(PyExc_TypeError, "key must be a string");
        return -1;
    }
    auto const *s = PyUnicode_AsUTF8AndSize(key, size);
    if (s == nullptr) {
        return -1;
    }
    *data = s;
    return 0;
}

auto stats_get_subkey_by_name(clingo_stats_t *stats, uint64_t key, std::string_view name, uint64_t *subkey, bool *found)
    -> bool {
    if (!clingo_stats_map_has_subkey(stats, key, name.data(), name.size(), subkey, found)) {
        py_handle_error();
        return false;
    }
    return true;
}

auto stats_get_key_name(clingo_stats_t *stats, uint64_t key, size_t i, clingo_string_t *name) -> bool {
    if (!clingo_stats_map_subkey_name(stats, key, i, name)) {
        py_handle_error();
        return false;
    }
    return true;
}

auto stats_nestify(clingo_stats_t *stats, uint64_t key) -> PyObject *;

auto stats_resolve_callable_arg(clingo_stats_t *stats, uint64_t key, bool has_old) -> PyObject * {
    if (!has_old) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    return stats_nestify(stats, key);
}

auto stats_detect_type(PyObject *value, clingo_stats_type_t *type) -> int {
    if (PyUnicode_Check(value) == 0 && PyBytes_Check(value) == 0 && PyByteArray_Check(value) == 0) {
        if (PySequence_Check(value) != 0) {
            // NOTE: Custom types implementing __getitem__ are classified as
            // sequences and mappings. We heuristically skip sequences that
            // implement items(). More complex tests could use collections.abc
            // but that would be overkill here.
            auto res = PyObject_HasAttrString(value, "items");
            if (res < 0) {
                return -1;
            }
            if (res == 0) {
                *type = clingo_stats_type_array;
                return 0;
            }
        }
        if (PyMapping_Check(value) != 0) {
            *type = clingo_stats_type_map;
            return 0;
        }
        if (PyFloat_Check(value) != 0 || PyLong_Check(value) != 0 || PyObject_HasAttrString(value, "__float__") != 0) {
            *type = clingo_stats_type_value;
            return 0;
        }
    }
    PyErr_SetString(PyExc_TypeError, "expected sequence, mapping, or float");
    return -1;
}

auto stats_prepare_value(clingo_stats_t *stats, uint64_t key, bool has_old, PyObject *value, clingo_stats_type_t *type,
                         PyObject **prepared) -> int {
    auto resolved = Guard<PyObject *, Py_DECREF>(nullptr);
    if (PyCallable_Check(value) == 1) {
        auto arg = Guard<PyObject *, Py_DECREF>(stats_resolve_callable_arg(stats, key, has_old));
        if (arg.get() == nullptr) {
            return -1;
        }
        resolved.get() = PyObject_CallFunctionObjArgs(value, arg.get(), nullptr);
        if (resolved.get() == nullptr) {
            return -1;
        }
    } else {
        resolved.get() = value;
        Py_INCREF(resolved);
    }

    if (stats_detect_type(resolved, type) < 0) {
        return -1;
    }

    *prepared = resolved.release();
    return 0;
}

auto stats_update(clingo_stats_t *stats, uint64_t key, PyObject *value, bool init) -> int {
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(stats, key, &ty)) {
        return -1;
    }

    switch (static_cast<clingo_stats_type_e>(ty)) {
        case clingo_stats_type_value: {
            auto src = Guard<PyObject *, Py_DECREF>{nullptr};
            if (PyCallable_Check(value) == 1) {
                auto hold = Guard<PyObject *, Py_DECREF>(nullptr);
                if (init) {
                    Py_INCREF(Py_None);
                    hold.get() = Py_None;
                } else {
                    double old = 0.0;
                    if (!clingo_stats_value_get(stats, key, &old)) {
                        py_handle_error();
                        return -1;
                    }
                    hold.get() = PyFloat_FromDouble(old);
                    if (hold.get() == nullptr) {
                        return -1;
                    }
                }
                src.get() = PyObject_CallFunctionObjArgs(value, hold.get(), nullptr);
                if (src.get() == nullptr) {
                    return -1;
                }
                if (src.get() == Py_None) {
                    return 0;
                }
            } else {
                src.get() = value;
                Py_INCREF(src.get());
            }

            auto x = PyFloat_AsDouble(src);
            if (PyErr_Occurred() != nullptr) {
                return -1;
            }
            if (!clingo_stats_value_set(stats, key, x)) {
                py_handle_error();
                return -1;
            }
            return 0;
        }

        case clingo_stats_type_array: {
            auto it = Guard<PyObject *, Py_DECREF>(PyObject_GetIter(value));
            if (it.get() == nullptr) {
                return -1;
            }

            size_t i = 0;
            size_t n = 0;
            if (!stats_array_len(stats, key, &n)) {
                return -1;
            }

            while (true) {
                auto elem = Guard<PyObject *, Py_DECREF>(PyIter_Next(it.get()));
                if (elem.get() == nullptr) {
                    if (PyErr_Occurred() != nullptr) {
                        return -1;
                    }
                    break;
                }

                if (i < n) {
                    uint64_t subkey = 0;
                    if (!clingo_stats_array_at(stats, key, i, &subkey)) {
                        py_handle_error();
                        return -1;
                    }
                    if (stats_update(stats, subkey, elem.get(), false) < 0) {
                        return -1;
                    }
                    ++i;
                } else {
                    clingo_stats_type_t subtype = 0;
                    auto prepared = Guard<PyObject *, Py_DECREF>(nullptr);
                    if (stats_prepare_value(stats, 0, false, elem.get(), &subtype, &prepared.get()) < 0) {
                        return -1;
                    }

                    uint64_t subkey = 0;
                    if (!clingo_stats_array_push(stats, key, subtype, &subkey)) {
                        py_handle_error();
                        return -1;
                    }
                    if (stats_update(stats, subkey, prepared.get(), true) < 0) {
                        return -1;
                    }
                }
            }
            return 0;
        }

        case clingo_stats_type_map: {
            auto items = Guard<PyObject *, Py_DECREF>(PyObject_CallMethod(value, "items", nullptr));
            if (items.get() == nullptr) {
                return -1;
            }
            auto it = Guard<PyObject *, Py_DECREF>(PyObject_GetIter(items.get()));
            if (it.get() == nullptr) {
                return -1;
            }

            while (true) {
                auto elem = Guard<PyObject *, Py_DECREF>(PyIter_Next(it.get()));
                if (elem.get() == nullptr) {
                    if (PyErr_Occurred() != nullptr) {
                        return -1;
                    }
                    break;
                }
                if (PySequence_Check(elem.get()) == 0) {
                    PyErr_SetString(PyExc_TypeError, "items() must yield key/value pairs");
                    return -1;
                }

                auto kobj = Guard<PyObject *, Py_DECREF>(PySequence_GetItem(elem.get(), 0));
                if (kobj.get() == nullptr) {
                    return -1;
                }

                auto vobj = Guard<PyObject *, Py_DECREF>(PySequence_GetItem(elem.get(), 1));
                if (vobj.get() == nullptr) {
                    return -1;
                }

                char const *k = nullptr;
                Py_ssize_t kn = 0;
                if (stats_key_to_str(kobj.get(), &k, &kn) < 0) {
                    return -1;
                }

                uint64_t old_subkey = 0;
                bool found = false;
                if (!stats_get_subkey_by_name(stats, key, std::string_view{k, static_cast<size_t>(kn)}, &old_subkey,
                                              &found)) {
                    return -1;
                }

                clingo_stats_type_t subtype = 0;
                auto prepared = Guard<PyObject *, Py_DECREF>(nullptr);
                if (stats_prepare_value(stats, old_subkey, found, vobj.get(), &subtype, &prepared.get()) < 0) {
                    return -1;
                }

                uint64_t subkey = 0;
                if (!clingo_stats_map_add_subkey(stats, key, k, static_cast<size_t>(kn), subtype, &subkey)) {
                    py_handle_error();
                    return -1;
                }

                if (stats_update(stats, subkey, prepared.get(), !found) < 0) {
                    return -1;
                }
            }

            return 0;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "invalid stats type");
    return -1;
}

auto stats_nestify(clingo_stats_t *stats, uint64_t key) -> PyObject * {
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(stats, key, &ty)) {
        return nullptr;
    }

    switch (static_cast<clingo_stats_type_e>(ty)) {
        case clingo_stats_type_value: {
            double v = 0.0;
            if (!clingo_stats_value_get(stats, key, &v)) {
                return py_handle_error();
            }
            return PyFloat_FromDouble(v);
        }

        case clingo_stats_type_array: {
            size_t n = 0;
            if (!stats_array_len(stats, key, &n)) {
                return nullptr;
            }
            auto out = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(n)));
            if (out.get() == nullptr) {
                return nullptr;
            }
            for (size_t i = 0; i < n; ++i) {
                uint64_t subkey = 0;
                if (!clingo_stats_array_at(stats, key, i, &subkey)) {
                    py_handle_error();
                    return nullptr;
                }
                auto *v = stats_nestify(stats, subkey);
                if (v == nullptr) {
                    return nullptr;
                }
                PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), v);
            }
            return out.release();
        }

        case clingo_stats_type_map: {
            size_t n = 0;
            if (!stats_map_len(stats, key, &n)) {
                return nullptr;
            }
            auto out = Guard<PyObject *, Py_DECREF>(PyDict_New());
            if (out.get() == nullptr) {
                return nullptr;
            }

            for (size_t i = 0; i < n; ++i) {
                clingo_string_t name;
                if (!stats_get_key_name(stats, key, i, &name)) {
                    return nullptr;
                }
                uint64_t subkey = 0;
                bool found = false;
                if (!stats_get_subkey_by_name(stats, key, std::string_view{name.data, name.size}, &subkey, &found)) {
                    return nullptr;
                }
                if (!found) {
                    PyErr_SetString(PyExc_RuntimeError, "inconsistent stats map");
                    return nullptr;
                }

                auto k = Guard<PyObject *, Py_DECREF>(
                    PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size)));
                auto v = Guard<PyObject *, Py_DECREF>(stats_nestify(stats, subkey));
                if (k.get() == nullptr || v.get() == nullptr) {
                    return nullptr;
                }
                if (PyDict_SetItem(out.get(), k.get(), v.get()) < 0) {
                    return nullptr;
                }
            }

            return out.release();
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "invalid stats type");
    return nullptr;
}

auto stats_subscript_impl(PyObject *self, PyObject *key, PyObject *out_type, char const *missing_error) -> PyObject * {
    auto *o = py_stats_cast(self);

    if (PyUnicode_Check(key) != 0) {
        char const *k = nullptr;
        Py_ssize_t kn = 0;
        if (stats_key_to_str(key, &k, &kn) < 0) {
            return nullptr;
        }
        uint64_t subkey = 0;
        bool found = false;
        if (!stats_get_subkey_by_name(o->stats, o->key, std::string_view{k, static_cast<size_t>(kn)}, &subkey,
                                      &found)) {
            return nullptr;
        }
        if (!found) {
            PyErr_SetString(PyExc_IndexError, missing_error);
            return nullptr;
        }
        return py_stats_alloc(out_type, o->stats, subkey);
    }

    size_t i = 0;
    if (stats_key_to_size(key, &i) < 0) {
        return nullptr;
    }

    uint64_t subkey = 0;
    if (!clingo_stats_array_at(o->stats, o->key, i, &subkey)) {
        return py_handle_error();
    }
    return py_stats_alloc(out_type, o->stats, subkey);
}

auto stats_map_subscript_impl(PyObject *self, PyObject *key, PyObject *out_type) -> PyObject * {
    auto *o = py_stats_cast(self);

    char const *k = nullptr;
    Py_ssize_t kn = 0;
    if (stats_key_to_str(key, &k, &kn) < 0) {
        return nullptr;
    }

    uint64_t subkey = 0;
    bool found = false;
    if (!stats_get_subkey_by_name(o->stats, o->key, std::string_view{k, static_cast<size_t>(kn)}, &subkey, &found)) {
        return nullptr;
    }
    if (!found) {
        PyErr_SetString(PyExc_IndexError, "invalid key");
        return nullptr;
    }

    return py_stats_alloc(out_type, o->stats, subkey);
}

auto stats_array_subscript_impl(PyObject *self, PyObject *key, PyObject *out_type) -> PyObject * {
    auto *o = py_stats_cast(self);
    size_t i = 0;
    if (stats_key_to_size(key, &i) < 0) {
        return nullptr;
    }

    size_t n = 0;
    if (!clingo_stats_array_size(o->stats, o->key, &n)) {
        return py_handle_error();
    }

    if (i >= n) {
        PyErr_SetString(PyExc_IndexError, "array index out of bounds");
        return nullptr;
    }

    uint64_t subkey = 0;
    if (!clingo_stats_array_at(o->stats, o->key, i, &subkey)) {
        return py_handle_error();
    }
    return py_stats_alloc(out_type, o->stats, subkey);
}

auto stats_get_array_impl(PyObject *self, PyObject *out_type) -> PyObject * {
    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return nullptr;
    }
    if (ty != clingo_stats_type_array) {
        PyErr_SetString(PyExc_TypeError, "not an array");
        return nullptr;
    }
    return py_stats_alloc(out_type, o->stats, o->key);
}

auto stats_get_map_impl(PyObject *self, PyObject *out_type) -> PyObject * {
    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return nullptr;
    }
    if (ty != clingo_stats_type_map) {
        PyErr_SetString(PyExc_TypeError, "not a map");
        return nullptr;
    }
    return py_stats_alloc(out_type, o->stats, o->key);
}

auto py_stats_get_type_enum(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return nullptr;
    }

    auto idx = static_cast<size_t>(ty);
    if (idx >= state->stats_type_vals.size() || state->stats_type_vals[idx] == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "stats type cache missing");
        return nullptr;
    }

    Py_INCREF(state->stats_type_vals[idx]);
    return state->stats_type_vals[idx];
}

auto py_stats_str(PyObject *self) -> PyObject * {
    auto *o = py_stats_cast(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>();
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_stats_to_string(o->stats, o->key, bld)) {
        return py_handle_error();
    }
    clingo_string_t s;
    if (!clingo_string_builder_string(bld, &s)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(s.data, static_cast<Py_ssize_t>(s.size));
}

auto py_stats_view_len(PyObject *self) -> Py_ssize_t {
    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return -1;
    }

    size_t n = 0;
    if (ty == clingo_stats_type_array) {
        if (!stats_array_len(o->stats, o->key, &n)) {
            return -1;
        }
    } else if (ty == clingo_stats_type_map) {
        if (!stats_map_len(o->stats, o->key, &n)) {
            return -1;
        }
    }

    if (n > static_cast<size_t>(std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto py_stats_view_contains(PyObject *self, PyObject *key) -> int {
    auto *o = py_stats_cast(self);

    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return -1;
    }
    if (ty != clingo_stats_type_map) {
        PyErr_SetString(PyExc_TypeError, "not a map");
        return -1;
    }

    char const *k = nullptr;
    Py_ssize_t kn = 0;
    if (stats_key_to_str(key, &k, &kn) < 0) {
        return -1;
    }

    bool found = false;
    if (!stats_get_subkey_by_name(o->stats, o->key, std::string_view{k, static_cast<size_t>(kn)}, nullptr, &found)) {
        return -1;
    }
    return found ? 1 : 0;
}

auto py_stats_iter_new(PyObject *owner, IterKind kind) -> PyObject * {
    auto *state = get_state_from_self(owner);
    if (state == nullptr || state->stats_iter_type == nullptr) {
        return nullptr;
    }

    auto *type = reinterpret_cast<PyTypeObject *>(state->stats_iter_type);
    auto *obj = type->tp_alloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }

    auto *it = reinterpret_cast<PyStatsIterObject *>(obj);
    auto *o = py_stats_cast(owner);
    it->stats = o->stats;
    it->key = o->key;
    it->index = 0;
    it->kind = kind;

    size_t n = 0;
    switch (kind) {
        case IterKind::map_keys:
        case IterKind::map_values_view:
        case IterKind::map_values:
        case IterKind::map_items_view:
        case IterKind::map_items: {
            if (!stats_map_len(it->stats, it->key, &n)) {
                Py_DECREF(obj);
                return nullptr;
            }
            break;
        }
        case IterKind::array_items_view:
        case IterKind::array_items: {
            if (!stats_array_len(it->stats, it->key, &n)) {
                Py_DECREF(obj);
                return nullptr;
            }
            break;
        }
    }

    it->size = n;
    return obj;
}

auto py_stats_iter_next(PyObject *self) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    auto *it = reinterpret_cast<PyStatsIterObject *>(self);
    if (it->index >= it->size) {
        return nullptr;
    }

    auto i = it->index++;
    switch (it->kind) {
        case IterKind::map_keys:
        case IterKind::map_values_view:
        case IterKind::map_values:
        case IterKind::map_items_view:
        case IterKind::map_items: {
            clingo_string_t name;
            if (!stats_get_key_name(it->stats, it->key, i, &name)) {
                return nullptr;
            }

            uint64_t subkey = 0;
            bool found = false;
            if (!stats_get_subkey_by_name(it->stats, it->key, std::string_view{name.data, name.size}, &subkey,
                                          &found)) {
                return nullptr;
            }
            if (!found) {
                PyErr_SetString(PyExc_RuntimeError, "inconsistent stats map iteration");
                return nullptr;
            }

            if (it->kind == IterKind::map_keys) {
                return PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size));
            }

            auto *val_type = (it->kind == IterKind::map_values_view || it->kind == IterKind::map_items_view)
                                 ? state->stats_view_type
                                 : state->stats_type;
            auto val = Guard<PyObject *, Py_DECREF>(py_stats_alloc(val_type, it->stats, subkey));
            if (val.get() == nullptr) {
                return nullptr;
            }

            if (it->kind == IterKind::map_values_view || it->kind == IterKind::map_values) {
                return val.release();
            }

            auto key = Guard<PyObject *, Py_DECREF>(
                PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size)));
            if (key.get() == nullptr) {
                return nullptr;
            }
            return PyTuple_Pack(2, key.get(), val.get());
        }

        case IterKind::array_items_view:
        case IterKind::array_items: {
            uint64_t subkey = 0;
            if (!clingo_stats_array_at(it->stats, it->key, i, &subkey)) {
                return py_handle_error();
            }

            auto *val_type = it->kind == IterKind::array_items_view ? state->stats_view_type : state->stats_type;
            return py_stats_alloc(val_type, it->stats, subkey);
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "invalid iterator kind");
    return nullptr;
}

auto py_stats_view_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_subscript_impl(self, key, state->stats_view_type, "invalid key");
}

auto py_stats_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_subscript_impl(self, key, state->stats_type, "invalid key");
}

auto py_stats_view_iter(PyObject *self) -> PyObject * {
    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return nullptr;
    }

    if (ty == clingo_stats_type_map) {
        return py_stats_iter_new(self, IterKind::map_keys);
    }
    if (ty == clingo_stats_type_array) {
        return py_stats_iter_new(self, IterKind::array_items_view);
    }
    PyErr_SetString(PyExc_TypeError, "'StatsType::value' is not iterable");
    return nullptr;
}

auto py_stats_iter(PyObject *self) -> PyObject * {
    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return nullptr;
    }

    if (ty == clingo_stats_type_map) {
        return py_stats_iter_new(self, IterKind::map_keys);
    }
    if (ty == clingo_stats_type_array) {
        return py_stats_iter_new(self, IterKind::array_items);
    }
    PyErr_SetString(PyExc_TypeError, "'StatsType::value' is not iterable");
    return nullptr;
}

auto py_stats_view_nestify(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *o = py_stats_cast(self);
    return stats_nestify(o->stats, o->key);
}

auto py_stats_view_get_array(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_get_array_impl(self, state->stats_array_view_type);
}

auto py_stats_get_array(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_get_array_impl(self, state->stats_array_type);
}

auto py_stats_view_get_map(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_get_map_impl(self, state->stats_map_view_type);
}

auto py_stats_get_map(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_get_map_impl(self, state->stats_map_type);
}

auto py_stats_view_get_value(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return nullptr;
    }
    if (ty != clingo_stats_type_value) {
        PyErr_SetString(PyExc_TypeError, "not a value");
        return nullptr;
    }
    double v = 0.0;
    if (!clingo_stats_value_get(o->stats, o->key, &v)) {
        return py_handle_error();
    }
    return PyFloat_FromDouble(v);
}

auto py_stats_array_view_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_array_subscript_impl(self, key, state->stats_view_type);
}

auto py_stats_array_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_array_subscript_impl(self, key, state->stats_type);
}

auto py_stats_array_view_iter(PyObject *self) -> PyObject * {
    return py_stats_iter_new(self, IterKind::array_items_view);
}

auto py_stats_array_iter(PyObject *self) -> PyObject * {
    return py_stats_iter_new(self, IterKind::array_items);
}

auto py_stats_array_ass_subscript(PyObject *self, PyObject *key, PyObject *value) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "deleting elements is not supported");
        return -1;
    }

    auto *o = py_stats_cast(self);
    size_t i = 0;
    if (stats_key_to_size(key, &i) < 0) {
        return -1;
    }

    size_t n = 0;
    if (!clingo_stats_array_size(o->stats, o->key, &n)) {
        py_handle_error();
        return -1;
    }

    if (i >= n) {
        PyErr_SetString(PyExc_IndexError, "array index out of bounds");
        return -1;
    }

    uint64_t subkey = 0;
    if (!clingo_stats_array_at(o->stats, o->key, i, &subkey)) {
        py_handle_error();
        return -1;
    }

    return stats_update(o->stats, subkey, value, false);
}

auto py_stats_array_append(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_stats_cast(self);

    clingo_stats_type_t subtype = 0;
    auto prepared = Guard<PyObject *, Py_DECREF>(nullptr);
    if (stats_prepare_value(o->stats, 0, false, arg, &subtype, &prepared.get()) < 0) {
        return nullptr;
    }

    uint64_t subkey = 0;
    if (!clingo_stats_array_push(o->stats, o->key, subtype, &subkey)) {
        return py_handle_error();
    }

    if (stats_update(o->stats, subkey, prepared.get(), true) < 0) {
        return nullptr;
    }

    Py_RETURN_NONE;
}

auto py_stats_map_view_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_map_subscript_impl(self, key, state->stats_view_type);
}

auto py_stats_map_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return stats_map_subscript_impl(self, key, state->stats_type);
}

auto py_stats_map_view_iter(PyObject *self) -> PyObject * {
    return py_stats_iter_new(self, IterKind::map_keys);
}

auto py_stats_map_keys(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_stats_iter_new(self, IterKind::map_keys);
}

auto py_stats_map_view_values(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_stats_iter_new(self, IterKind::map_values_view);
}

auto py_stats_map_values(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_stats_iter_new(self, IterKind::map_values);
}

auto py_stats_map_view_items(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_stats_iter_new(self, IterKind::map_items_view);
}

auto py_stats_map_items(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return py_stats_iter_new(self, IterKind::map_items);
}

auto py_stats_map_ass_subscript(PyObject *self, PyObject *key, PyObject *value) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "deleting keys is not supported");
        return -1;
    }

    auto *o = py_stats_cast(self);

    char const *k = nullptr;
    Py_ssize_t kn = 0;
    if (stats_key_to_str(key, &k, &kn) < 0) {
        return -1;
    }

    uint64_t old_subkey = 0;
    bool found = false;
    if (!stats_get_subkey_by_name(o->stats, o->key, std::string_view{k, static_cast<size_t>(kn)}, &old_subkey,
                                  &found)) {
        return -1;
    }

    clingo_stats_type_t subtype = 0;
    auto prepared = Guard<PyObject *, Py_DECREF>(nullptr);
    if (stats_prepare_value(o->stats, old_subkey, found, value, &subtype, &prepared.get()) < 0) {
        return -1;
    }

    uint64_t subkey = 0;
    if (!clingo_stats_map_add_subkey(o->stats, o->key, k, static_cast<size_t>(kn), subtype, &subkey)) {
        py_handle_error();
        return -1;
    }

    return stats_update(o->stats, subkey, prepared.get(), !found);
}

auto py_stats_update_method(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_stats_cast(self);
    if (stats_update(o->stats, o->key, arg, false) < 0) {
        return nullptr;
    }
    Py_RETURN_NONE;
}

auto py_stats_set_value(PyObject *self, PyObject *value, [[maybe_unused]] void *arg) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "cannot delete value");
        return -1;
    }

    auto *o = py_stats_cast(self);
    clingo_stats_type_t ty = 0;
    if (!stats_type_of(o->stats, o->key, &ty)) {
        return -1;
    }
    if (ty != clingo_stats_type_value) {
        PyErr_SetString(PyExc_TypeError, "not a value");
        return -1;
    }

    auto x = PyFloat_AsDouble(value);
    if (PyErr_Occurred() != nullptr) {
        return -1;
    }
    if (!clingo_stats_value_set(o->stats, o->key, x)) {
        py_handle_error();
        return -1;
    }
    return 0;
}

auto py_stats_iter_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Lazy iterator over stats containers."))},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(PyObject_SelfIter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_stats_iter_next)},
    PyType_Slot{0, nullptr},
};

auto py_stats_iter_spec = PyType_Spec{
    "clingo.stats._StatsIter",  sizeof(PyStatsIterObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_stats_iter_slots.data(),
};

auto py_stats_view_methods = std::array{
    PyMethodDef{"nestify", py_stats_view_nestify, METH_NOARGS, PyDoc_STR("Convert stats to nested Python objects.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_stats_view_getset = std::array{
    PyGetSetDef{"type", py_stats_get_type_enum, nullptr, PyDoc_STR("Get the type of this stats object."), nullptr},
    PyGetSetDef{"array", py_stats_view_get_array, nullptr, PyDoc_STR("Get as StatsArrayView."), nullptr},
    PyGetSetDef{"map", py_stats_view_get_map, nullptr, PyDoc_STR("Get as StatsMapView."), nullptr},
    PyGetSetDef{"value", py_stats_view_get_value, nullptr, PyDoc_STR("Get scalar value."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_stats_view_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Class representing read-only solver stats."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_stats_str)},
    PyType_Slot{Py_tp_methods, py_stats_view_methods.data()},
    PyType_Slot{Py_tp_getset, py_stats_view_getset.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_stats_view_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_stats_view_subscript)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_stats_view_contains)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_stats_view_iter)},
    PyType_Slot{0, nullptr},
};

auto py_stats_view_spec = PyType_Spec{
    "clingo.stats.StatsView",
    sizeof(PyStatsObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_BASETYPE,
    py_stats_view_slots.data(),
};

auto py_stats_array_view_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Class representing a read-only array of stats."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_stats_str)},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_stats_view_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_stats_array_view_subscript)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_stats_array_view_iter)},
    PyType_Slot{0, nullptr},
};

auto py_stats_array_view_spec = PyType_Spec{
    "clingo.stats.StatsArrayView",
    sizeof(PyStatsObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING | Py_TPFLAGS_BASETYPE,
    py_stats_array_view_slots.data(),
};

auto py_stats_array_methods = std::array{
    PyMethodDef{"append", py_stats_array_append, METH_O, PyDoc_STR("Append a value to the array.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_stats_array_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Class representing an array of stats."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_stats_str)},
    PyType_Slot{Py_tp_methods, py_stats_array_methods.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_stats_view_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_stats_array_subscript)},
    PyType_Slot{Py_mp_ass_subscript, reinterpret_cast<void *>(py_stats_array_ass_subscript)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_stats_array_iter)},
    PyType_Slot{0, nullptr},
};

auto py_stats_array_spec = PyType_Spec{
    "clingo.stats.StatsArray",
    sizeof(PyStatsObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING,
    py_stats_array_slots.data(),
};

auto py_stats_map_view_methods = std::array{
    PyMethodDef{"keys", py_stats_map_keys, METH_NOARGS, PyDoc_STR("Get iterator over keys.")},
    PyMethodDef{"values", py_stats_map_view_values, METH_NOARGS, PyDoc_STR("Get iterator over values.")},
    PyMethodDef{"items", py_stats_map_view_items, METH_NOARGS, PyDoc_STR("Get iterator over items.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_stats_map_view_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Class representing a read-only map of stats."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_stats_str)},
    PyType_Slot{Py_tp_methods, py_stats_map_view_methods.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_stats_view_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_stats_map_view_subscript)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_stats_map_view_iter)},
    PyType_Slot{0, nullptr},
};

auto py_stats_map_view_spec = PyType_Spec{
    "clingo.stats.StatsMapView",
    sizeof(PyStatsObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING | Py_TPFLAGS_BASETYPE,
    py_stats_map_view_slots.data(),
};

auto py_stats_map_methods = std::array{
    PyMethodDef{"keys", py_stats_map_keys, METH_NOARGS, PyDoc_STR("Get iterator over keys.")},
    PyMethodDef{"values", py_stats_map_values, METH_NOARGS, PyDoc_STR("Get iterator over values.")},
    PyMethodDef{"items", py_stats_map_items, METH_NOARGS, PyDoc_STR("Get iterator over items.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_stats_map_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Class representing a map of stats."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_stats_str)},
    PyType_Slot{Py_tp_methods, py_stats_map_methods.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_stats_view_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_stats_map_subscript)},
    PyType_Slot{Py_mp_ass_subscript, reinterpret_cast<void *>(py_stats_map_ass_subscript)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_stats_map_view_iter)},
    PyType_Slot{0, nullptr},
};

auto py_stats_map_spec = PyType_Spec{
    "clingo.stats.StatsMap",
    sizeof(PyStatsObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_MAPPING,
    py_stats_map_slots.data(),
};

auto py_stats_methods = std::array{
    PyMethodDef{"update", py_stats_update_method, METH_O, PyDoc_STR("Update statistics with nested values.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_stats_getset = std::array{
    PyGetSetDef{"type", py_stats_get_type_enum, nullptr, PyDoc_STR("Get the type of this stats object."), nullptr},
    PyGetSetDef{"array", py_stats_get_array, nullptr, PyDoc_STR("Get as StatsArray."), nullptr},
    PyGetSetDef{"map", py_stats_get_map, nullptr, PyDoc_STR("Get as StatsMap."), nullptr},
    PyGetSetDef{"value", py_stats_view_get_value, py_stats_set_value, PyDoc_STR("Get/set scalar value."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_stats_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Class representing solver stats."))},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_stats_str)},
    PyType_Slot{Py_tp_methods, py_stats_methods.data()},
    PyType_Slot{Py_tp_getset, py_stats_getset.data()},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_stats_view_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_stats_subscript)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_stats_view_contains)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_stats_iter)},
    PyType_Slot{0, nullptr},
};

auto py_stats_spec = PyType_Spec{
    "clingo.stats.Stats",  sizeof(PyStatsObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_stats_slots.data(),
};

auto stats_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<StatsModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->stats_type_enum);
    for (auto &obj : state->stats_type_vals) {
        Py_VISIT(obj);
    }
    Py_VISIT(state->stats_view_type);
    Py_VISIT(state->stats_type);
    Py_VISIT(state->stats_array_view_type);
    Py_VISIT(state->stats_array_type);
    Py_VISIT(state->stats_map_view_type);
    Py_VISIT(state->stats_map_type);
    Py_VISIT(state->stats_iter_type);
    return 0;
}

auto stats_module_clear(PyObject *module) -> int {
    auto *state = static_cast<StatsModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->stats_type_enum);
    for (auto &obj : state->stats_type_vals) {
        Py_CLEAR(obj);
    }
    Py_CLEAR(state->stats_view_type);
    Py_CLEAR(state->stats_type);
    Py_CLEAR(state->stats_array_view_type);
    Py_CLEAR(state->stats_array_type);
    Py_CLEAR(state->stats_map_view_type);
    Py_CLEAR(state->stats_map_type);
    Py_CLEAR(state->stats_iter_type);
    return 0;
}

void stats_module_free(void *module) {
    stats_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef stats_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.stats",
    PyDoc_STR("Functions and classes related to solver stats."),
    static_cast<Py_ssize_t>(sizeof(StatsModuleState)),
    nullptr,
    nullptr,
    stats_module_traverse,
    stats_module_clear,
    stats_module_free,
};

} // namespace

void register_stats(pybind11::module &m) {
    auto stats_mod = Guard<PyObject *, Py_DECREF>(nullptr);
    if (!add_submodule(m.ptr(), &stats_def, &stats_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(stats_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    if (!add_int_enum(stats_mod, "StatsType", stats_type_members, &state->stats_type_enum, state->stats_type_vals)) {
        throw py::error_already_set();
    }

    if (!add_type(stats_mod.get(), "_StatsIter", &py_stats_iter_spec, &state->stats_iter_type) ||
        !add_type(stats_mod.get(), "StatsView", &py_stats_view_spec, &state->stats_view_type) ||
        !add_type(stats_mod.get(), "Stats", &py_stats_spec, &state->stats_type, state->stats_view_type) ||
        !add_type(stats_mod.get(), "StatsArrayView", &py_stats_array_view_spec, &state->stats_array_view_type) ||
        !add_type(stats_mod.get(), "StatsArray", &py_stats_array_spec, &state->stats_array_type,
                  state->stats_array_view_type) ||
        !add_type(stats_mod.get(), "StatsMapView", &py_stats_map_view_spec, &state->stats_map_view_type) ||
        !add_type(stats_mod.get(), "StatsMap", &py_stats_map_spec, &state->stats_map_type,
                  state->stats_map_view_type)) {
        throw py::error_already_set();
    }
}

auto py_stats_new(PyObject *type, clingo_stats_t *stats, uint64_t key) -> PyObject * {
    return py_stats_alloc(type, stats, key);
}

auto py_stats_type() -> PyObject * {
    auto mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("clingo.stats"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    return state->stats_type;
}

auto py_stats_view_type() -> PyObject * {
    auto mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("clingo.stats"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    return state->stats_view_type;
}

auto py_stats_get(PyObject *type, PyObject *obj, clingo_stats_t **stats, uint64_t *key) -> bool {
    assert(type != nullptr && obj != nullptr);
    if (PyObject_TypeCheck(obj, reinterpret_cast<PyTypeObject *>(type)) == 0) {
        PyErr_SetString(PyExc_TypeError, "object is not of expected clingo.stats type");
        return false;
    }
    auto *s = reinterpret_cast<PyStatsObject *>(obj);
    if (stats != nullptr) {
        *stats = s->stats;
    }
    if (key != nullptr) {
        *key = s->key;
    }
    return true;
}

} // namespace PyClingo
