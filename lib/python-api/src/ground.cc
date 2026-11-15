#define PY_SSIZE_T_CLEAN

#include "ground.hh"

#include "core.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/ground.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <utility>

namespace PyClingo {

namespace {

struct GroundModuleState {
    PyObject *symbol_type{nullptr};
    PyObject *ground_result_enum{nullptr};
    PyObject *handle_type{nullptr};
};

using PyGroundHandleObject = struct {
    PyObject_HEAD clingo_ground_handle_t *hnd;
    GroundModuleState *state;
    PyObject *owner;
    PyObject *ctx;
    PyObject *finish;
};

constexpr auto ground_result_members = std::array<IntEnumMember, 3>{
    IntEnumMember{"Ok", static_cast<int>(clingo_ground_result_ok)},
    IntEnumMember{"Unsatisfiable", static_cast<int>(clingo_ground_result_unsatisfiable)},
    IntEnumMember{"Interrupted", static_cast<int>(clingo_ground_result_interrupted)},
};

auto get_state(PyObject *module) -> GroundModuleState * {
    auto *state = static_cast<GroundModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "ground module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_type(PyObject *type) -> GroundModuleState * {
    auto *state = static_cast<GroundModuleState *>(PyType_GetModuleState(reinterpret_cast<PyTypeObject *>(type)));
    if (state == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "ground module state is unavailable");
        }
        return nullptr;
    }
    return state;
}

auto py_handle_cast(PyObject *obj) -> PyGroundHandleObject * {
    return reinterpret_cast<PyGroundHandleObject *>(obj);
}

auto eval_context(GroundModuleState *state, PyObject *ctx, char const *name, size_t name_size,
                  clingo_symbol_t const *arguments, size_t arguments_size, PyGuard &keep,
                  PyBuffer<clingo_symbol_t> &out) -> bool {
    auto fun_name = steal(PyUnicode_FromStringAndSize(name, static_cast<Py_ssize_t>(name_size)));
    if (fun_name.get() == nullptr) {
        return false;
    }
    auto fun = steal(PyObject_GetAttr(ctx, fun_name.get()));
    if (fun.get() == nullptr) {
        return false;
    }

    auto args = steal(PyTuple_New(static_cast<Py_ssize_t>(arguments_size)));
    if (args.get() == nullptr) {
        return false;
    }
    for (size_t i = 0; i < arguments_size; ++i) {
        auto *sym = py_symbol_new(state->symbol_type, arguments[i], true);
        if (sym == nullptr) {
            return false;
        }
        if (PyTuple_SetItem(args.get(), static_cast<Py_ssize_t>(i), sym) < 0) {
            return false;
        }
    }

    auto res = steal(PyObject_CallObject(fun.get(), args.get()));
    if (res.get() == nullptr) {
        return false;
    }

    clingo_symbol_t sym = 0;
    auto is_sym = py_symbol_check(state->symbol_type, res.get(), &sym);
    if (is_sym < 0) {
        return false;
    }
    if (is_sym > 0) {
        if (!out.alloc(1)) {
            return false;
        }
        out[0] = sym;
        keep = std::move(res);
        return true;
    }

    auto seq = steal(PySequence_Tuple(res.get()));
    if (seq.get() == nullptr) {
        if (PyErr_ExceptionMatches(PyExc_TypeError) != 0) {
            PyErr_Clear();
            PyErr_SetString(PyExc_TypeError, "context function must return a symbol or a sequence of symbols");
        }
        return false;
    }
    auto n = PyTuple_GET_SIZE(seq.get());
    if (!out.alloc(static_cast<size_t>(n))) {
        return false;
    }
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *item = PyTuple_GetItem(seq.get(), i);
        if (item == nullptr) {
            return false;
        }
        auto check = py_symbol_check(state->symbol_type, item, &out[static_cast<size_t>(i)]);
        if (check < 0) {
            return false;
        }
        if (check == 0) {
            PyErr_SetString(PyExc_TypeError, "context function must return a symbol or a sequence of symbols");
            return false;
        }
    }
    keep = std::move(seq);
    return true;
}

auto context_call(GroundModuleState *state, PyObject *ctx, char const *name, size_t name_size,
                  clingo_symbol_t const *arguments, size_t arguments_size, clingo_symbol_callback_t symbol_callback,
                  void *symbol_callback_data) -> bool {
    auto gil = GILGuard{};
    PyBuffer<clingo_symbol_t> syms;
    auto keep = PyGuard{nullptr};
    if (!eval_context(state, ctx, name, name_size, arguments, arguments_size, keep, syms)) {
        return store_python_error("context function failed");
    }
    return symbol_callback(syms.data(), syms.size(), symbol_callback_data);
}

auto context_callable([[maybe_unused]] char const *name, [[maybe_unused]] size_t name_size,
                      [[maybe_unused]] size_t arguments_size, [[maybe_unused]] void *data, bool *result) -> bool {
    *result = true;
    return true;
}

struct BlockingData {
    GroundModuleState *state;
    PyObject *ctx;
};

auto blocking_call([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *location,
                   char const *name, size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                   void *data, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> bool {
    auto *bd = static_cast<BlockingData *>(data);
    return context_call(bd->state, bd->ctx, name, name_size, arguments, arguments_size, symbol_callback,
                        symbol_callback_data);
}

auto start_call([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *location,
                char const *name, size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> bool {
    auto *h = py_handle_cast(static_cast<PyObject *>(data));
    return context_call(h->state, h->ctx, name, name_size, arguments, arguments_size, symbol_callback,
                        symbol_callback_data);
}

void start_finish(clingo_ground_result_t result, void *data) {
    auto gil = GILGuard{};
    auto *h = py_handle_cast(static_cast<PyObject *>(data));
    if (h->finish == nullptr) {
        return;
    }
    auto res = steal(PyObject_CallFunction(h->state->ground_result_enum, "i", static_cast<int>(result)));
    auto ret = steal(res.get() != nullptr ? PyObject_CallOneArg(h->finish, res.get()) : nullptr);
    if (ret.get() == nullptr) {
        PyErr_Print();
        fprintf(stderr, "panic: ground finish callback failed\n");
        std::abort();
    }
}

auto ensure_handle_open(PyObject *self) -> clingo_ground_handle_t * {
    auto *h = py_handle_cast(self);
    if (h->hnd == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "ground handle is closed");
        return nullptr;
    }
    return h->hnd;
}

// Closes the handle (releasing the GIL); returns false if clingo reported an error.
auto close_handle(PyGroundHandleObject *h) -> bool {
    auto *hnd = std::exchange(h->hnd, nullptr);
    if (hnd == nullptr) {
        return true;
    }
    return with_gil_released([&]() {
        clingo_ground_handle_close(hnd);
        return true;
    });
}

auto py_handle_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *h = py_handle_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(h->owner);
    Py_VISIT(h->ctx);
    Py_VISIT(h->finish);
    return 0;
}

auto py_handle_clear(PyObject *self) -> int {
    auto *h = py_handle_cast(self);
    close_handle(h);
    Py_CLEAR(h->owner);
    Py_CLEAR(h->ctx);
    Py_CLEAR(h->finish);
    return 0;
}

auto py_handle_get(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    clingo_ground_result_t res = 0;
    if (!with_gil_released([&]() { return clingo_ground_handle_get(hnd, &res); })) {
        return py_handle_error();
    }
    return PyObject_CallFunction(py_handle_cast(self)->state->ground_result_enum, "i", static_cast<int>(res));
}

auto py_handle_cancel(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *hnd = ensure_handle_open(self);
    if (hnd == nullptr) {
        return nullptr;
    }
    if (!with_gil_released([&]() { return clingo_ground_handle_cancel(hnd); })) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_handle_wait(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"timeout", nullptr};
    PyObject *timeout_obj = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O:wait", cast_kw(kwlist), &timeout_obj) == 0) {
        return nullptr;
    }
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
    if (!with_gil_released([&]() { return clingo_ground_handle_wait(hnd, timeout, &result); })) {
        return py_handle_error();
    }
    return PyBool_FromLong(result ? 1 : 0);
}

auto py_handle_close(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    if (!close_handle(py_handle_cast(self))) {
        return py_handle_error();
    }
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
    if (!close_handle(py_handle_cast(self))) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_handle_methods = std::array{
    PyMethodDef{
        "get",
        py_handle_get,
        METH_NOARGS,
        PyDoc_STR("Get the ground result."),
    },
    PyMethodDef{
        "wait",
        py_c_function_cast(py_handle_wait),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Wait for the ground call to finish or the next result with an optional timeout."),
    },
    PyMethodDef{
        "cancel",
        py_handle_cancel,
        METH_NOARGS,
        PyDoc_STR("Cancel the running search."),
    },
    PyMethodDef{"close", py_handle_close, METH_NOARGS, PyDoc_STR("Stop grounding and close the handle.")},
    PyMethodDef{"__enter__", py_handle_enter, METH_NOARGS, PyDoc_STR("Start the search.")},
    PyMethodDef{"__exit__", py_handle_exit, METH_VARARGS, PyDoc_STR("Stop grounding and close the handle.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_handle_slots = std::array{
    PyType_Slot{
        Py_tp_doc,
        PyDoc_STR(const_cast<char *>("An object to interact with a running search.")),
    },
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_handle_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_handle_clear)},
    PyType_Slot{Py_tp_methods, py_handle_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_handle_spec = PyType_Spec{
    "clingo.ground.GroundHandle",
    sizeof(PyGroundHandleObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_handle_slots.data(),
};

auto handle_new(GroundModuleState *state, PyObject *owner, PyObject *ctx, PyObject *finish) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(state->handle_type);
    auto *obj = PyType_GenericAlloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *h = py_handle_cast(obj);
    h->state = state;
    h->owner = owner;
    h->ctx = ctx;
    h->finish = finish;
    Py_XINCREF(owner);
    Py_XINCREF(ctx);
    Py_XINCREF(finish);
    return obj;
}

auto ground_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<GroundModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->symbol_type);
    Py_VISIT(state->ground_result_enum);
    Py_VISIT(state->handle_type);
    return 0;
}

auto ground_module_clear(PyObject *module) -> int {
    auto *state = static_cast<GroundModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->symbol_type);
    Py_CLEAR(state->ground_result_enum);
    Py_CLEAR(state->handle_type);
    return 0;
}

void ground_module_free(void *module) {
    ground_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef ground_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.ground",
    PyDoc_STR("Functions and classes related to grounding."),
    static_cast<Py_ssize_t>(sizeof(GroundModuleState)),
    nullptr,
    nullptr,
    ground_module_traverse,
    ground_module_clear,
    ground_module_free,
};

} // namespace

auto py_ground_handle_type() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.ground"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    if (state->handle_type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.ground module is not initialized");
        return nullptr;
    }
    Py_INCREF(state->handle_type);
    return state->handle_type;
}

auto py_ground(PyObject *type, clingo_control_t *ctl, clingo_part_t const *parts, size_t size, PyObject *ctx) -> bool {
    if (ctx == Py_None) {
        ctx = nullptr;
    }

    GroundModuleState *state = nullptr;
    if (ctx != nullptr) {
        state = get_state_from_type(type);
        if (state == nullptr) {
            return false;
        }
    }

    auto data = BlockingData{state, ctx};
    auto handler = clingo_ground_event_handler_t{context_callable, blocking_call, nullptr, nullptr};
    auto ok = with_gil_released(
        [&]() { return clingo_control_ground(ctl, parts, size, ctx != nullptr ? &handler : nullptr, &data); });
    if (!ok) {
        py_handle_error();
        return false;
    }
    return true;
}

auto py_ground_start(PyObject *type, clingo_control_t *ctl, PyObject *owner, clingo_part_t const *parts, size_t size,
                     PyObject *ctx, PyObject *finish) -> PyObject * {
    if (ctx == Py_None) {
        ctx = nullptr;
    }
    if (finish == Py_None) {
        finish = nullptr;
    }
    if (finish != nullptr && PyCallable_Check(finish) == 0) {
        PyErr_SetString(PyExc_TypeError, "on_finish must be callable");
        return nullptr;
    }

    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }

    auto obj = steal(handle_new(state, owner, ctx, finish));
    if (obj.get() == nullptr) {
        return nullptr;
    }

    auto handler = clingo_ground_event_handler_t{
        ctx != nullptr ? context_callable : nullptr,
        ctx != nullptr ? start_call : nullptr,
        finish != nullptr ? start_finish : nullptr,
        nullptr,
    };
    auto has_handler = ctx != nullptr || finish != nullptr;
    if (!clingo_control_start_ground(ctl, parts, size, has_handler ? &handler : nullptr,
                                     has_handler ? static_cast<void *>(obj.get()) : nullptr,
                                     &py_handle_cast(obj.get())->hnd)) {
        py_handle_error();
        return nullptr;
    }
    return obj.release();
}

void register_ground(pybind11::module &m) {
    auto ground_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &ground_def, &ground_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(ground_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    state->symbol_type = py_symbol_type();
    if (state->symbol_type == nullptr) {
        throw py::error_already_set();
    }

    if (!add_int_enum(ground_mod.get(), "GroundResult", ground_result_members, &state->ground_result_enum, {}) ||
        !add_type(ground_mod.get(), "GroundHandle", &py_handle_spec, &state->handle_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
