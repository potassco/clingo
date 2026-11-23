#include "util.hh"

namespace PyClingo {

auto add_int_enum(PyObject *module, char const *enum_name, std::span<IntEnumMember const> members, PyObject **store,
                  std::span<PyObject *> cache) -> bool {
    auto enum_mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("enum"));
    if (enum_mod.get() == nullptr) {
        return false;
    }

    auto int_enum = Guard<PyObject *, Py_DECREF>(PyObject_GetAttrString(enum_mod.get(), "IntEnum"));
    if (int_enum.get() == nullptr) {
        return false;
    }

    auto name = Guard<PyObject *, Py_DECREF>(PyUnicode_FromString(enum_name));
    if (name.get() == nullptr) {
        return false;
    }

    auto members_dict = Guard<PyObject *, Py_DECREF>(PyDict_New());
    if (members_dict.get() == nullptr) {
        return false;
    }

    for (auto const &[member_name, member_value] : members) {
        auto value = Guard<PyObject *, Py_DECREF>(PyLong_FromLong(member_value));
        if (value.get() == nullptr || PyDict_SetItemString(members_dict.get(), member_name, value.get()) < 0) {
            return false;
        }
    }

    auto enum_obj = Guard<PyObject *, Py_DECREF>(
        PyObject_CallFunctionObjArgs(int_enum.get(), name.get(), members_dict.get(), nullptr));
    if (enum_obj.get() == nullptr) {
        return false;
    }

    auto const *module_name = PyModule_GetName(module);
    if (module_name == nullptr) {
        return false;
    }

    auto mod = Guard<PyObject *, Py_DECREF>(PyUnicode_FromString(module_name));
    if (mod.get() == nullptr || PyObject_SetAttrString(enum_obj.get(), "__module__", mod.get()) < 0) {
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
            if (member.get() == nullptr) {
                return false;
            }
            auto idx = static_cast<size_t>(member_value);
            assert(idx < cache.size() && cache[idx] == nullptr);
            cache[idx] = member.release();
        }
    }

    if (PyModule_AddObjectRef(module, enum_name, enum_obj.get()) < 0) {
        return false;
    }

    *store = enum_obj.release();

    return true;
}

auto add_type(PyObject *module, const char *name, PyType_Spec *spec, PyObject **store, PyObject *base) -> bool {
    auto bases = Guard<PyObject *, Py_DECREF>(nullptr);
    if (base != nullptr) {
        bases.get() = PyTuple_Pack(1, base);
        if (bases.get() == nullptr) {
            return false;
        }
    }

    auto obj = Guard<PyObject *, Py_DECREF>(PyType_FromModuleAndSpec(module, spec, bases.get()));
    if (obj.get() == nullptr || PyModule_AddObjectRef(module, name, obj.get()) < 0) {
        return false;
    }

    *store = obj.release();
    return true;
}

auto register_abc(PyObject *type, char const *abc) -> bool {
    auto mod = steal(PyImport_ImportModule("collections.abc"));
    auto cls = steal(mod.get() != nullptr ? PyObject_GetAttrString(mod.get(), abc) : nullptr);
    auto res = steal(cls.get() != nullptr ? PyObject_CallMethod(cls.get(), "register", "O", type) : nullptr);
    return res.get() != nullptr;
}

auto add_submodule(PyObject *module, PyModuleDef *def, PyObject **store) -> bool {
    const char *n = std::strrchr(def->m_name, '.');
    if (n == nullptr) {
        PyErr_Format(PyExc_RuntimeError, "invalid submodule name '%s'", def->m_name);
        return false;
    }
    n = n + 1;

    auto submodule = steal(PyModule_Create(def));
    auto *modules = PyImport_GetModuleDict();
    if (submodule.get() == nullptr || modules == nullptr || PyModule_AddObjectRef(module, n, submodule.get()) < 0 ||
        PyDict_SetItemString(modules, def->m_name, submodule.get()) < 0) {
        return false;
    }
    *store = submodule.release();
    return true;
}

// asyncio integration

auto take_error() -> PyObject * {
    // NOTE: PyErr_GetRaisedException requires Python 3.12
    PyObject *type = nullptr;
    PyObject *value = nullptr;
    PyObject *traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);
    if (value != nullptr && traceback != nullptr) {
        PyException_SetTraceback(value, traceback);
    }
    Py_XDECREF(type);
    Py_XDECREF(traceback);
    return value;
}

auto future_new(PyObject *loop) -> PyObject * {
    return PyObject_CallMethod(loop, "create_future", nullptr);
}

namespace {

auto future_flag(PyObject *fut, char const *name) -> int {
    auto res = Guard<PyObject *, Py_DECREF>(PyObject_CallMethod(fut, name, nullptr));
    return res.get() != nullptr ? PyObject_IsTrue(res.get()) : -1;
}

auto future_set(PyObject *fut, char const *name, PyObject *value) -> bool {
    auto done = future_flag(fut, "done");
    if (done != 0) {
        return done > 0;
    }
    auto set = Guard<PyObject *, Py_DECREF>(PyObject_GetAttrString(fut, name));
    if (set.get() == nullptr) {
        return false;
    }
    auto res = Guard<PyObject *, Py_DECREF>(PyObject_CallFunctionObjArgs(set.get(), value, nullptr));
    return res.get() != nullptr;
}

} // namespace

auto future_done(PyObject *fut) -> int {
    return future_flag(fut, "done");
}

auto future_cancelled(PyObject *fut) -> int {
    return future_flag(fut, "cancelled");
}

auto future_set_result(PyObject *fut, PyObject *value) -> bool {
    return future_set(fut, "set_result", value);
}

auto future_set_exception(PyObject *fut, PyObject *exc) -> bool {
    return future_set(fut, "set_exception", exc);
}

auto future_stop(PyObject *fut) -> bool {
    auto exc = Guard<PyObject *, Py_DECREF>(PyObject_CallNoArgs(PyExc_StopAsyncIteration));
    return exc.get() != nullptr && future_set_exception(fut, exc.get());
}

auto future_add_done(PyObject *fut, PyMethodDef *def, PyObject *self) -> bool {
    auto fun = Guard<PyObject *, Py_DECREF>(PyCFunction_NewEx(def, self, nullptr));
    if (fun.get() == nullptr) {
        return false;
    }
    auto add = Guard<PyObject *, Py_DECREF>(PyObject_GetAttrString(fut, "add_done_callback"));
    if (add.get() == nullptr) {
        return false;
    }
    auto res = Guard<PyObject *, Py_DECREF>(PyObject_CallFunctionObjArgs(add.get(), fun.get(), nullptr));
    return res.get() != nullptr;
}

namespace {

auto py_future_chain(PyObject *dst, PyObject *src) -> PyObject * {
    auto exc = Guard<PyObject *, Py_DECREF>(PyObject_CallMethod(src, "exception", nullptr));
    if (exc.get() == nullptr) {
        return nullptr;
    }
    if (exc.get() != Py_None) {
        return future_set_exception(dst, exc.get()) ? Py_NewRef(Py_None) : nullptr;
    }
    auto res = Guard<PyObject *, Py_DECREF>(PyObject_CallMethod(src, "result", nullptr));
    return res.get() != nullptr && future_set_result(dst, res.get()) ? Py_NewRef(Py_None) : nullptr;
}

PyMethodDef future_chain_def = {"_chain", py_future_chain, METH_O, nullptr};

} // namespace

auto future_chain(PyObject *src, PyObject *dst) -> bool {
    return future_add_done(src, &future_chain_def, dst);
}

auto loop_post(PyObject *post, PyMethodDef *def, PyObject *self) -> bool {
    auto fun = Guard<PyObject *, Py_DECREF>(PyCFunction_NewEx(def, self, nullptr));
    if (fun.get() == nullptr) {
        return false;
    }
    auto res = Guard<PyObject *, Py_DECREF>(PyObject_CallFunctionObjArgs(post, fun.get(), nullptr));
    return res.get() != nullptr;
}

} // namespace PyClingo
