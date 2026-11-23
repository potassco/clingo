#define PY_SSIZE_T_CLEAN

#include "core.hh"
#include "collection.hh"
#include "util.hh"

#include <array>
#include <cassert>
#include <cstdio>
#include <tuple>
#include <utility>

namespace PyClingo {

auto py_handle_error() -> std::nullptr_t {
    raise_python_error();
    return nullptr;
}

auto report_main_error(bool res, int code, clingo_string_t name) -> bool {
    // NOLINTNEXTLINE
    if (!res || (code & 65) == 65 || (code & 33) == 33) {
        clingo_string_t str;
        clingo_result_t rc = clingo_result_success;
        clingo_get_error(&rc, &str);
        if (rc != clingo_result_success) {
            fprintf(stderr, "*** ERROR: (%.*s): %.*s\n", static_cast<int>(name.size), name.data,
                    static_cast<int>(str.size), str.data);
        }
        return false;
    }
    return true;
}

namespace {

auto exc_str(PyObject *ptype, PyObject *pvalue, PyObject *ptrace) -> PyObject * {
    auto mod = steal(PyImport_ImportModule("traceback"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto te = steal(PyObject_CallMethod(mod.get(), "TracebackException", "OOO", ptype != nullptr ? ptype : Py_None,
                                        pvalue != nullptr ? pvalue : Py_None, ptrace != nullptr ? ptrace : Py_None));
    if (te.get() == nullptr) {
        return nullptr;
    }

    auto only_it = steal(PyObject_CallMethod(te.get(), "format_exception_only", nullptr));
    if (only_it.get() == nullptr) {
        return nullptr;
    }
    auto only = steal(PySequence_List(only_it.get()));
    if (only.get() == nullptr) {
        return nullptr;
    }

    // Chain and traceback; the final exception line(s) come last.
    auto full_it = steal(PyObject_CallMethod(te.get(), "format", nullptr));
    if (full_it.get() == nullptr) {
        return nullptr;
    }
    auto full = steal(PySequence_List(full_it.get()));
    if (full.get() == nullptr) {
        return nullptr;
    }

    auto n_only = PyList_Size(only.get());
    auto n_full = PyList_Size(full.get());
    if (n_only < 0 || n_full < 0) {
        return nullptr;
    }

    // Drop the trailing exception line(s) from the full output: they are
    // already on top.
    auto rest = steal(PyList_GetSlice(full.get(), 0, n_full > n_only ? n_full - n_only : 0));
    if (rest.get() == nullptr) {
        return nullptr;
    }
    auto all = steal(PySequence_Concat(only.get(), rest.get()));
    if (all.get() == nullptr) {
        return nullptr;
    }

    auto sep = steal(PyUnicode_FromString(""));
    if (sep.get() == nullptr) {
        return nullptr;
    }
    return PyUnicode_Join(sep.get(), all.get());
}

} // namespace

auto store_python_error(std::string_view fallback) -> bool {
    if (PyErr_Occurred() == nullptr) {
        clingo_set_error(clingo_result_runtime, fallback.data(), fallback.size());
        return false;
    }

    auto code = clingo_result_runtime;
    if (PyErr_ExceptionMatches(PyExc_ValueError) != 0) {
        code = clingo_result_invalid;
    } else if (PyErr_ExceptionMatches(PyExc_IndexError) != 0) {
        code = clingo_result_range;
    } else if (PyErr_ExceptionMatches(PyExc_MemoryError) != 0) {
        code = clingo_result_bad_alloc;
    }

    auto ptype = PyGuard{nullptr};
    auto pvalue = PyGuard{nullptr};
    auto ptrace = PyGuard{nullptr};
    PyErr_Fetch(&ptype.get(), &pvalue.get(), &ptrace.get());
    PyErr_NormalizeException(&ptype.get(), &pvalue.get(), &ptrace.get());

    auto str = steal(exc_str(ptype.get(), pvalue.get(), ptrace.get()));
    std::string_view msg = fallback;
    if (str.get() != nullptr) {
        Py_ssize_t n = 0;
        auto const *u = PyUnicode_AsUTF8AndSize(str.get(), &n);
        if (u != nullptr) {
            msg = {u, static_cast<size_t>(n)};
        } else {
            PyErr_Clear();
        }
    } else {
        PyErr_Clear();
    }
    return clingo_set_error(code, msg.data(), msg.size());
}

void raise_python_error() {
    clingo_result_t code = clingo_result_success;
    clingo_string_t msg;
    clingo_get_error(&code, &msg);
    PyObject *exc_type = PyExc_RuntimeError;
    switch (code) {
        case clingo_result_invalid: {
            exc_type = PyExc_ValueError;
            break;
        }
        case clingo_result_range: {
            exc_type = PyExc_IndexError;
            break;
        }
        case clingo_result_bad_alloc: {
            exc_type = PyExc_MemoryError;
            break;
        }
        default: {
            break;
        }
    }

    auto s = steal(PyUnicode_FromStringAndSize(msg.data, static_cast<Py_ssize_t>(msg.size)));
    if (s.get() != nullptr) {
        PyErr_SetObject(exc_type, s.get());
    } else {
        PyErr_SetString(exc_type, "no message");
    }
}

namespace {

// The core module wraps clingo libraries, positions, and locations.
//
// Each clingo library is represented by at most one library object, which is
// registered as its owner. Library objects created from Python own their
// clingo library. Non-owning library objects are created for clingo libraries
// passed to Python (see py_library_attach()); the clingo library keeps them
// alive and invalidates them via a finalizer when it is released.
//
// The logger of a library is stored together with the message type enum in a
// tuple, whose reference is owned by the clingo library and released via the
// free function of the clingo logger.

struct CoreModuleState {
    PyObject *log_level_enum{nullptr};
    PyObject *message_type_enum{nullptr};
    PyObject *library_type{nullptr};
    PyObject *position_type{nullptr};
    PyObject *location_type{nullptr};
    PyObject *keys_view_type{nullptr};
    PyObject *values_view_type{nullptr};
    PyObject *items_view_type{nullptr};
    //! The collections.abc.Set class (used by the views).
    PyObject *set_abc{nullptr};
    //! The collections.abc.Mapping class (used by mappings).
    PyObject *mapping_abc{nullptr};
    //! The address of this member is the tag under which library objects are registered as owners.
    char owner_tag{0};
};

constexpr auto state_members = std::array{
    &CoreModuleState::log_level_enum,   &CoreModuleState::message_type_enum, &CoreModuleState::library_type,
    &CoreModuleState::position_type,    &CoreModuleState::location_type,     &CoreModuleState::keys_view_type,
    &CoreModuleState::values_view_type, &CoreModuleState::items_view_type,   &CoreModuleState::set_abc,
    &CoreModuleState::mapping_abc,
};

struct PyLibraryObject {
    PyObject_HEAD
    clingo_lib_t *lib;
    //! The logger data owned by the clingo library (borrowed, nullptr if there is none).
    PyObject *logger;
    //! Objects that have to live as long as the library object (e.g., scripts).
    PyObject *keep;
    //! Whether the object holds a reference to its clingo library.
    bool owns;
};

struct PyPositionObject {
    PyObject_HEAD
    clingo_position_t const *pos;
};

struct PyLocationObject {
    PyObject_HEAD
    clingo_location_t const *loc;
};

constexpr size_t default_message_limit = 25;

// casts and module state

auto library_cast(PyObject *obj) -> PyLibraryObject * {
    return reinterpret_cast<PyLibraryObject *>(obj);
}

auto position_cast(PyObject *obj) -> PyPositionObject * {
    return reinterpret_cast<PyPositionObject *>(obj);
}

auto location_cast(PyObject *obj) -> PyLocationObject * {
    return reinterpret_cast<PyLocationObject *>(obj);
}

auto get_state(PyObject *module) -> CoreModuleState * {
    auto *state = static_cast<CoreModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "core module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_type(PyTypeObject *type) -> CoreModuleState * {
    auto *state = static_cast<CoreModuleState *>(PyType_GetModuleState(type));
    if (state == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "core module state is unavailable");
        }
        return nullptr;
    }
    return state;
}

auto state_of(PyObject *self) -> CoreModuleState * {
    return get_state_from_type(Py_TYPE(self));
}

auto alloc_object(PyObject *type) -> PyObject * {
    return PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(type), 0);
}

auto check_type(PyObject *type, PyObject *obj, char const *msg) -> bool {
    if (PyObject_TypeCheck(obj, reinterpret_cast<PyTypeObject *>(type)) == 0) {
        PyErr_SetString(PyExc_TypeError, msg);
        return false;
    }
    return true;
}

auto import_core_state() -> CoreModuleState * {
    auto mod = steal(PyImport_ImportModule("clingo.core"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    // NOTE: the state stays alive because the module is kept in sys.modules
    return state;
}

auto get_type(PyObject *CoreModuleState::*member) -> PyObject * {
    auto *state = import_core_state();
    if (state == nullptr) {
        return nullptr;
    }
    auto *type = state->*member;
    if (type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.core module is not initialized");
        return nullptr;
    }
    return Py_NewRef(type);
}

using BuilderGuard = Guard<clingo_string_builder_t *, clingo_string_builder_free>;

//! Convert an object into a Python string using the given to_string function.
template <class T> auto to_str(T obj, bool (*to_string)(T, clingo_string_builder_t *)) -> PyObject * {
    auto bld = BuilderGuard{nullptr};
    if (!clingo_string_builder_new(&bld.get()) || !to_string(obj, bld.get())) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld.get(), &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

// logger

void logger_log(clingo_message_t code, char const *message, size_t size, void *data) {
    if (Py_IsInitialized() == 0) {
        fprintf(stderr, "%.*s\n", static_cast<int>(size), message);
        return;
    }
    auto gil = GILGuard{};
    // NOTE: messages might be reported while an exception is pending
    PyObject *type = nullptr;
    PyObject *value = nullptr;
    PyObject *trace = nullptr;
    PyErr_Fetch(&type, &value, &trace);
    auto *logger = PyTuple_GetItem(static_cast<PyObject *>(data), 0);
    auto *message_type = PyTuple_GetItem(static_cast<PyObject *>(data), 1);
    auto py_code = steal(PyObject_CallFunction(message_type, "i", code));
    auto py_msg = steal(
        py_code.get() != nullptr ? PyUnicode_DecodeUTF8(message, static_cast<Py_ssize_t>(size), "replace") : nullptr);
    auto res = steal(
        py_msg.get() != nullptr ? PyObject_CallFunctionObjArgs(logger, py_code.get(), py_msg.get(), nullptr) : nullptr);
    if (res.get() == nullptr) {
        PyErr_WriteUnraisable(logger);
    }
    PyErr_Restore(type, value, trace);
}

void logger_free(void *data) {
    // NOTE: objects cannot be released anymore once the interpreter is gone
    if (Py_IsInitialized() == 0) {
        return;
    }
    auto gil = GILGuard{};
    Py_DECREF(static_cast<PyObject *>(data));
}

constexpr auto c_logger = clingo_logger_t{logger_log, logger_free};

// library ownership

auto library_owner(CoreModuleState *state, clingo_lib_t *lib) -> PyObject * {
    return static_cast<PyObject *>(clingo_lib_get_owner(lib, &state->owner_tag));
}

//! Detach a library object from its clingo library.
//!
//! Removes the owner registration and releases the clingo library if the
//! object owns it.
void library_invalidate(PyObject *self) noexcept {
    auto *l = library_cast(self);
    auto *lib = std::exchange(l->lib, nullptr);
    l->logger = nullptr;
    if (lib == nullptr) {
        return;
    }
    auto *state = static_cast<CoreModuleState *>(PyType_GetModuleState(Py_TYPE(self)));
    assert(state != nullptr);
    if (library_owner(state, lib) == self) {
        // NOTE: removing an owner does not allocate and cannot fail
        std::ignore = clingo_lib_set_owner(lib, &state->owner_tag, nullptr);
    }
    if (l->owns) {
        clingo_lib_release(lib);
    }
}

//! Create a library object and register it as the owner of the clingo library.
//!
//! If owns is true, the reference to the clingo library is taken over (also
//! on failure).
auto library_new(CoreModuleState *state, clingo_lib_t *lib, PyObject *logger, bool owns) -> PyObject * {
    auto obj = steal(alloc_object(state->library_type));
    if (obj.get() == nullptr) {
        if (owns) {
            clingo_lib_release(lib);
        }
        return nullptr;
    }
    auto *l = library_cast(obj.get());
    l->lib = lib;
    l->logger = logger;
    l->owns = owns;
    l->keep = PyList_New(0);
    if (l->keep == nullptr) {
        return nullptr;
    }
    if (!clingo_lib_set_owner(lib, &state->owner_tag, obj.get())) {
        return py_handle_error();
    }
    return obj.release();
}

//! Invalidate a non-owning library object and release the reference held by its clingo library.
void library_finalizer(void *data) {
    // NOTE: objects cannot be released anymore once the interpreter is gone
    if (Py_IsInitialized() == 0) {
        return;
    }
    auto gil = GILGuard{};
    auto *obj = static_cast<PyObject *>(data);
    library_invalidate(obj);
    Py_DECREF(obj);
}

auto library_get(PyObject *self) -> clingo_lib_t * {
    auto *lib = library_cast(self)->lib;
    if (lib == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "library is no longer valid");
    }
    return lib;
}

// Library

auto py_library_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 7>{
        "shared", "slotted", "fast_release", "log_level", "logger", "message_limit", nullptr,
    };
    int shared = 1;
    int slotted = 1;
    int fast_release = 0;
    PyObject *level_obj = nullptr;
    PyObject *logger_obj = nullptr;
    PyObject *limit_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|pppOOO:Library", cast_kw(kwlist), &shared, &slotted, &fast_release,
                                    &level_obj, &logger_obj, &limit_obj) == 0) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    auto level = clingo_log_level_t{clingo_log_level_info};
    auto limit = default_message_limit;
    if (!parse_num(level_obj, &level, "log_level") || !parse_num(limit_obj, &limit, "message_limit")) {
        return nullptr;
    }
    if (logger_obj == Py_None) {
        logger_obj = nullptr;
    }
    if (logger_obj != nullptr && PyCallable_Check(logger_obj) == 0) {
        PyErr_SetString(PyExc_TypeError, "logger must be callable or None");
        return nullptr;
    }
    auto data = steal(logger_obj != nullptr ? PyTuple_Pack(2, logger_obj, state->message_type_enum) : nullptr);
    if (logger_obj != nullptr && data.get() == nullptr) {
        return nullptr;
    }
    auto flags = clingo_lib_flags_t{0};
    if (shared != 0) {
        flags |= clingo_lib_flags_shared;
    }
    if (slotted != 0) {
        flags |= clingo_lib_flags_slotted;
    }
    if (fast_release != 0) {
        flags |= clingo_lib_flags_fast_release;
    }
    // NOTE: the clingo library takes over the reference to the logger data
    // (also on failure) and releases it via logger_free()
    auto *logger_data = data.release();
    clingo_lib_t *lib = nullptr;
    if (!clingo_lib_new(flags, level, logger_data != nullptr ? &c_logger : nullptr, logger_data, limit, &lib)) {
        return py_handle_error();
    }
    return library_new(state, lib, logger_data, true);
}

auto py_library_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *l = library_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(l->keep);
    if (l->owns && l->lib != nullptr) {
        // NOTE: the logger is reachable via the owned clingo library
        Py_VISIT(l->logger);
    }
    return 0;
}

auto py_library_clear(PyObject *self) -> int {
    Py_CLEAR(library_cast(self)->keep);
    return 0;
}

void py_library_dealloc(PyObject *self) {
    // NOTE: the clingo library is released after the kept objects (see core.hh)
    PyObject_GC_UnTrack(self);
    py_library_clear(self);
    library_invalidate(self);
    auto *type = Py_TYPE(self);
    reinterpret_cast<freefunc>(PyType_GetSlot(type, Py_tp_free))(self);
    Py_DECREF(type);
}

auto py_library_capsule(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *lib = library_get(self);
    if (lib == nullptr) {
        return nullptr;
    }
    return PyCapsule_New(lib, "clingo_lib_t", nullptr);
}

auto py_library_enter(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    return Py_NewRef(self);
}

auto py_library_exit(PyObject *self, PyObject *const *args, Py_ssize_t nargs) -> PyObject * {
    PyObject *type = unset_arg();
    PyObject *value = unset_arg();
    PyObject *traceback = unset_arg();
    if (!parse_args("__exit__", {"type", "value", "traceback"}, {}, args, nargs, {&type, &value, &traceback})) {
        return nullptr;
    }
    library_invalidate(self);
    Py_RETURN_FALSE;
}

auto py_library_methods = std::array{
    PyMethodDef{
        "_capsule",
        py_library_capsule,
        METH_NOARGS,
        PyDoc_STR("Get a capsule holding the underlying C library object."),
    },
    PyMethodDef{"__enter__", py_library_enter, METH_NOARGS, PyDoc_STR("Return the library object.")},
    PyMethodDef{"__exit__", method_cast(py_library_exit), METH_FASTCALL, PyDoc_STR("Close the library object.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_library_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A library object managing clingo's core resources.")}, // NOLINT
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(py_library_tp_new)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_library_dealloc)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_library_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_library_clear)},
    PyType_Slot{Py_tp_methods, py_library_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_library_spec = PyType_Spec{
    "clingo.core.Library", sizeof(PyLibraryObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, py_library_slots.data(),
};

// Position

//! Create a position object taking over the given position (also on failure).
auto position_new(PyObject *type, clingo_position_t const *pos) -> PyObject * {
    auto *obj = alloc_object(type);
    if (obj == nullptr) {
        clingo_position_free(pos);
        return nullptr;
    }
    position_cast(obj)->pos = pos;
    return obj;
}

auto py_position_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 5>{"lib", "file", "line", "column", nullptr};
    PyObject *lib_obj = nullptr;
    PyObject *file_obj = nullptr;
    PyObject *line_obj = nullptr;
    PyObject *column_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OUOO:Position", cast_kw(kwlist), &lib_obj, &file_obj, &line_obj,
                                    &column_obj) == 0) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    size_t line = 0;
    size_t column = 0;
    if (!parse_num(line_obj, &line, "line") || !parse_num(column_obj, &column, "column")) {
        return nullptr;
    }
    Py_ssize_t size = 0;
    auto const *file = PyUnicode_AsUTF8AndSize(file_obj, &size);
    if (file == nullptr) {
        return nullptr;
    }
    auto *lib = py_library_get(state->library_type, lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }
    clingo_position_t const *pos = nullptr;
    if (!clingo_position_new(lib, file, static_cast<size_t>(size), line, column, &pos)) {
        return py_handle_error();
    }
    return position_new(reinterpret_cast<PyObject *>(type), pos);
}

void py_position_dealloc(PyObject *self) {
    if (auto const *pos = position_cast(self)->pos; pos != nullptr) {
        clingo_position_free(pos);
    }
    free_object(self);
}

auto py_position_file(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    clingo_string_t file;
    clingo_position_file(position_cast(self)->pos, &file);
    return PyUnicode_FromStringAndSize(file.data, static_cast<Py_ssize_t>(file.size));
}

auto py_position_line(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return PyLong_FromSize_t(clingo_position_line(position_cast(self)->pos));
}

auto py_position_column(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return PyLong_FromSize_t(clingo_position_column(position_cast(self)->pos));
}

auto py_position_str(PyObject *self) -> PyObject * {
    return to_str(position_cast(self)->pos, clingo_position_to_string);
}

auto py_position_repr(PyObject *self) -> PyObject * {
    auto file = steal(py_position_file(self, nullptr));
    if (file.get() == nullptr) {
        return nullptr;
    }
    auto const *pos = position_cast(self)->pos;
    return PyUnicode_FromFormat("Position(%R,%zu,%zu)", file.get(), clingo_position_line(pos),
                                clingo_position_column(pos));
}

auto py_position_hash(PyObject *self) -> Py_hash_t {
    return py_hash(clingo_position_hash(position_cast(self)->pos));
}

auto py_position_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (PyObject_TypeCheck(b, Py_TYPE(a)) == 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto const *pa = position_cast(a)->pos;
    auto const *pb = position_cast(b)->pos;
    if (op == Py_EQ || op == Py_NE) {
        return PyBool_FromLong(clingo_position_equal(pa, pb) == (op == Py_EQ) ? 1 : 0);
    }
    Py_RETURN_RICHCOMPARE(clingo_position_compare(pa, pb), 0, op);
}

auto py_position_getset = std::array{
    PyGetSetDef{"file", py_position_file, nullptr, PyDoc_STR("The file name."), nullptr},
    PyGetSetDef{"line", py_position_line, nullptr, PyDoc_STR("The line number."), nullptr},
    PyGetSetDef{"column", py_position_column, nullptr, PyDoc_STR("The column number."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_position_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A position in a source file.")}, // NOLINT
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(py_position_tp_new)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_position_dealloc)},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_position_str)},
    PyType_Slot{Py_tp_repr, reinterpret_cast<void *>(py_position_repr)},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_position_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_position_richcompare)},
    PyType_Slot{Py_tp_getset, py_position_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_position_spec = PyType_Spec{
    "clingo.core.Position", sizeof(PyPositionObject), 0, Py_TPFLAGS_DEFAULT, py_position_slots.data(),
};

// Location

//! Create a location object taking over the given location (also on failure).
auto location_new(PyObject *type, clingo_location_t const *loc) -> PyObject * {
    auto *obj = alloc_object(type);
    if (obj == nullptr) {
        clingo_location_free(loc);
        return nullptr;
    }
    location_cast(obj)->loc = loc;
    return obj;
}

auto py_location_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"begin", "end", nullptr};
    PyObject *begin = nullptr;
    PyObject *end = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO:Location", cast_kw(kwlist), &begin, &end) == 0) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    static constexpr char const *msg = "expected clingo.core.Position";
    if (!check_type(state->position_type, begin, msg) || !check_type(state->position_type, end, msg)) {
        return nullptr;
    }
    clingo_location_t const *loc = nullptr;
    if (!clingo_location_new(position_cast(begin)->pos, position_cast(end)->pos, &loc)) {
        return py_handle_error();
    }
    return location_new(reinterpret_cast<PyObject *>(type), loc);
}

void py_location_dealloc(PyObject *self) {
    if (auto const *loc = location_cast(self)->loc; loc != nullptr) {
        clingo_location_free(loc);
    }
    free_object(self);
}

//! Create a position object holding a copy of the given position.
auto position_copy(PyObject *self, clingo_position_t const *pos) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_position_t const *res = nullptr;
    if (!clingo_position_copy(pos, &res)) {
        return py_handle_error();
    }
    return position_new(state->position_type, res);
}

auto py_location_begin(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return position_copy(self, clingo_location_begin(location_cast(self)->loc));
}

auto py_location_end(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    return position_copy(self, clingo_location_end(location_cast(self)->loc));
}

auto py_location_str(PyObject *self) -> PyObject * {
    return to_str(location_cast(self)->loc, clingo_location_to_string);
}

auto py_location_repr(PyObject *self) -> PyObject * {
    auto begin = steal(py_location_begin(self, nullptr));
    auto end = steal(begin.get() != nullptr ? py_location_end(self, nullptr) : nullptr);
    if (end.get() == nullptr) {
        return nullptr;
    }
    return PyUnicode_FromFormat("Location(%R,%R)", begin.get(), end.get());
}

auto py_location_hash(PyObject *self) -> Py_hash_t {
    return py_hash(clingo_location_hash(location_cast(self)->loc));
}

auto py_location_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (PyObject_TypeCheck(b, Py_TYPE(a)) == 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto const *la = location_cast(a)->loc;
    auto const *lb = location_cast(b)->loc;
    if (op == Py_EQ || op == Py_NE) {
        return PyBool_FromLong(clingo_location_equal(la, lb) == (op == Py_EQ) ? 1 : 0);
    }
    Py_RETURN_RICHCOMPARE(clingo_location_compare(la, lb), 0, op);
}

auto py_location_getset = std::array{
    PyGetSetDef{"begin", py_location_begin, nullptr, PyDoc_STR("The beginning of the location."), nullptr},
    PyGetSetDef{"end", py_location_end, nullptr, PyDoc_STR("The end of the location."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_location_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A range of positions in a source file.")}, // NOLINT
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(py_location_tp_new)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_location_dealloc)},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_location_str)},
    PyType_Slot{Py_tp_repr, reinterpret_cast<void *>(py_location_repr)},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_location_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_location_richcompare)},
    PyType_Slot{Py_tp_getset, py_location_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_location_spec = PyType_Spec{
    "clingo.core.Location", sizeof(PyLocationObject), 0, Py_TPFLAGS_DEFAULT, py_location_slots.data(),
};

// module

auto py_version([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    int major = 0;
    int minor = 0;
    int patch = 0;
    clingo_version(&major, &minor, &patch);
    return Py_BuildValue("(iii)", major, minor, patch);
}

auto core_methods = std::array{
    PyMethodDef{
        "version",
        py_version,
        METH_NOARGS,
        PyDoc_STR("Get clingo's version as a (major, minor, revision) tuple."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

constexpr auto log_level_members = std::array<IntEnumMember, 5>{
    IntEnumMember{"Trace", static_cast<int>(clingo_log_level_trace)},
    IntEnumMember{"Debug", static_cast<int>(clingo_log_level_debug)},
    IntEnumMember{"Info", static_cast<int>(clingo_log_level_info)},
    IntEnumMember{"Warn", static_cast<int>(clingo_log_level_warn)},
    IntEnumMember{"Error", static_cast<int>(clingo_log_level_error)},
};

constexpr auto message_type_members = std::array<IntEnumMember, 9>{
    IntEnumMember{"Trace", static_cast<int>(clingo_message_trace)},
    IntEnumMember{"Debug", static_cast<int>(clingo_message_debug)},
    IntEnumMember{"Info", static_cast<int>(clingo_message_info)},
    IntEnumMember{"OperationUndefined", static_cast<int>(clingo_message_operation_undefined)},
    IntEnumMember{"AtomUndefined", static_cast<int>(clingo_message_atom_undefined)},
    IntEnumMember{"FileIncluded", static_cast<int>(clingo_message_file_included)},
    IntEnumMember{"GlobalVariable", static_cast<int>(clingo_message_global_variable)},
    IntEnumMember{"Warn", static_cast<int>(clingo_message_warn)},
    IntEnumMember{"Error", static_cast<int>(clingo_message_error)},
};

auto core_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<CoreModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_VISIT(state->*member);
    }
    return 0;
}

auto core_module_clear(PyObject *module) -> int {
    auto *state = static_cast<CoreModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_CLEAR(state->*member);
    }
    return 0;
}

void core_module_free(void *module) {
    core_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef core_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.core",
    PyDoc_STR("Core functionality used throughout the clingo package."),
    static_cast<Py_ssize_t>(sizeof(CoreModuleState)),
    core_methods.data(),
    nullptr,
    core_module_traverse,
    core_module_clear,
    core_module_free,
};

} // namespace

auto py_library_type() -> PyObject * {
    return get_type(&CoreModuleState::library_type);
}

auto py_library_get(PyObject *type, PyObject *obj) -> clingo_lib_t * {
    if (!check_type(type, obj, "expected clingo.core.Library")) {
        return nullptr;
    }
    return library_get(obj);
}

auto py_library_attach(PyObject *type, clingo_lib_t *lib) -> PyObject * {
    auto *state = get_state_from_type(reinterpret_cast<PyTypeObject *>(type));
    if (state == nullptr) {
        return nullptr;
    }
    if (auto *owner = library_owner(state, lib); owner != nullptr) {
        return Py_NewRef(owner);
    }
    auto obj = steal(library_new(state, lib, nullptr, false));
    if (obj.get() == nullptr) {
        return nullptr;
    }
    // the clingo library keeps the non-owning object alive; if this fails,
    // destroying the object removes its registration
    if (!clingo_lib_add_finalizer(lib, library_finalizer, obj.get())) {
        return py_handle_error();
    }
    Py_INCREF(obj.get());
    return obj.release();
}

auto py_library_keep(PyObject *type, PyObject *library, PyObject *obj) -> bool {
    if (!check_type(type, library, "expected clingo.core.Library")) {
        return false;
    }
    auto *keep = library_cast(library)->keep;
    if (keep == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "library is no longer valid");
        return false;
    }
    return PyList_Append(keep, obj) == 0;
}

auto py_location_type() -> PyObject * {
    return get_type(&CoreModuleState::location_type);
}

auto py_location_new(PyObject *type, clingo_location_t const *loc) -> PyObject * {
    clingo_location_t const *res = nullptr;
    if (!clingo_location_copy(loc, &res)) {
        return py_handle_error();
    }
    return location_new(type, res);
}

auto py_location_get(PyObject *type, PyObject *obj) -> clingo_location_t const * {
    if (!check_type(type, obj, "expected clingo.core.Location")) {
        return nullptr;
    }
    return location_cast(obj)->loc;
}

auto create_core(PyObject *parent) -> PyObject * {
    auto core_mod = PyGuard{nullptr};
    if (!add_submodule(parent, &core_def, &core_mod.get())) {
        return nullptr;
    }
    auto *state = get_state(core_mod.get());
    if (state == nullptr || !add_int_enum(core_mod.get(), "LogLevel", log_level_members, &state->log_level_enum, {}) ||
        !add_int_enum(core_mod.get(), "MessageType", message_type_members, &state->message_type_enum, {}) ||
        !add_type(core_mod.get(), "Library", &py_library_spec, &state->library_type) ||
        !add_type(core_mod.get(), "Position", &py_position_spec, &state->position_type) ||
        !add_type(core_mod.get(), "Location", &py_location_spec, &state->location_type) ||
        !add_view_types(core_mod.get(), &state->keys_view_type, &state->values_view_type, &state->items_view_type)) {
        return nullptr;
    }
    auto abc = steal(PyImport_ImportModule("collections.abc"));
    if (abc.get() == nullptr) {
        return nullptr;
    }
    state->set_abc = PyObject_GetAttrString(abc.get(), "Set");
    state->mapping_abc = state->set_abc != nullptr ? PyObject_GetAttrString(abc.get(), "Mapping") : nullptr;
    if (state->mapping_abc == nullptr) {
        return nullptr;
    }
    return core_mod.release();
}

auto py_keys_view_type() -> PyObject * {
    return get_type(&CoreModuleState::keys_view_type);
}

auto py_values_view_type() -> PyObject * {
    return get_type(&CoreModuleState::values_view_type);
}

auto py_items_view_type() -> PyObject * {
    return get_type(&CoreModuleState::items_view_type);
}

auto py_set_abc(PyTypeObject *type) -> PyObject * {
    auto *state = get_state_from_type(type);
    return state != nullptr ? state->set_abc : nullptr;
}

auto py_mapping_abc(PyTypeObject *type) -> PyObject * {
    auto *state = get_state_from_type(type);
    return state != nullptr ? state->mapping_abc : nullptr;
}

} // namespace PyClingo
