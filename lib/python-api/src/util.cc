#include "util.hh"

namespace PyClingo {

auto add_int_enum(char const *module_name, char const *enum_name, std::span<IntEnumMember const> members,
                  PyObject *module, PyObject **store, std::span<PyObject *> cache) -> bool {
    auto enum_mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("enum"));
    if (enum_mod == nullptr) {
        return false;
    }

    auto int_enum = Guard<PyObject *, Py_DECREF>(PyObject_GetAttrString(enum_mod.get(), "IntEnum"));
    if (int_enum == nullptr) {
        return false;
    }

    auto name = Guard<PyObject *, Py_DECREF>(PyUnicode_FromString(enum_name));
    if (name == nullptr) {
        return false;
    }

    auto members_dict = Guard<PyObject *, Py_DECREF>(PyDict_New());
    if (members_dict == nullptr) {
        return false;
    }

    for (auto const &[member_name, member_value] : members) {
        auto value = Guard<PyObject *, Py_DECREF>(PyLong_FromLong(member_value));
        if (value == nullptr || PyDict_SetItemString(members_dict.get(), member_name, value.get()) < 0) {
            return false;
        }
    }

    auto enum_obj = Guard<PyObject *, Py_DECREF>(
        PyObject_CallFunctionObjArgs(int_enum.get(), name.get(), members_dict.get(), nullptr));
    if (enum_obj == nullptr) {
        return false;
    }

    auto mod = Guard<PyObject *, Py_DECREF>(PyUnicode_FromString(module_name));
    if (mod == nullptr || PyObject_SetAttrString(enum_obj.get(), "__module__", mod.get()) < 0) {
        return false;
    }

    if (!cache.empty()) {
        for (auto const &[member_name, member_value] : members) {
            if (member_value < 0 || static_cast<size_t>(member_value) >= cache.size()) {
                PyErr_Format(PyExc_RuntimeError, "enum value %d for '%s' is out of cache bounds", member_value,
                             member_name);
                return false;
            }

            auto member = Guard<PyObject *, Py_DECREF>(PyObject_CallFunction(enum_obj.get(), "i", member_value));
            if (member == nullptr) {
                return false;
            }
            auto idx = static_cast<size_t>(member_value);
            assert(idx < cache.size() && cache[idx] == nullptr);
            cache[idx] = member.release();
        }
    }

    if (PyModule_AddObjectRef(module, enum_name, *store) < 0) {
        return false;
    }

    *store = enum_obj.release();

    return true;
}

auto add_type(PyObject *module, const char *name, PyType_Spec *spec, PyObject **store) -> bool {
    auto obj = Guard<PyObject *, Py_DECREF>(PyType_FromModuleAndSpec(module, spec, nullptr));
    if (obj.get() == nullptr || PyModule_AddObjectRef(module, name, obj.get()) < 0) {
        return false;
    }
    *store = obj.release();
    return true;
}

} // namespace PyClingo
