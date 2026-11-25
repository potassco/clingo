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

// AsyncStep
//
// Awaiting a future of the event loop directly would hide cancellations of
// the awaiting task if the future is already done. The step forwards to the
// iterator of the future and stops the operation if an exception is thrown
// into the awaiting coroutine.

namespace {

struct PyAsyncStepObject {
    PyObject_HEAD
    //! The handle owning the operation.
    PyObject *handle;
    //! The function stopping the operation.
    AsyncStop stop;
    //! The event loop.
    PyObject *loop;
    //! The iterator of the awaited future.
    PyObject *iter;
    //! The future resolved once the operation stopped.
    PyObject *result;
    //! The exception raised once the operation stopped.
    PyObject *pending;
};

auto step_cast(PyObject *obj) -> PyAsyncStepObject * {
    return reinterpret_cast<PyAsyncStepObject *>(obj);
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

//! Raise the pending exception.
auto step_raise(PyObject *self) -> PyObject * {
    auto *exc = std::exchange(step_cast(self)->pending, nullptr);
    auto *type = Py_NewRef(reinterpret_cast<PyObject *>(Py_TYPE(exc)));
    PyErr_Restore(type, exc, PyException_GetTraceback(exc));
    return nullptr;
}

//! Wait until the operation stopped ignoring its outcome.
auto step_wait(PyObject *self) -> PyObject * {
    auto *s = step_cast(self);
    auto fut = steal(future_new(s->loop));
    if (fut.get() == nullptr || !future_chain(s->result, fut.get())) {
        return nullptr;
    }
    auto *iter = PyObject_CallMethod(fut.get(), "__await__", nullptr);
    if (iter == nullptr) {
        return nullptr;
    }
    Py_DECREF(std::exchange(s->iter, iter));
    auto *none = Py_None;
    return step_call(self, "send", &none, 1);
}

auto py_step_send(PyObject *self, PyObject *value) -> PyObject * {
    auto *res = step_call(self, "send", &value, 1);
    if (res != nullptr || step_cast(self)->pending == nullptr) {
        return res;
    }
    // NOTE: the operation stopped and its outcome is passed via the result future
    PyErr_Clear();
    return step_raise(self);
}

auto py_step_throw(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> PyObject * {
    auto *s = step_cast(self);
    auto done = future_done(s->result);
    if (done < 0) {
        return nullptr;
    }
    if (s->pending == nullptr) {
        if (done == 0 && !s->stop(s->handle)) {
            PyErr_WriteUnraisable(s->handle);
        }
        auto *res = step_call(self, "throw", args, nargs);
        if (res != nullptr || done > 0) {
            return res;
        }
        // NOTE: the exception is raised by the iterator to normalize the arguments
        s->pending = take_error();
    }
    // NOTE: exceptions thrown while waiting (e.g., repeated cancellations) are dropped
    return done > 0 ? step_raise(self) : step_wait(self);
}

auto py_step_close(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    Py_CLEAR(step_cast(self)->pending);
    return step_call(self, "close", nullptr, 0);
}

auto py_step_iternext(PyObject *self) -> PyObject * {
    return py_step_send(self, Py_None);
}

auto py_step_await(PyObject *self) -> PyObject * {
    return Py_NewRef(self);
}

auto py_step_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *s = step_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(s->handle);
    Py_VISIT(s->loop);
    Py_VISIT(s->iter);
    Py_VISIT(s->result);
    Py_VISIT(s->pending);
    return 0;
}

auto py_step_clear(PyObject *self) -> int {
    auto *s = step_cast(self);
    Py_CLEAR(s->handle);
    Py_CLEAR(s->loop);
    Py_CLEAR(s->iter);
    Py_CLEAR(s->result);
    Py_CLEAR(s->pending);
    return 0;
}

auto py_step_methods = std::array{
    PyMethodDef{"send", py_step_send, METH_O, PyDoc_STR("Send a value into the awaited future.")},
    PyMethodDef{
        "throw",
        method_cast(py_step_throw),
        METH_FASTCALL,
        PyDoc_STR("Stop the operation and throw an exception into the awaited future once it stopped."),
    },
    PyMethodDef{"close", py_step_close, METH_NOARGS, PyDoc_STR("Close the iterator of the awaited future.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_step_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("An awaitable step of an asynchronous operation.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_step_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_step_clear)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_step_await)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_step_iternext)},
    PyType_Slot{Py_am_await, reinterpret_cast<void *>(py_step_await)},
    PyType_Slot{Py_tp_methods, py_step_methods.data()},
    PyType_Slot{0, nullptr},
};

} // namespace

PyType_Spec async_step_spec = {
    "clingo._AsyncStep",
    sizeof(PyAsyncStepObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_step_slots.data(),
};

auto async_step_new(PyObject *type, PyObject *handle, AsyncStop stop, PyObject *loop, PyObject *fut, PyObject *result)
    -> PyObject * {
    auto iter = steal(PyObject_CallMethod(fut, "__await__", nullptr));
    if (iter.get() == nullptr) {
        return nullptr;
    }
    auto *step = reinterpret_cast<PyAsyncStepObject *>(PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(type), 0));
    if (step == nullptr) {
        return nullptr;
    }
    step->handle = Py_NewRef(handle);
    step->stop = stop;
    step->loop = Py_NewRef(loop);
    step->iter = iter.release();
    step->result = Py_NewRef(result);
    step->pending = nullptr;
    return reinterpret_cast<PyObject *>(step);
}

} // namespace PyClingo
