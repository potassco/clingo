#define PY_SSIZE_T_CLEAN

#include "config.hh"

#include "core.hh"

#include <array>
#include <limits>

namespace PyClingo {

namespace {

using PyConfigObject = struct {
    PyObject_HEAD clingo_config_t *config;
    PyObject *keeper;
    clingo_id_t key;
};

struct ConfigModuleState {
    PyObject *config_type{nullptr};
};

struct ConfigEntry {
    PyObject *py_get{nullptr};
    PyObject *py_set{nullptr};
    PyObject *py_size{nullptr};
    PyObject *cache{nullptr};
};

auto get_state(PyObject *module) -> ConfigModuleState * {
    auto *state = static_cast<ConfigModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "config module state is unavailable");
        return nullptr;
    }
    return state;
}

auto py_config_cast(PyObject *obj) -> PyConfigObject * {
    return reinterpret_cast<PyConfigObject *>(obj);
}

auto py_config_alloc(PyObject *type, PyObject *keeper, clingo_config_t *config, clingo_id_t key) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    auto *obj = to->tp_alloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *cfg = reinterpret_cast<PyConfigObject *>(obj);
    cfg->keeper = keeper;
    cfg->config = config;
    cfg->key = key;
    return obj;
}

auto py_config_type_bits(PyObject *self, clingo_config_type_bitset_t *type) -> int {
    auto *o = py_config_cast(self);
    if (!clingo_config_type(o->config, o->key, type)) {
        py_handle_error();
        return -1;
    }
    return 0;
}

auto py_config_is_map(PyObject *self) -> int {
    clingo_config_type_bitset_t type = 0;
    if (py_config_type_bits(self, &type) < 0) {
        return -1;
    }
    return (type & clingo_config_type_map) != 0 ? 1 : 0;
}

auto py_config_is_value_impl(PyObject *self) -> int {
    clingo_config_type_bitset_t type = 0;
    if (py_config_type_bits(self, &type) < 0) {
        return -1;
    }
    return (type & clingo_config_type_value) != 0 ? 1 : 0;
}

auto py_config_is_sequence_impl(PyObject *self) -> int {
    clingo_config_type_bitset_t type = 0;
    if (py_config_type_bits(self, &type) < 0) {
        return -1;
    }
    return (type & clingo_config_type_array) != 0 ? 1 : 0;
}

auto unicode_to_str(PyObject *obj, char const **data, Py_ssize_t *size, char const *error) -> bool {
    if (PyUnicode_Check(obj) == 0) {
        PyErr_SetString(PyExc_TypeError, error);
        return false;
    }
    *data = PyUnicode_AsUTF8AndSize(obj, size);
    return *data != nullptr;
}

auto py_config_get_subkey(PyObject *self, PyObject *name_obj, clingo_id_t *subkey, bool *found) -> int {
    auto is_map = py_config_is_map(self);
    if (is_map < 0) {
        return -1;
    }
    if (is_map == 0) {
        PyErr_SetString(PyExc_AttributeError, "invalid attribute");
        return -1;
    }

    char const *name = nullptr;
    Py_ssize_t size = 0;
    if (!unicode_to_str(name_obj, &name, &size, "attribute name must be a string")) {
        return -1;
    }

    auto *o = py_config_cast(self);
    if (!clingo_config_map_at(o->config, o->key, name, static_cast<size_t>(size), subkey, found)) {
        py_handle_error();
        return -1;
    }
    return 0;
}

auto py_config_set_value_impl(PyObject *self, PyObject *value) -> int {
    auto is_value = py_config_is_value_impl(self);
    if (is_value < 0) {
        return -1;
    }
    if (is_value == 0) {
        PyErr_SetString(PyExc_AttributeError, "invalid attribute");
        return -1;
    }

    auto s = steal(PyObject_Str(value));
    if (s.get() == nullptr) {
        return -1;
    }

    char const *data = nullptr;
    Py_ssize_t size = 0;
    if (!unicode_to_str(s.get(), &data, &size, "failed to convert value to string")) {
        return -1;
    }

    auto *o = py_config_cast(self);
    if (!clingo_config_value_set(o->config, o->key, data, static_cast<size_t>(size))) {
        py_handle_error();
        return -1;
    }
    return 0;
}

auto py_config_str(PyObject *self) -> PyObject * {
    auto *o = py_config_cast(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_config_to_string(o->config, o->key, bld)) {
        return py_handle_error();
    }
    clingo_string_t out;
    if (!clingo_string_builder_string(bld, &out)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(out.data, static_cast<Py_ssize_t>(out.size));
}

auto py_config_get_description(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_config_cast(self);
    clingo_string_t desc;
    if (!clingo_config_description(o->config, o->key, &desc)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(desc.data, static_cast<Py_ssize_t>(desc.size));
}

auto py_config_get_is_value(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto is_value = py_config_is_value_impl(self);
    if (is_value < 0) {
        return nullptr;
    }
    if (is_value != 0) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_config_get_value(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    clingo_string_t val;
    bool has_value = false;
    auto *o = py_config_cast(self);
    if (!clingo_config_value_get(o->config, o->key, &val, &has_value)) {
        return py_handle_error();
    }
    if (!has_value) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromStringAndSize(val.data, static_cast<Py_ssize_t>(val.size));
}

auto py_config_set_value(PyObject *self, PyObject *value, [[maybe_unused]] void *arg) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "cannot delete value");
        return -1;
    }
    return py_config_set_value_impl(self, value);
}

auto py_config_get_is_sequence(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto is_seq = py_config_is_sequence_impl(self);
    if (is_seq < 0) {
        return nullptr;
    }
    if (is_seq != 0) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_config_len(PyObject *self) -> Py_ssize_t {
    auto is_seq = py_config_is_sequence_impl(self);
    if (is_seq < 0) {
        return -1;
    }
    if (is_seq == 0) {
        PyErr_SetString(PyExc_AttributeError, "invalid attribute");
        return -1;
    }

    auto *o = py_config_cast(self);
    size_t n = 0;
    if (!clingo_config_array_size(o->config, o->key, &n)) {
        py_handle_error();
        return -1;
    }
    if (std::cmp_greater(n, std::numeric_limits<Py_ssize_t>::max())) {
        PyErr_SetString(PyExc_OverflowError, "size does not fit in Py_ssize_t");
        return -1;
    }
    return static_cast<Py_ssize_t>(n);
}

auto py_config_item(PyObject *self, Py_ssize_t index) -> PyObject * {
    auto *cfg = py_config_cast(self);
    auto *type = reinterpret_cast<PyObject *>(Py_TYPE(self));
    if (index < 0) {
        PyErr_SetString(PyExc_IndexError, "array index out of bounds");
        return nullptr;
    }

    auto *o = py_config_cast(self);
    clingo_id_t subkey = 0;
    if (!clingo_config_array_at(o->config, o->key, static_cast<size_t>(index), &subkey)) {
        return py_handle_error();
    }
    return py_config_alloc(type, cfg->keeper, o->config, subkey);
}

auto py_config_getattr(PyObject *self, PyObject *name) -> PyObject * {
    auto *cfg = py_config_cast(self);
    auto *type = reinterpret_cast<PyObject *>(Py_TYPE(self));
    auto attr = steal(PyObject_GenericGetAttr(self, name));
    if (attr != nullptr) {
        return attr.release();
    }
    if (PyErr_ExceptionMatches(PyExc_AttributeError) == 0) {
        return nullptr;
    }
    PyErr_Clear();

    clingo_id_t subkey = 0;
    bool found = false;
    if (py_config_get_subkey(self, name, &subkey, &found) < 0) {
        return nullptr;
    }
    if (!found) {
        PyErr_SetString(PyExc_AttributeError, "invalid attribute");
        return nullptr;
    }

    auto *o = py_config_cast(self);
    return py_config_alloc(type, cfg->keeper, o->config, subkey);
}

auto py_config_setattr(PyObject *self, PyObject *name, PyObject *value) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "cannot delete configuration entries");
        return -1;
    }
    auto *type = reinterpret_cast<PyObject *>(Py_TYPE(self));
    auto *cfg = py_config_cast(self);
    auto descr = steal(PyObject_GetAttr(type, name));
    if (descr.get() != nullptr) {
        int has_set = PyObject_HasAttrString(descr.get(), "__set__");
        if (has_set < 0) {
            return -1;
        }
        if (has_set != 0) {
            auto res = steal(PyObject_CallMethod(descr.get(), "__set__", "OO", self, value));
            if (res.get() == nullptr) {
                return -1;
            }
            return 0;
        }
    } else if (PyErr_ExceptionMatches(PyExc_AttributeError) != 0) {
        PyErr_Clear();
    } else {
        return -1;
    }

    clingo_id_t subkey = 0;
    bool found = false;
    if (py_config_get_subkey(self, name, &subkey, &found) < 0) {
        return -1;
    }
    if (!found) {
        PyErr_SetString(PyExc_AttributeError, "invalid attribute");
        return -1;
    }

    auto *o = py_config_cast(self);
    auto sub = steal(py_config_alloc(type, cfg->keeper, o->config, subkey));
    if (sub.get() == nullptr) {
        return -1;
    }
    if (py_config_set_value_impl(sub.get(), value) < 0) {
        return -1;
    }

    return 0;
}

auto py_config_get_attributes(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto out = steal(PyList_New(0));
    if (out.get() == nullptr) {
        return nullptr;
    }

    auto is_map = py_config_is_map(self);
    if (is_map < 0) {
        return nullptr;
    }
    if (is_map == 0) {
        return out.release();
    }

    auto *o = py_config_cast(self);
    size_t n = 0;
    if (!clingo_config_map_size(o->config, o->key, &n)) {
        return py_handle_error();
    }

    for (size_t i = 0; i < n; ++i) {
        clingo_string_t name;
        if (!clingo_config_map_subkey_name(o->config, o->key, i, &name)) {
            return py_handle_error();
        }
        auto item = steal(PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size)));
        if (item.get() == nullptr) {
            return nullptr;
        }
        if (PyList_Append(out.get(), item.get()) < 0) {
            return nullptr;
        }
    }

    return out.release();
}

auto config_entry_get(size_t const *index, void *data, clingo_string_t *value, bool *has_value) -> bool {
    auto *entry = static_cast<ConfigEntry *>(data);
    auto gil = GILGuard{};

    *has_value = false;
    value->data = nullptr;
    value->size = 0;

    if (entry->py_get == nullptr || entry->py_get == Py_None) {
        return true;
    }

    auto arg = PyGuard{nullptr};
    if (index != nullptr) {
        arg = steal(PyLong_FromSize_t(*index));
        if (arg.get() == nullptr) {
            return store_python_error("failed to build callback argument");
        }
    }

    auto ret = PyGuard{nullptr};
    if (index != nullptr) {
        ret = steal(PyObject_CallFunctionObjArgs(entry->py_get, arg.get(), nullptr));
    } else {
        ret = steal(PyObject_CallFunctionObjArgs(entry->py_get, nullptr));
    }
    if (ret.get() == nullptr) {
        return store_python_error("config getter callback failed");
    }

    if (ret.get() == Py_None) {
        return true;
    }

    auto text = steal(PyObject_Str(ret.get()));
    if (text.get() == nullptr) {
        return store_python_error("failed to convert getter result to string");
    }

    Py_XDECREF(entry->cache);
    entry->cache = text.release();

    Py_ssize_t n = 0;
    const auto *buf = PyUnicode_AsUTF8AndSize(entry->cache, &n);
    if (buf == nullptr) {
        return store_python_error("failed to get UTF-8 data from getter result");
    }

    *has_value = true;
    value->data = buf;
    value->size = static_cast<size_t>(n);
    return true;
}

auto config_entry_set(size_t const *index, char const *value, size_t size, void *data) -> bool {
    auto *entry = static_cast<ConfigEntry *>(data);
    GILGuard gil;

    if (entry->py_set == nullptr || entry->py_set == Py_None) {
        auto msg = std::string_view{"invalid attribute"};
        clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
        return false;
    }

    auto text = steal(PyUnicode_FromStringAndSize(value, static_cast<Py_ssize_t>(size)));
    if (text.get() == nullptr) {
        return store_python_error("failed to prepare setter argument");
    }

    auto ret = PyGuard{nullptr};
    if (index != nullptr) {
        auto idx = steal(PyLong_FromSize_t(*index));
        if (idx.get() == nullptr) {
            return store_python_error("failed to prepare index argument");
        }
        ret = steal(PyObject_CallFunctionObjArgs(entry->py_set, text.get(), idx.get(), nullptr));
    } else {
        ret = steal(PyObject_CallFunctionObjArgs(entry->py_set, text.get(), nullptr));
    }

    if (ret.get() == nullptr) {
        return store_python_error("config setter callback failed");
    }

    return true;
}

auto config_entry_size(void *data, size_t *size, bool *has_size) -> bool {
    auto *entry = static_cast<ConfigEntry *>(data);
    auto gil = GILGuard{};

    *has_size = false;
    if (entry->py_size == nullptr || entry->py_size == Py_None) {
        return true;
    }

    auto ret = steal(PyObject_CallFunctionObjArgs(entry->py_size, nullptr));
    if (ret.get() == nullptr) {
        auto ok = store_python_error("config size callback failed");
        return ok;
    }

    auto n = PyLong_AsSize_t(ret.get());
    if (PyErr_Occurred() != nullptr) {
        auto ok = store_python_error("size callback must return non-negative int");
        return ok;
    }

    *size = n;
    *has_size = true;
    return true;
}

void config_entry_free(void *data) {
    auto *entry = static_cast<ConfigEntry *>(data);
    if (entry == nullptr) {
        return;
    }

    auto gil = GILGuard{};
    Py_XDECREF(entry->cache);
    PyMem_Free(entry);
}

auto py_config_add_entry(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 6>{"name", "description", "get", "set", "size", nullptr};

    auto *cfg = py_config_cast(self);
    PyObject *name_obj = nullptr;
    PyObject *desc_obj = nullptr;
    PyObject *get = Py_None;
    PyObject *set = Py_None;
    PyObject *size = Py_None;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO|OOO:add_entry", cast_kw(kwlist), &name_obj, &desc_obj, &get, &set,
                                    &size) == 0) {
        return nullptr;
    }

    char const *name = nullptr;
    Py_ssize_t name_n = 0;
    if (!unicode_to_str(name_obj, &name, &name_n, "name must be a string")) {
        return nullptr;
    }

    char const *desc = nullptr;
    Py_ssize_t desc_n = 0;
    if (!unicode_to_str(desc_obj, &desc, &desc_n, "description must be a string")) {
        return nullptr;
    }

    if (get != Py_None && PyCallable_Check(get) == 0) {
        PyErr_SetString(PyExc_TypeError, "get must be callable or None");
        return nullptr;
    }
    if (set != Py_None && PyCallable_Check(set) == 0) {
        PyErr_SetString(PyExc_TypeError, "set must be callable or None");
        return nullptr;
    }
    if (size != Py_None && PyCallable_Check(size) == 0) {
        PyErr_SetString(PyExc_TypeError, "size must be callable or None");
        return nullptr;
    }

    auto entry_data =
        Guard<ConfigEntry *, config_entry_free>{static_cast<ConfigEntry *>(PyMem_Calloc(1, sizeof(ConfigEntry)))};
    if (entry_data.get() == nullptr) {
        return PyErr_NoMemory();
    }

    if (PyList_Append(cfg->keeper, get) < 0) {
        return nullptr;
    }
    if (PyList_Append(cfg->keeper, set) < 0) {
        return nullptr;
    }
    if (PyList_Append(cfg->keeper, size) < 0) {
        return nullptr;
    }
    entry_data.get()->py_get = get;
    entry_data.get()->py_set = set;
    entry_data.get()->py_size = size;

    auto entry = clingo_config_entry_t{
        get != Py_None ? &config_entry_get : nullptr,
        set != Py_None ? &config_entry_set : nullptr,
        size != Py_None ? &config_entry_size : nullptr,
        &config_entry_free,
    };

    auto *o = py_config_cast(self);
    if (!clingo_config_add(o->config, o->key, name, static_cast<size_t>(name_n), desc, static_cast<size_t>(desc_n),
                           &entry, entry_data.release())) {
        config_entry_free(entry_data);
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

auto py_config_methods = std::array{
    PyMethodDef{
        "add_entry",
        py_c_function_cast(py_config_add_entry),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Add a custom configuration entry."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_config_getset = std::array{
    PyGetSetDef{
        "description",
        py_config_get_description,
        nullptr,
        PyDoc_STR("Get the description of a configuration entry."),
        nullptr,
    },
    PyGetSetDef{
        "is_value",
        py_config_get_is_value,
        nullptr,
        PyDoc_STR("Whether the configuration entry is a value."),
        nullptr,
    },
    PyGetSetDef{
        "value",
        py_config_get_value,
        py_config_set_value,
        PyDoc_STR("Get/set the string value of the configuration entry."),
        nullptr,
    },
    PyGetSetDef{
        "is_sequence",
        py_config_get_is_sequence,
        nullptr,
        PyDoc_STR("Whether the configuration is a sequence."),
        nullptr,
    },
    PyGetSetDef{
        "attributes",
        py_config_get_attributes,
        nullptr,
        PyDoc_STR("Get the attribute names of nested configurations."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_config_slots = std::array{
    PyType_Slot{
        Py_tp_doc,
        PyDoc_STR(const_cast<char *>("Allows for changing the configuration of the underlying solver.")),
    },
    PyType_Slot{Py_tp_getattro, reinterpret_cast<void *>(py_config_getattr)},
    PyType_Slot{Py_tp_setattro, reinterpret_cast<void *>(py_config_setattr)},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_config_str)},
    PyType_Slot{Py_tp_methods, py_config_methods.data()},
    PyType_Slot{Py_tp_getset, py_config_getset.data()},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(py_config_len)},
    PyType_Slot{Py_sq_item, reinterpret_cast<void *>(py_config_item)},
    PyType_Slot{0, nullptr},
};

auto py_config_spec = PyType_Spec{
    "clingo.config.Config", sizeof(PyConfigObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_config_slots.data(),
};

auto config_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<ConfigModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->config_type);
    return 0;
}

auto config_module_clear(PyObject *module) -> int {
    auto *state = static_cast<ConfigModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->config_type);
    return 0;
}

void config_module_free(void *module) {
    config_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef config_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.config",
    PyDoc_STR("Functions and classes related to configuration."),
    static_cast<Py_ssize_t>(sizeof(ConfigModuleState)),
    nullptr,
    nullptr,
    config_module_traverse,
    config_module_clear,
    config_module_free,
};

} // namespace

auto py_config_type_ref() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.config"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    return state->config_type; // borrowed
}

auto py_config_new(PyObject *type, PyObject *keeper, clingo_config_t *config, clingo_id_t key) -> PyObject * {
    if (type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "config type is unavailable");
        return nullptr;
    }
    return py_config_alloc(type, keeper, config, key);
}

auto py_config_get(PyObject *type, PyObject *obj, clingo_config_t **config, clingo_id_t *key) -> bool {
    if (type == nullptr || obj == nullptr || config == nullptr || key == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "invalid argument");
        return false;
    }
    if (PyObject_TypeCheck(obj, reinterpret_cast<PyTypeObject *>(type)) == 0) {
        PyErr_SetString(PyExc_TypeError, "Config expected");
        return false;
    }
    auto *o = py_config_cast(obj);
    *config = o->config;
    *key = o->key;
    return true;
}
void register_config(pybind11::module &m) {
    auto config_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &config_def, &config_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(config_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    if (!add_type(config_mod.get(), "Config", &py_config_spec, &state->config_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
