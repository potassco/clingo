#define PY_SSIZE_T_CLEAN

#include "script.hh"

#include "control.hh"
#include "core.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/script.h>

#include <array>
#include <cstring>
#include <memory>
#include <new>
#include <string_view>
#include <utility>

namespace PyClingo {

namespace {

// Ownership
//
// Scripts registered from Python (`register`, `enable_python`) are
// represented by a `_ScriptHandle` whose address is passed to clingo. The
// library object keeps the handle alive via its keeper list. Because the
// library releases the clingo library only in tp_dealloc (and not in
// tp_clear), all objects referenced by the script (e.g., symbols) are
// released before the clingo library is destroyed.
//
// The main script registered from C (`register_python`) is owned by the
// clingo library and freed via the script's free callback.
//
// Handles do not store their library. Callbacks receive the clingo library
// and get the library object via py_library_attach(), which returns the
// registered owner or attaches a non-owning library object to it.

struct ScriptModuleState {
    PyObject *lib_type{nullptr};
    PyObject *control_type{nullptr};
    PyObject *symbol_type{nullptr};
    PyObject *script_type{nullptr};
    PyObject *handle_type{nullptr};
    PyObject *str_execute{nullptr};
    PyObject *str_call{nullptr};
    PyObject *str_callable{nullptr};
    PyObject *str_main{nullptr};
    PyObject *str_name{nullptr};
    PyObject *str_version{nullptr};
};

constexpr auto state_members = std::array{
    &ScriptModuleState::lib_type,    &ScriptModuleState::control_type, &ScriptModuleState::symbol_type,
    &ScriptModuleState::script_type, &ScriptModuleState::handle_type,  &ScriptModuleState::str_execute,
    &ScriptModuleState::str_call,    &ScriptModuleState::str_callable, &ScriptModuleState::str_main,
    &ScriptModuleState::str_name,    &ScriptModuleState::str_version,
};

struct PyScriptHandleObject {
    PyObject_HEAD
    //! The custom script or nullptr for the main python script.
    PyObject *script;
    //! The global scope of the main python script.
    PyObject *scope;
    //! Cached name of a custom script.
    PyObject *name;
    //! Cached version of a custom script.
    PyObject *version;
};

//! The main python script registered from C.
struct ExternalScript {
    //! The handle of the main python script (created lazily).
    PyObject *handle{nullptr};
    //! The main thread state if the interpreter was initialized by the script.
    PyThreadState *thread{nullptr};
};

auto handle_cast(PyObject *obj) -> PyScriptHandleObject * {
    return reinterpret_cast<PyScriptHandleObject *>(obj);
}

auto get_state(PyObject *module) -> ScriptModuleState * {
    auto *state = static_cast<ScriptModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "script module state is unavailable");
        return nullptr;
    }
    return state;
}

auto state_of(PyObject *self) -> ScriptModuleState * {
    auto *state = static_cast<ScriptModuleState *>(PyType_GetModuleState(Py_TYPE(self)));
    if (state == nullptr && PyErr_Occurred() == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "script module state is unavailable");
    }
    return state;
}

auto set_clingo_error(std::string_view msg) -> bool {
    clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
    return false;
}

// conversion

auto symbol_list(ScriptModuleState *state, clingo_symbol_t const *symbols, size_t size) -> PyObject * {
    auto list = steal(PyList_New(static_cast<Py_ssize_t>(size)));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < size; ++i) {
        auto *sym = py_symbol_new(state->symbol_type, symbols[i], true);
        if (sym == nullptr || PyList_SetItem(list.get(), static_cast<Py_ssize_t>(i), sym) < 0) {
            return nullptr;
        }
    }
    return list.release();
}

//! Pass a Symbol or a sequence of Symbols to the symbol callback.
//!
//! The result object must be kept alive while the callback runs.
auto return_symbols(ScriptModuleState *state, PyObject *res, clingo_symbol_callback_t cb, void *cb_data) -> bool {
    static constexpr char const *msg = "call must return a Symbol or a sequence of Symbols";
    clingo_symbol_t sym = 0;
    auto check = py_symbol_check(state->symbol_type, res, &sym);
    if (check < 0) {
        return store_python_error("converting call result failed");
    }
    if (check > 0) {
        return cb(&sym, 1, cb_data);
    }
    auto seq = steal(PySequence_Tuple(res));
    if (seq.get() == nullptr) {
        PyErr_Clear();
        PyErr_SetString(PyExc_TypeError, msg);
        return store_python_error("converting call result failed");
    }
    auto n = PyTuple_Size(seq.get());
    auto syms = PyBuffer<clingo_symbol_t>{};
    if (!syms.alloc(static_cast<size_t>(n))) {
        return store_python_error("converting call result failed");
    }
    for (Py_ssize_t i = 0; i < n; ++i) {
        check = py_symbol_check(state->symbol_type, PyTuple_GetItem(seq.get(), i), &syms.data()[i]);
        if (check <= 0) {
            if (check == 0) {
                PyErr_SetString(PyExc_TypeError, msg);
            }
            return store_python_error("converting call result failed");
        }
    }
    return cb(syms.data(), syms.size(), cb_data);
}

// _ScriptHandle

auto handle_new(ScriptModuleState *state, PyObject *script, PyObject *scope) -> PyObject * {
    auto *obj = PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(state->handle_type), 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *h = handle_cast(obj);
    Py_XINCREF(script);
    h->script = script;
    Py_XINCREF(scope);
    h->scope = scope;
    return obj;
}

auto py_handle_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *h = handle_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(h->script);
    Py_VISIT(h->scope);
    Py_VISIT(h->name);
    Py_VISIT(h->version);
    return 0;
}

auto py_handle_clear(PyObject *self) -> int {
    auto *h = handle_cast(self);
    Py_CLEAR(h->script);
    Py_CLEAR(h->scope);
    Py_CLEAR(h->name);
    Py_CLEAR(h->version);
    return 0;
}

auto py_handle_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("Internal object connecting a script to a library.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_handle_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_handle_clear)},
    PyType_Slot{0, nullptr},
};

auto py_handle_spec = PyType_Spec{
    "clingo.script._ScriptHandle",
    sizeof(PyScriptHandleObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_handle_slots.data(),
};

//! Check that the handle is still connected (Python error set otherwise).
auto handle_check(PyScriptHandleObject *h, bool main) -> bool {
    if (main ? h->scope == nullptr : h->script == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "script has been released");
        return false;
    }
    return true;
}

// main python script (all functions require the GIL)

auto main_execute(PyObject *self, char const *code, size_t size) -> bool {
    auto *h = handle_cast(self);
    if (!handle_check(h, true)) {
        return store_python_error("executing python code failed");
    }
    // NOTE: bytes objects are null-terminated as required by Py_CompileString
    auto src = steal(PyBytes_FromStringAndSize(code, static_cast<Py_ssize_t>(size)));
    if (src.get() == nullptr) {
        return store_python_error("executing python code failed");
    }
    auto compiled = steal(Py_CompileString(PyBytes_AsString(src.get()), "<string>", Py_file_input));
    if (compiled.get() == nullptr) {
        return store_python_error("compiling python code failed");
    }
    auto res = steal(PyEval_EvalCode(compiled.get(), h->scope, h->scope));
    if (res.get() == nullptr) {
        return store_python_error("executing python code failed");
    }
    return true;
}

//! Lookup a name in the scope (new reference or nullptr without error if missing).
auto main_lookup(PyScriptHandleObject *h, char const *name, size_t size) -> PyObject * {
    auto key = steal(PyUnicode_FromStringAndSize(name, static_cast<Py_ssize_t>(size)));
    if (key.get() == nullptr) {
        return nullptr;
    }
    auto *value = PyDict_GetItemWithError(h->scope, key.get());
    Py_XINCREF(value);
    return value;
}

auto main_callable(PyObject *self, char const *name, size_t size, bool *result) -> bool {
    auto *h = handle_cast(self);
    if (!handle_check(h, true)) {
        return store_python_error("checking callable failed");
    }
    auto fun = steal(main_lookup(h, name, size));
    if (fun.get() == nullptr && PyErr_Occurred() != nullptr) {
        return store_python_error("checking callable failed");
    }
    *result = fun.get() != nullptr && PyCallable_Check(fun.get()) != 0;
    return true;
}

auto main_call(PyObject *self, clingo_lib_t *lib, char const *name, size_t name_size, clingo_symbol_t const *arguments,
               size_t arguments_size, clingo_symbol_callback_t cb, void *cb_data) -> bool {
    auto *h = handle_cast(self);
    auto *state = state_of(self);
    if (state == nullptr || !handle_check(h, true)) {
        return store_python_error("calling python function failed");
    }
    auto fun = steal(main_lookup(h, name, name_size));
    if (fun.get() == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_Format(PyExc_NameError, "name '%.*s' is not defined", static_cast<int>(name_size), name);
        }
        return store_python_error("calling python function failed");
    }
    auto args = steal(PyTuple_New(static_cast<Py_ssize_t>(arguments_size) + 1));
    if (args.get() == nullptr) {
        return store_python_error("calling python function failed");
    }
    auto *library = py_library_attach(state->lib_type, lib);
    if (library == nullptr || PyTuple_SetItem(args.get(), 0, library) < 0) {
        return store_python_error("calling python function failed");
    }
    for (size_t i = 0; i < arguments_size; ++i) {
        auto *sym = py_symbol_new(state->symbol_type, arguments[i], true);
        if (sym == nullptr || PyTuple_SetItem(args.get(), static_cast<Py_ssize_t>(i + 1), sym) < 0) {
            return store_python_error("calling python function failed");
        }
    }
    auto res = steal(PyObject_Call(fun.get(), args.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("calling python function failed");
    }
    return return_symbols(state, res.get(), cb, cb_data);
}

auto main_main(PyObject *self, clingo_lib_t *lib, clingo_control_t *control) -> bool {
    auto *h = handle_cast(self);
    if (!handle_check(h, true)) {
        return store_python_error("running main failed");
    }
    auto fun = steal(main_lookup(h, "main", 4));
    if (fun.get() == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_NameError, "name 'main' is not defined");
        }
        return store_python_error("running main failed");
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return store_python_error("running main failed");
    }
    auto library = steal(py_library_attach(state->lib_type, lib));
    if (library.get() == nullptr) {
        return store_python_error("creating library failed");
    }
    auto ctl = ControlGuard{state->control_type, library.get(), control, ControlRef::share};
    if (ctl.get() == nullptr) {
        return store_python_error("creating control failed");
    }
    auto res = steal(PyObject_CallFunctionObjArgs(fun.get(), library.get(), ctl.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("running main failed");
    }
    return true;
}

void main_name([[maybe_unused]] void *data, clingo_string_t *name) {
    constexpr auto view = std::string_view{"python"};
    *name = clingo_string_t{view.data(), view.size()};
}

void main_version([[maybe_unused]] void *data, clingo_string_t *version) {
    *version = clingo_string_t{CLINGO_PYTHON_VERSION, std::strlen(CLINGO_PYTHON_VERSION)};
}

// callbacks for the main python script registered from Python

auto main_execute_cb(char const *code, size_t size, void *data) -> bool {
    auto gil = GILGuard{};
    return main_execute(static_cast<PyObject *>(data), code, size);
}

auto main_call_cb(clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc, char const *name, size_t name_size,
                  clingo_symbol_t const *arguments, size_t arguments_size, clingo_symbol_callback_t cb, void *cb_data,
                  void *data) -> bool {
    auto gil = GILGuard{};
    return main_call(static_cast<PyObject *>(data), lib, name, name_size, arguments, arguments_size, cb, cb_data);
}

auto main_callable_cb(char const *name, size_t size, [[maybe_unused]] size_t arguments, bool *result, void *data)
    -> bool {
    // NOTE: python cannot check the number of arguments
    auto gil = GILGuard{};
    return main_callable(static_cast<PyObject *>(data), name, size, result);
}

auto main_main_cb(clingo_lib_t *lib, clingo_control_t *control, void *data) -> bool {
    auto gil = GILGuard{};
    return main_main(static_cast<PyObject *>(data), lib, control);
}

constexpr auto main_script = clingo_script_t{
    main_execute_cb, main_call_cb, main_callable_cb, main_main_cb, main_name, main_version, nullptr,
};

// callbacks for the main python script registered from C

//! Initialize the interpreter if necessary and create the script handle.
auto external_init(ExternalScript *ext) -> bool {
    if (Py_IsInitialized() == 0) {
#ifdef CLINGO_PYTHON_EMBED
        // NOTE: the interpreter is finalized when the script is freed, which
        // happens when the (main) library is released
        Py_InitializeEx(0);
        ext->thread = PyEval_SaveThread();
#else
        return set_clingo_error("the python interpreter is not initialized");
#endif
    }
    if (ext->handle != nullptr) {
        return true;
    }
    auto gil = GILGuard{};
    auto clingo = steal(PyImport_ImportModule("clingo"));
    if (clingo.get() == nullptr) {
        return store_python_error("importing clingo failed");
    }
    auto mod = steal(PyImport_ImportModule("clingo.script"));
    if (mod.get() == nullptr) {
        return store_python_error("importing clingo.script failed");
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return store_python_error("initializing python failed");
    }
    auto *main = PyImport_AddModule("__main__");
    if (main == nullptr) {
        return store_python_error("initializing python failed");
    }
    ext->handle = handle_new(state, nullptr, PyModule_GetDict(main));
    if (ext->handle == nullptr) {
        return store_python_error("initializing python failed");
    }
    return true;
}

auto external_cast(void *data) -> ExternalScript * {
    return static_cast<ExternalScript *>(data);
}

auto external_execute_cb(char const *code, size_t size, void *data) -> bool {
    auto *ext = external_cast(data);
    if (!external_init(ext)) {
        return false;
    }
    auto gil = GILGuard{};
    return main_execute(ext->handle, code, size);
}

auto external_call_cb(clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc, char const *name,
                      size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                      clingo_symbol_callback_t cb, void *cb_data, void *data) -> bool {
    auto *ext = external_cast(data);
    if (ext->handle == nullptr) {
        return set_clingo_error("no python code has been executed");
    }
    auto gil = GILGuard{};
    return main_call(ext->handle, lib, name, name_size, arguments, arguments_size, cb, cb_data);
}

auto external_callable_cb(char const *name, size_t size, [[maybe_unused]] size_t arguments, bool *result, void *data)
    -> bool {
    auto *ext = external_cast(data);
    if (ext->handle == nullptr) {
        *result = false;
        return true;
    }
    auto gil = GILGuard{};
    return main_callable(ext->handle, name, size, result);
}

auto external_main_cb(clingo_lib_t *lib, clingo_control_t *control, void *data) -> bool {
    auto *ext = external_cast(data);
    if (ext->handle == nullptr) {
        return true;
    }
    auto gil = GILGuard{};
    return main_main(ext->handle, lib, control);
}

void external_free(void *data) {
    auto ext = std::unique_ptr<ExternalScript>{external_cast(data)};
    if (ext->handle == nullptr || Py_IsInitialized() == 0) {
        return;
    }
    if (ext->thread != nullptr) {
        PyEval_RestoreThread(ext->thread);
    }
    {
        // NOTE: an attached library object has already been invalidated by
        // the library's finalizer
        auto gil = GILGuard{};
        Py_CLEAR(ext->handle);
    }
    if (ext->thread != nullptr) {
        Py_FinalizeEx();
    }
}

constexpr auto external_script = clingo_script_t{
    external_execute_cb, external_call_cb, external_callable_cb, external_main_cb,
    main_name,           main_version,     external_free,
};

// custom scripts

auto custom_get(PyObject *self, ScriptModuleState **state) -> PyScriptHandleObject * {
    auto *h = handle_cast(self);
    *state = state_of(self);
    if (*state == nullptr || !handle_check(h, false)) {
        return nullptr;
    }
    return h;
}

auto custom_execute_cb(char const *code, size_t size, void *data) -> bool {
    auto gil = GILGuard{};
    ScriptModuleState *state = nullptr;
    auto *h = custom_get(static_cast<PyObject *>(data), &state);
    if (h == nullptr) {
        return store_python_error("script execute failed");
    }
    auto str = steal(PyUnicode_FromStringAndSize(code, static_cast<Py_ssize_t>(size)));
    if (str.get() == nullptr) {
        return store_python_error("script execute failed");
    }
    auto res = steal(PyObject_CallMethodObjArgs(h->script, state->str_execute, str.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("script execute failed");
    }
    return true;
}

auto custom_call_cb(clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc, char const *name,
                    size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                    clingo_symbol_callback_t cb, void *cb_data, void *data) -> bool {
    auto gil = GILGuard{};
    ScriptModuleState *state = nullptr;
    auto *h = custom_get(static_cast<PyObject *>(data), &state);
    if (h == nullptr) {
        return store_python_error("script call failed");
    }
    auto py_name = steal(PyUnicode_FromStringAndSize(name, static_cast<Py_ssize_t>(name_size)));
    if (py_name.get() == nullptr) {
        return store_python_error("script call failed");
    }
    auto args = steal(symbol_list(state, arguments, arguments_size));
    if (args.get() == nullptr) {
        return store_python_error("script call failed");
    }
    auto library = steal(py_library_attach(state->lib_type, lib));
    if (library.get() == nullptr) {
        return store_python_error("script call failed");
    }
    auto res = steal(
        PyObject_CallMethodObjArgs(h->script, state->str_call, library.get(), py_name.get(), args.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("script call failed");
    }
    return return_symbols(state, res.get(), cb, cb_data);
}

auto custom_callable_cb(char const *name, size_t size, size_t arguments, bool *result, void *data) -> bool {
    auto gil = GILGuard{};
    ScriptModuleState *state = nullptr;
    auto *h = custom_get(static_cast<PyObject *>(data), &state);
    if (h == nullptr) {
        return store_python_error("script callable failed");
    }
    auto py_name = steal(PyUnicode_FromStringAndSize(name, static_cast<Py_ssize_t>(size)));
    auto py_args = steal(PyLong_FromSize_t(arguments));
    if (py_name.get() == nullptr || py_args.get() == nullptr) {
        return store_python_error("script callable failed");
    }
    auto res = steal(PyObject_CallMethodObjArgs(h->script, state->str_callable, py_name.get(), py_args.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("script callable failed");
    }
    auto truth = PyObject_IsTrue(res.get());
    if (truth < 0) {
        return store_python_error("script callable failed");
    }
    *result = truth != 0;
    return true;
}

auto custom_main_cb(clingo_lib_t *lib, clingo_control_t *control, void *data) -> bool {
    auto gil = GILGuard{};
    ScriptModuleState *state = nullptr;
    auto *h = custom_get(static_cast<PyObject *>(data), &state);
    if (h == nullptr) {
        return store_python_error("script main failed");
    }
    auto library = steal(py_library_attach(state->lib_type, lib));
    if (library.get() == nullptr) {
        return store_python_error("creating library failed");
    }
    auto ctl = ControlGuard{state->control_type, library.get(), control, ControlRef::share};
    if (ctl.get() == nullptr) {
        return store_python_error("creating control failed");
    }
    auto res = steal(PyObject_CallMethodObjArgs(h->script, state->str_main, library.get(), ctl.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("script main failed");
    }
    return true;
}

//! Get a string from the script and cache it in the handle.
//!
//! The string stays valid as long as the handle is alive.
void custom_string(void *data, PyObject *PyScriptHandleObject::*slot, PyObject *ScriptModuleState::*method,
                   clingo_string_t *out) {
    static constexpr std::string_view error = "<error>";
    auto gil = GILGuard{};
    ScriptModuleState *state = nullptr;
    auto *h = custom_get(static_cast<PyObject *>(data), &state);
    if (h != nullptr && h->*slot == nullptr) {
        auto res = steal(PyObject_CallMethodObjArgs(h->script, state->*method, nullptr));
        if (res.get() != nullptr && PyUnicode_Check(res.get()) == 0) {
            PyErr_SetString(PyExc_TypeError, "script name and version must be strings");
            res = steal(nullptr);
        }
        h->*slot = res.release();
    }
    Py_ssize_t size = 0;
    auto const *str = h != nullptr && h->*slot != nullptr ? PyUnicode_AsUTF8AndSize(h->*slot, &size) : nullptr;
    if (str == nullptr) {
        PyErr_Clear();
        *out = clingo_string_t{error.data(), error.size()};
        return;
    }
    *out = clingo_string_t{str, static_cast<size_t>(size)};
}

void custom_name_cb(void *data, clingo_string_t *name) {
    custom_string(data, &PyScriptHandleObject::name, &ScriptModuleState::str_name, name);
}

void custom_version_cb(void *data, clingo_string_t *version) {
    custom_string(data, &PyScriptHandleObject::version, &ScriptModuleState::str_version, version);
}

constexpr auto custom_script = clingo_script_t{
    custom_execute_cb, custom_call_cb, custom_callable_cb, custom_main_cb, custom_name_cb, custom_version_cb, nullptr,
};

// Script

auto not_implemented(PyObject *self, char const *name) -> PyObject * {
    auto type_name = steal(PyType_GetName(Py_TYPE(self)));
    if (type_name.get() != nullptr) {
        PyErr_Format(PyExc_NotImplementedError, "%U.%s is not implemented", type_name.get(), name);
    }
    return nullptr;
}

auto py_script_execute(PyObject *self, [[maybe_unused]] PyObject *args, [[maybe_unused]] PyObject *kwargs)
    -> PyObject * {
    return not_implemented(self, "execute");
}

auto py_script_call(PyObject *self, [[maybe_unused]] PyObject *args, [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    return not_implemented(self, "call");
}

auto py_script_callable(PyObject *self, [[maybe_unused]] PyObject *args, [[maybe_unused]] PyObject *kwargs)
    -> PyObject * {
    return not_implemented(self, "callable");
}

auto py_script_main(PyObject *self, [[maybe_unused]] PyObject *args, [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    return not_implemented(self, "main");
}

auto py_script_name(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return not_implemented(self, "name");
}

auto py_script_version(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    return not_implemented(self, "version");
}

auto py_script_methods = std::array{
    PyMethodDef{
        "execute",
        py_c_function_cast(py_script_execute),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Execute the given code."),
    },
    PyMethodDef{
        "call",
        py_c_function_cast(py_script_call),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Call the function with the given name and arguments and return a list of symbols."),
    },
    PyMethodDef{
        "callable",
        py_c_function_cast(py_script_callable),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check if a function with the given name and number of arguments is callable."),
    },
    PyMethodDef{
        "main",
        py_c_function_cast(py_script_main),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Run the main function with the given library and control object."),
    },
    PyMethodDef{
        "name",
        py_script_name,
        METH_NOARGS,
        PyDoc_STR("Get the name of the script."),
    },
    PyMethodDef{
        "version",
        py_script_version,
        METH_NOARGS,
        PyDoc_STR("Get the version of the script."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_script_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("ABC for custom scripts.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_methods, py_script_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_script_spec = PyType_Spec{
    "clingo.script.Script", sizeof(PyObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, py_script_slots.data(),
};

// module functions

auto register_handle(PyObject *type, PyObject *lib, PyObject *handle, clingo_script_t const *script) -> PyObject * {
    auto *c_lib = py_library_get(type, lib);
    if (c_lib == nullptr) {
        return nullptr;
    }
    // NOTE: the library keeps the handle alive and owns the reference passed to clingo
    if (!py_library_keep(type, lib, handle)) {
        return nullptr;
    }
    if (!clingo_script_register(c_lib, script, static_cast<void *>(handle))) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_register(PyObject *module, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"lib", "script", nullptr};
    PyObject *lib = nullptr;
    PyObject *script = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO:register", cast_kw(kwlist), &lib, &script) == 0) {
        return nullptr;
    }
    auto *state = get_state(module);
    if (state == nullptr) {
        return nullptr;
    }
    auto is_script = PyObject_IsInstance(script, state->script_type);
    if (is_script < 0) {
        return nullptr;
    }
    if (is_script == 0) {
        PyErr_SetString(PyExc_TypeError, "script must be an instance of clingo.script.Script");
        return nullptr;
    }
    auto handle = steal(handle_new(state, script, nullptr));
    if (handle.get() == nullptr) {
        return nullptr;
    }
    return register_handle(state->lib_type, lib, handle.get(), &custom_script);
}

auto py_enable_python(PyObject *module, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"lib", nullptr};
    PyObject *lib = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:enable_python", cast_kw(kwlist), &lib) == 0) {
        return nullptr;
    }
    auto *state = get_state(module);
    if (state == nullptr) {
        return nullptr;
    }
    auto *main = PyImport_AddModule("__main__");
    if (main == nullptr) {
        return nullptr;
    }
    auto handle = steal(handle_new(state, nullptr, PyModule_GetDict(main)));
    if (handle.get() == nullptr) {
        return nullptr;
    }
    return register_handle(state->lib_type, lib, handle.get(), &main_script);
}

auto script_module_methods = std::array{
    PyMethodDef{
        "register",
        py_c_function_cast(py_register),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Register a script language which can then be embedded into a logic program."),
    },
    PyMethodDef{
        "enable_python",
        py_c_function_cast(py_enable_python),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Enable embedded python scripts."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto script_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<ScriptModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_VISIT(state->*member);
    }
    return 0;
}

auto script_module_clear(PyObject *module) -> int {
    auto *state = static_cast<ScriptModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_CLEAR(state->*member);
    }
    return 0;
}

void script_module_free(void *module) {
    script_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef script_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.script",
    PyDoc_STR("Module to add custom scripts that can be embedded into logic programs."),
    static_cast<Py_ssize_t>(sizeof(ScriptModuleState)),
    script_module_methods.data(),
    nullptr,
    script_module_traverse,
    script_module_clear,
    script_module_free,
};

auto intern_names(ScriptModuleState *state) -> bool {
    auto names = std::array{
        std::pair{&state->str_execute, "execute"},   std::pair{&state->str_call, "call"},
        std::pair{&state->str_callable, "callable"}, std::pair{&state->str_main, "main"},
        std::pair{&state->str_name, "name"},         std::pair{&state->str_version, "version"},
    };
    return std::ranges::all_of(names, [](auto const &pair) { return pair.first != nullptr; });
}

} // namespace

auto register_python(clingo_lib_t *lib) -> bool {
    auto ext = std::unique_ptr<ExternalScript>{new (std::nothrow) ExternalScript{}};
    if (ext == nullptr) {
        return set_clingo_error("out of memory");
    }
    return clingo_script_register(lib, &external_script, static_cast<void *>(ext.release()));
}

void register_script(pybind11::module &m) {
    auto script_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &script_def, &script_mod.get())) {
        throw py::error_already_set();
    }
    auto *state = get_state(script_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }
    state->lib_type = py_library_type();
    if (state->lib_type == nullptr) {
        throw py::error_already_set();
    }
    state->control_type = py_control_type();
    if (state->control_type == nullptr) {
        throw py::error_already_set();
    }
    state->symbol_type = py_symbol_type();
    if (state->symbol_type == nullptr) {
        throw py::error_already_set();
    }
    if (!intern_names(state) || !add_type(script_mod.get(), "Script", &py_script_spec, &state->script_type) ||
        !add_type(script_mod.get(), "_ScriptHandle", &py_handle_spec, &state->handle_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
