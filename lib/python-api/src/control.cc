#define PY_SSIZE_T_CLEAN

#include "control.hh"

#include "ast.hh"
#include "backend.hh"
#include "base.hh"
#include "collection.hh"
#include "config.hh"
#include "core.hh"
#include "ground.hh"
#include "propagate.hh"
#include "solve.hh"
#include "stats.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/ast.h>
#include <clingo/base.h>
#include <clingo/config.h>
#include <clingo/observe.h>
#include <clingo/profile.h>
#include <clingo/stats.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <tuple>
#include <utility>

namespace PyClingo {

namespace {

// The control module wraps clingo control objects.
//
// Each clingo control object is represented by at most one control object,
// which is registered as its owner so that callbacks receive the same object.
// Owned control objects are created from Python or claimed for a callback
// (see py_control_begin()) and hold a reference to their clingo control object.
// Non-owned control objects are created for clingo control objects shared with
// a callback; the clingo control object keeps them alive and invalidates them
// via a finalizer when it is destroyed.
//
// Objects registered with the control object (e.g., propagators) are kept in a
// list, which lives as long as the Python object. Together with the library,
// the list is visible to the garbage collector.

struct ControlModuleState {
    PyObject *lib_type{nullptr};
    PyObject *symbol_type{nullptr};
    PyObject *base_type{nullptr};
    PyObject *config_type{nullptr};
    PyObject *stats_type{nullptr};
    PyObject *ground_handle_type{nullptr};
    PyObject *solve_handle_type{nullptr};
    PyObject *backend_manager_type{nullptr};
    PyObject *propagator_type{nullptr};
    PyObject *program_type{nullptr};
    PyObject *backend_module{nullptr};
    PyObject *mode_enum{nullptr};
    PyObject *const_map_type{nullptr};
    PyObject *const_map_iter_type{nullptr};
    PyObject *control_type{nullptr};
    PyObject *keys_view_type{nullptr};
    PyObject *values_view_type{nullptr};
    PyObject *items_view_type{nullptr};
    //! The address of this member is the tag under which control objects are registered as owners.
    char owner_tag{0};
};

constexpr auto state_members = std::array{
    &ControlModuleState::lib_type,          &ControlModuleState::symbol_type,
    &ControlModuleState::base_type,         &ControlModuleState::config_type,
    &ControlModuleState::stats_type,        &ControlModuleState::ground_handle_type,
    &ControlModuleState::solve_handle_type, &ControlModuleState::backend_manager_type,
    &ControlModuleState::propagator_type,   &ControlModuleState::program_type,
    &ControlModuleState::backend_module,    &ControlModuleState::mode_enum,
    &ControlModuleState::const_map_type,    &ControlModuleState::const_map_iter_type,
    &ControlModuleState::control_type,      &ControlModuleState::keys_view_type,
    &ControlModuleState::values_view_type,  &ControlModuleState::items_view_type,
};

struct PyControlObject {
    PyObject_HEAD
    clingo_control_t *ctl;
    //! The library object, which has to outlive the clingo control object.
    PyObject *library;
    //! Objects that have to live as long as the clingo control object (e.g., propagators).
    PyObject *keep;
    //! Whether the object holds a reference to its clingo control object.
    bool owns;
};

struct PyConstMapObject {
    PyObject_HEAD
    PyObject *owner;
    void const *valid;
    clingo_const_map_t const *map;
};

enum class IterType : uint8_t { keys, values, items };

struct PyConstMapIterObject {
    PyObject_HEAD
    PyObject *map;
    size_t index;
    IterType type;
};

// casts and module state

auto control_cast(PyObject *obj) -> PyControlObject * {
    return reinterpret_cast<PyControlObject *>(obj);
}

auto const_map_cast(PyObject *obj) -> PyConstMapObject * {
    return reinterpret_cast<PyConstMapObject *>(obj);
}

auto const_map_iter_cast(PyObject *obj) -> PyConstMapIterObject * {
    return reinterpret_cast<PyConstMapIterObject *>(obj);
}

auto get_state(PyObject *module) -> ControlModuleState * {
    auto *state = static_cast<ControlModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "control module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_type(PyTypeObject *type) -> ControlModuleState * {
    auto *state = static_cast<ControlModuleState *>(PyType_GetModuleState(type));
    if (state == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "control module state is unavailable");
        }
        return nullptr;
    }
    return state;
}

auto state_of(PyObject *self) -> ControlModuleState * {
    return get_state_from_type(Py_TYPE(self));
}

//! Get the view type of the given kind for a mapping of this module (borrowed reference).
auto view_type(PyObject *mapping, ViewKind kind) -> PyObject * {
    auto *state = state_of(mapping);
    if (state == nullptr) {
        return nullptr;
    }
    switch (kind) {
        case ViewKind::keys: {
            return state->keys_view_type;
        }
        case ViewKind::values: {
            return state->values_view_type;
        }
        case ViewKind::items: {
            break;
        }
    }
    return state->items_view_type;
}

auto alloc_object(PyObject *type) -> PyObject * {
    return PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(type), 0);
}

// ownership

auto owner_tag(ControlModuleState *state) -> void const * {
    return &state->owner_tag;
}

auto control_owner(ControlModuleState *state, clingo_control_t *ctl) -> PyObject * {
    return static_cast<PyObject *>(clingo_control_get_owner(ctl, owner_tag(state)));
}

//! Detach a control object from its clingo control object.
//!
//! Removes the owner registration and releases the clingo control object if
//! the object owns it.
void control_invalidate(PyObject *self) noexcept {
    auto *c = control_cast(self);
    auto *ctl = std::exchange(c->ctl, nullptr);
    if (ctl == nullptr) {
        return;
    }
    auto *state = static_cast<ControlModuleState *>(PyType_GetModuleState(Py_TYPE(self)));
    assert(state != nullptr);
    if (control_owner(state, ctl) == self) {
        // NOTE: removing an owner does not allocate and cannot fail
        std::ignore = clingo_control_set_owner(ctl, owner_tag(state), nullptr);
    }
    if (c->owns) {
        clingo_control_release(ctl);
    }
}

//! Create a control object and register it as the owner of the clingo control object.
//!
//! If owns is true, the reference to the clingo control object is taken over
//! (also on failure).
auto control_new(ControlModuleState *state, PyObject *library, clingo_control_t *ctl, bool owns) -> PyObject * {
    auto obj = steal(alloc_object(state->control_type));
    if (obj.get() == nullptr) {
        if (owns) {
            clingo_control_release(ctl);
        }
        return nullptr;
    }
    auto *c = control_cast(obj.get());
    c->ctl = ctl;
    c->owns = owns;
    Py_INCREF(library);
    c->library = library;
    c->keep = PyList_New(0);
    if (c->keep == nullptr) {
        return nullptr;
    }
    if (!clingo_control_set_owner(ctl, owner_tag(state), obj.get())) {
        return py_handle_error();
    }
    return obj.release();
}

void control_finalizer(void *data) {
    if (Py_IsInitialized() == 0) {
        fprintf(stderr, "warning: control finalizer called after interpreter shutdown\n");
        return;
    }
    auto gil = GILGuard{};
    auto *obj = static_cast<PyObject *>(data);
    control_invalidate(obj);
    Py_DECREF(obj);
}

auto control_get(PyObject *self) -> clingo_control_t * {
    auto *ctl = control_cast(self)->ctl;
    if (ctl == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "control is no longer valid");
    }
    return ctl;
}

// program parts

struct PartBuffer {
    PyGuard keep{nullptr};
    PyBuffer<clingo_part_t> parts;
    PyBuffer<clingo_symbol_t> symbols;
};

auto parse_parts(ControlModuleState *state, PyObject *obj, PartBuffer &out) -> bool {
    static constexpr char const *msg = "parts must be a sequence of pairs of names and symbol sequences";
    if (PyUnicode_Check(obj) != 0) {
        PyErr_SetString(PyExc_TypeError, msg);
        return false;
    }
    auto seq = steal(PySequence_Tuple(obj));
    if (seq.get() == nullptr) {
        return false;
    }
    auto n = PyTuple_Size(seq.get());
    if (n < 0) {
        return false;
    }
    out.keep = steal(PyTuple_New(2 * n));
    if (out.keep.get() == nullptr || !out.parts.alloc(static_cast<size_t>(n))) {
        return false;
    }
    size_t total = 0;
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto part = steal(PySequence_Tuple(PyTuple_GetItem(seq.get(), i)));
        if (part.get() == nullptr) {
            return false;
        }
        if (PyTuple_Size(part.get()) != 2) {
            PyErr_SetString(PyExc_TypeError, msg);
            return false;
        }
        auto *name = PyTuple_GetItem(part.get(), 0);
        if (PyUnicode_Check(name) == 0) {
            PyErr_SetString(PyExc_TypeError, msg);
            return false;
        }
        Py_ssize_t size = 0;
        auto const *str = PyUnicode_AsUTF8AndSize(name, &size);
        if (str == nullptr) {
            return false;
        }
        auto params = steal(PySequence_Tuple(PyTuple_GetItem(part.get(), 1)));
        if (params.get() == nullptr) {
            return false;
        }
        auto m = PyTuple_Size(params.get());
        if (m < 0) {
            return false;
        }
        out.parts[i] = clingo_part_t{str, static_cast<size_t>(size), nullptr, static_cast<size_t>(m)};
        total += static_cast<size_t>(m);
        PyTuple_SetItem(out.keep.get(), 2 * i, part.release());
        PyTuple_SetItem(out.keep.get(), (2 * i) + 1, params.release());
    }
    if (total == 0) {
        return true;
    }
    if (!out.symbols.alloc(total)) {
        return false;
    }
    size_t offset = 0;
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *params = PyTuple_GetItem(out.keep.get(), (2 * i) + 1);
        auto &part = out.parts[i];
        part.params = out.symbols.data() + offset;
        for (size_t j = 0; j < part.params_size; ++j) {
            auto *item = PyTuple_GetItem(params, static_cast<Py_ssize_t>(j));
            if (!py_symbol_get(state->symbol_type, item, &out.symbols[offset + j])) {
                return false;
            }
        }
        offset += part.params_size;
    }
    return true;
}

//! Get the parts to ground: the given ones, the ones of the control object, or the base part.
//!
//! The control pointer is fetched after converting the parts.
auto ground_parts(ControlModuleState *state, PyObject *self, PyObject *obj, PartBuffer &buf, clingo_control_t **ctl,
                  clingo_part_t const **parts, size_t *size) -> bool {
    auto given = obj != nullptr && obj != Py_None;
    if (given && !parse_parts(state, obj, buf)) {
        return false;
    }
    *ctl = control_get(self);
    if (*ctl == nullptr) {
        return false;
    }
    if (given) {
        *parts = buf.parts.data();
        *size = buf.parts.size();
        return true;
    }
    auto has_value = false;
    if (!clingo_control_get_parts(*ctl, parts, size, &has_value)) {
        py_handle_error();
        return false;
    }
    if (!has_value) {
        static constexpr auto base = clingo_part_t{"base", 4, nullptr, 0};
        *parts = &base;
        *size = 1;
    }
    return true;
}

auto parts_list(ControlModuleState *state, clingo_part_t const *parts, size_t size) -> PyObject * {
    auto res = steal(PyList_New(static_cast<Py_ssize_t>(size)));
    if (res.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < size; ++i) {
        auto const &part = parts[i];
        auto name = steal(PyUnicode_FromStringAndSize(part.name, static_cast<Py_ssize_t>(part.name_size)));
        auto params = steal(PyList_New(static_cast<Py_ssize_t>(part.params_size)));
        if (name.get() == nullptr || params.get() == nullptr) {
            return nullptr;
        }
        for (size_t j = 0; j < part.params_size; ++j) {
            auto *sym = py_symbol_new(state->symbol_type, part.params[j], true);
            if (sym == nullptr) {
                return nullptr;
            }
            PyList_SetItem(params.get(), static_cast<Py_ssize_t>(j), sym);
        }
        auto *tup = PyTuple_Pack(2, name.get(), params.get());
        if (tup == nullptr) {
            return nullptr;
        }
        PyList_SetItem(res.get(), static_cast<Py_ssize_t>(i), tup);
    }
    return res.release();
}

// profile

struct ProfileData {
    PyObject *res;
    PyObject *stack;
};

auto set_item(PyObject *dict, char const *key, PyObject *value) -> bool {
    auto guard = steal(value);
    return guard.get() != nullptr && PyDict_SetItemString(dict, key, guard.get()) == 0;
}

//! Add a node at the given depth of the profile tree (borrowed reference).
auto profile_add(ProfileData *data, size_t depth, char const *type) -> PyObject * {
    auto node = steal(PyDict_New());
    if (node.get() == nullptr || !set_item(node.get(), "type", PyUnicode_FromString(type))) {
        return nullptr;
    }
    auto size = PyList_Size(data->stack);
    if (std::cmp_less(depth, size) && PyList_SetSlice(data->stack, static_cast<Py_ssize_t>(depth), size, nullptr) < 0) {
        return nullptr;
    }
    if (depth == 0) {
        if (PyList_Append(data->res, node.get()) < 0) {
            return nullptr;
        }
    } else {
        auto *parent = PyList_GetItem(data->stack, PyList_Size(data->stack) - 1);
        if (parent == nullptr) {
            return nullptr;
        }
        auto *children = PyDict_GetItemString(parent, "children");
        if (children == nullptr || PyList_Append(children, node.get()) < 0) {
            return nullptr;
        }
    }
    if (PyList_Append(data->stack, node.get()) < 0) {
        return nullptr;
    }
    return node.get();
}

auto profile_internal(size_t depth, char const *key, size_t key_size, bool nested, void *data) -> bool {
    auto *node = profile_add(static_cast<ProfileData *>(data), depth, "internal");
    return node != nullptr &&
           set_item(node, "key", PyUnicode_FromStringAndSize(key, static_cast<Py_ssize_t>(key_size))) &&
           set_item(node, "nested", PyBool_FromLong(nested ? 1 : 0)) && set_item(node, "children", PyList_New(0));
}

auto profile_leaf(size_t depth, clingo_profile_data_t *values, clingo_profile_type_t type, void *data) -> bool {
    auto *node = profile_add(static_cast<ProfileData *>(data), depth, "leaf");
    return node != nullptr &&
           set_item(node, "profile_type", PyUnicode_FromString(type == clingo_profile_type_step ? "step" : "accu")) &&
           set_item(node, "matches", PyLong_FromUnsignedLongLong(values->matches)) &&
           set_item(node, "instances", PyLong_FromUnsignedLongLong(values->instances)) &&
           set_item(node, "time_instantiate", PyLong_FromUnsignedLongLong(values->time_instantiate)) &&
           set_item(node, "time_propagate", PyLong_FromUnsignedLongLong(values->time_propagate));
}

// Control

auto py_control_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"lib", "options", nullptr};
    PyObject *lib_obj = nullptr;
    PyObject *options = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:Control", cast_kw(kwlist), &lib_obj, &options) == 0) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    auto seq = PyGuard{nullptr};
    auto opts = PyBuffer<clingo_string_t>{};
    if (options != nullptr && !parse_string_sequence(options, seq, opts, "options must be a sequence of strings")) {
        return nullptr;
    }
    auto *lib = py_library_get(state->lib_type, lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }
    clingo_control_t *ctl = nullptr;
    if (!clingo_control_new(lib, opts.data(), opts.size(), &ctl)) {
        return py_handle_error();
    }
    return control_new(state, lib_obj, ctl, true);
}

auto py_control_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *c = control_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(c->library);
    Py_VISIT(c->keep);
    return 0;
}

auto py_control_clear(PyObject *self) -> int {
    auto *c = control_cast(self);
    // NOTE: releasing the clingo control object first runs its finalizers and
    // ensures that registered objects outlive it
    control_invalidate(self);
    Py_CLEAR(c->keep);
    Py_CLEAR(c->library);
    return 0;
}

auto py_control_register_propagator(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    -> PyObject * {
    PyObject *propagator{};
    if (!parse_args("register_propagator", {"propagator"}, {}, args, nargs, kwnames, {&propagator})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto *c = control_cast(self);
    if (!py_register_propagator(state->propagator_type, ctl, c->keep, c->library, propagator)) {
        return nullptr;
    }
    Py_RETURN_NONE;
}

auto py_control_join(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *program = unset_arg();
    if (!parse_args("join", {"program"}, {}, args, nargs, kwnames, {&program})) {
        return nullptr;
    }
    auto *state = state_of(self);
    auto *prg = state != nullptr ? py_ast_program_get(state->program_type, program) : nullptr;
    if (prg == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!with_gil_released([&] { return clingo_control_join(ctl, prg); })) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_control_parse_string(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *program = unset_arg();
    if (!parse_args("parse_string", {"program"}, {}, args, nargs, kwnames, {&program})) {
        return nullptr;
    }
    if (PyUnicode_Check(program) == 0) {
        PyErr_SetString(PyExc_TypeError, "program must be a string");
        return nullptr;
    }
    Py_ssize_t size = 0;
    auto const *str = PyUnicode_AsUTF8AndSize(program, &size);
    if (str == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!with_gil_released([&] { return clingo_control_parse_string(ctl, str, static_cast<size_t>(size)); })) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_control_parse_files(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *files = unset_arg();
    if (!parse_args("parse_files", {"files"}, {}, args, nargs, kwnames, {&files})) {
        return nullptr;
    }
    auto seq = PyGuard{nullptr};
    auto buf = PyBuffer<clingo_string_t>{};
    if (!parse_string_sequence(files, seq, buf, "files must be a sequence of strings")) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!with_gil_released([&] { return clingo_control_parse_files(ctl, buf.data(), buf.size()); })) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_control_write_aspif(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *path_obj{};
    PyObject *symbols_obj{};
    PyObject *append_obj{};
    PyObject *preamble_obj{};
    PyObject *preprocess_obj{};
    if (!parse_args("write_aspif", {}, {"path", "symbols", "append", "preamble", "preprocess"}, args, nargs, kwnames,
                    {&path_obj, &symbols_obj, &append_obj, &preamble_obj, &preprocess_obj})) {
        return nullptr;
    }
    auto symbols = false;
    auto append = false;
    auto preamble = false;
    auto preprocess = true;
    auto auto_preamble = preamble_obj == nullptr || preamble_obj == Py_None;
    if (!parse_bool(symbols_obj, &symbols) || !parse_bool(append_obj, &append) ||
        (!auto_preamble && !parse_bool(preamble_obj, &preamble)) || !parse_bool(preprocess_obj, &preprocess)) {
        return nullptr;
    }
    clingo_write_aspif_mode_t mode = 0;
    if (symbols) {
        mode |= clingo_write_aspif_mode_symbols;
    }
    if (append) {
        mode |= clingo_write_aspif_mode_append;
    }
    if (auto_preamble) {
        mode |= clingo_write_aspif_mode_preamble_auto;
    } else if (preamble) {
        mode |= clingo_write_aspif_mode_preamble;
    }
    if (preprocess) {
        mode |= clingo_write_aspif_mode_preprocess;
    }
    char const *path = nullptr;
    Py_ssize_t size = 0;
    if (path_obj != nullptr && path_obj != Py_None) {
        if (PyUnicode_Check(path_obj) == 0) {
            PyErr_SetString(PyExc_TypeError, "path must be a string");
            return nullptr;
        }
        path = PyUnicode_AsUTF8AndSize(path_obj, &size);
        if (path == nullptr) {
            return nullptr;
        }
        if (size == 0) {
            PyErr_SetString(PyExc_ValueError, "path cannot be empty");
            return nullptr;
        }
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!clingo_control_write_aspif(ctl, path, static_cast<size_t>(size), mode)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_control_ground(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *parts_obj{};
    PyObject *ctx_obj{};
    if (!parse_args("ground", {}, {"parts", "context"}, args, nargs, kwnames, {&parts_obj, &ctx_obj})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto buf = PartBuffer{};
    clingo_control_t *ctl = nullptr;
    clingo_part_t const *parts = nullptr;
    size_t size = 0;
    if (!ground_parts(state, self, parts_obj, buf, &ctl, &parts, &size)) {
        return nullptr;
    }
    if (!py_ground(state->ground_handle_type, ctl, parts, size, ctx_obj != nullptr ? ctx_obj : Py_None)) {
        return nullptr;
    }
    Py_RETURN_NONE;
}

auto py_control_start_ground(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *parts_obj{};
    PyObject *ctx_obj{};
    PyObject *finish_obj{};
    if (!parse_args("start_ground", {}, {"parts", "context", "on_finish"}, args, nargs, kwnames,
                    {&parts_obj, &ctx_obj, &finish_obj})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto buf = PartBuffer{};
    clingo_control_t *ctl = nullptr;
    clingo_part_t const *parts = nullptr;
    size_t size = 0;
    if (!ground_parts(state, self, parts_obj, buf, &ctl, &parts, &size)) {
        return nullptr;
    }
    return py_ground_start(state->ground_handle_type, ctl, self, parts, size, ctx_obj != nullptr ? ctx_obj : Py_None,
                           finish_obj != nullptr ? finish_obj : Py_None);
}

auto py_control_ground_async(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *loop = unset_arg();
    PyObject *parts_obj{};
    PyObject *ctx_obj{};
    if (!parse_args("ground_async", {"loop"}, {"parts", "context"}, args, nargs, kwnames,
                    {&loop, &parts_obj, &ctx_obj})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto buf = PartBuffer{};
    clingo_control_t *ctl = nullptr;
    clingo_part_t const *parts = nullptr;
    size_t size = 0;
    if (!ground_parts(state, self, parts_obj, buf, &ctl, &parts, &size)) {
        return nullptr;
    }
    return py_ground_async(state->ground_handle_type, ctl, self, parts, size, ctx_obj != nullptr ? ctx_obj : Py_None,
                           loop);
}

auto py_control_base(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_base_t const *base = nullptr;
    if (!clingo_control_base(ctl, &base)) {
        return py_handle_error();
    }
    return py_base_new(self, pointer_slot(&control_cast(self)->ctl), state->base_type, base);
}

//! Convert the assumptions and get the control pointer afterward.
auto solve_assumptions(ControlModuleState *state, PyObject *self, PyObject *assumptions,
                       PyBuffer<clingo_literal_t> &buf, clingo_control_t **ctl) -> bool {
    if (assumptions != nullptr) {
        auto base = steal(py_control_base(self, nullptr));
        if (base.get() == nullptr || !convert(state->base_type, base.get(), assumptions, false, false, buf)) {
            return false;
        }
    }
    *ctl = control_get(self);
    return *ctl != nullptr;
}

auto py_control_solve(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *ass_obj{};
    PyObject *model_obj{};
    PyObject *unsat_obj{};
    PyObject *stats_obj{};
    PyObject *finish_obj{};
    if (!parse_args("solve", {}, {"assumptions", "on_model", "on_unsat", "on_stats", "on_finish"}, args, nargs, kwnames,
                    {&ass_obj, &model_obj, &unsat_obj, &stats_obj, &finish_obj})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto buf = PyBuffer<clingo_literal_t>{};
    clingo_control_t *ctl = nullptr;
    if (!solve_assumptions(state, self, ass_obj, buf, &ctl)) {
        return nullptr;
    }
    return py_solve(state->solve_handle_type, ctl, self, std::span<clingo_literal_t const>{buf.data(), buf.size()},
                    model_obj, unsat_obj, stats_obj, finish_obj);
}

auto py_control_start_solve(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *ass_obj{};
    PyObject *model_obj{};
    PyObject *unsat_obj{};
    PyObject *stats_obj{};
    PyObject *finish_obj{};
    PyObject *yield_obj{};
    PyObject *async_obj{};
    if (!parse_args("start_solve", {},
                    {"assumptions", "on_model", "on_unsat", "on_stats", "on_finish", "yield_", "async_"}, args, nargs,
                    kwnames, {&ass_obj, &model_obj, &unsat_obj, &stats_obj, &finish_obj, &yield_obj, &async_obj})) {
        return nullptr;
    }
    auto yield = false;
    auto async = false;
    if (!parse_bool(yield_obj, &yield) || !parse_bool(async_obj, &async)) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto buf = PyBuffer<clingo_literal_t>{};
    clingo_control_t *ctl = nullptr;
    if (!solve_assumptions(state, self, ass_obj, buf, &ctl)) {
        return nullptr;
    }
    return py_solve_start(state->solve_handle_type, ctl, self,
                          std::span<clingo_literal_t const>{buf.data(), buf.size()}, model_obj, unsat_obj, stats_obj,
                          finish_obj, yield, async);
}

auto py_control_solve_async(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *loop = unset_arg();
    PyObject *ass_obj{};
    PyObject *model_obj{};
    PyObject *unsat_obj{};
    PyObject *stats_obj{};
    PyObject *yield_obj{};
    if (!parse_args("solve_async", {"loop"}, {"assumptions", "on_model", "on_unsat", "on_stats", "yield_"}, args, nargs,
                    kwnames, {&loop, &ass_obj, &model_obj, &unsat_obj, &stats_obj, &yield_obj})) {
        return nullptr;
    }
    auto yield = false;
    if (!parse_bool(yield_obj, &yield)) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto buf = PyBuffer<clingo_literal_t>{};
    clingo_control_t *ctl = nullptr;
    if (!solve_assumptions(state, self, ass_obj, buf, &ctl)) {
        return nullptr;
    }
    return py_solve_async(state->solve_handle_type, ctl, self,
                          std::span<clingo_literal_t const>{buf.data(), buf.size()}, model_obj, unsat_obj, stats_obj,
                          loop, yield);
}

auto py_control_main(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!with_gil_released([ctl] { return clingo_control_main(ctl); })) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_control_interrupt(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_control_interrupt(ctl);
    Py_RETURN_NONE;
}

auto py_control_observe(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *observer{};
    PyObject *preprocess_obj{};
    if (!parse_args("observe", {"observer"}, {"preprocess"}, args, nargs, kwnames, {&observer, &preprocess_obj})) {
        return nullptr;
    }
    auto preprocess = true;
    if (!parse_bool(preprocess_obj, &preprocess)) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!py_observe(state->backend_module, ctl, observer, preprocess)) {
        return nullptr;
    }
    Py_RETURN_NONE;
}

auto py_control_discard(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *minimize_obj{};
    PyObject *project_obj{};
    if (!parse_args("discard", {}, {"minimize", "project"}, args, nargs, kwnames, {&minimize_obj, &project_obj})) {
        return nullptr;
    }
    auto minimize = false;
    auto project = false;
    if (!parse_bool(minimize_obj, &minimize) || !parse_bool(project_obj, &project)) {
        return nullptr;
    }
    clingo_discard_type_t type = 0;
    if (minimize) {
        type |= clingo_discard_type_e::minimize;
    }
    if (project) {
        type |= clingo_discard_type_e::project;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!clingo_control_discard(ctl, type)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_control_buffer(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_string_t res;
    if (!clingo_control_buffer(ctl, &res)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(res.data, static_cast<Py_ssize_t>(res.size));
}

auto py_control_backend(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    return py_backend_manager_new(state->backend_manager_type, ctl, self);
}

auto py_control_config(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_config_t *config = nullptr;
    clingo_id_t key = 0;
    if (!clingo_control_config(ctl, &config) || !clingo_config_root(config, &key)) {
        return py_handle_error();
    }
    auto *c = control_cast(self);
    return py_config_new(self, pointer_slot(&c->ctl), c->keep, state->config_type, config, key);
}

auto py_control_stats(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_stats_t const *stats = nullptr;
    uint64_t key = 0;
    if (!clingo_control_stats(ctl, &stats) || !clingo_stats_root(stats, &key)) {
        return py_handle_error();
    }
    return py_stats_new(self, pointer_slot(&control_cast(self)->ctl), state->stats_type,
                        const_cast<clingo_stats_t *>(stats), key); // NOLINT
}

auto py_control_mode(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_mode_t mode = 0;
    if (!clingo_control_mode(ctl, &mode)) {
        return py_handle_error();
    }
    return PyObject_CallFunction(state->mode_enum, "i", mode);
}

auto py_control_profile(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto res = steal(PyList_New(0));
    auto stack = steal(PyList_New(0));
    if (res.get() == nullptr || stack.get() == nullptr) {
        return nullptr;
    }
    auto data = ProfileData{res.get(), stack.get()};
    static constexpr auto visitor = clingo_profile_visitor_t{profile_internal, profile_leaf};
    if (!clingo_control_profile(ctl, &visitor, &data)) {
        return PyErr_Occurred() != nullptr ? nullptr : py_handle_error();
    }
    return res.release();
}

auto py_control_const_map(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_const_map_t const *map = nullptr;
    if (!clingo_control_const_map(ctl, &map)) {
        return py_handle_error();
    }
    auto *obj = alloc_object(state->const_map_type);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *m = const_map_cast(obj);
    Py_INCREF(self);
    m->owner = self;
    m->valid = pointer_slot(&control_cast(self)->ctl);
    m->map = map;
    return obj;
}

auto py_control_get_parts(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_part_t const *parts = nullptr;
    size_t size = 0;
    auto has_value = false;
    if (!clingo_control_get_parts(ctl, &parts, &size, &has_value)) {
        return py_handle_error();
    }
    if (!has_value) {
        Py_RETURN_NONE;
    }
    return parts_list(state, parts, size);
}

auto py_control_set_parts(PyObject *self, PyObject *value, [[maybe_unused]] void *closure) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "cannot delete parts");
        return -1;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return -1;
    }
    auto buf = PartBuffer{};
    auto has_value = value != Py_None;
    if (has_value && !parse_parts(state, value, buf)) {
        return -1;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return -1;
    }
    if (!clingo_control_set_parts(ctl, buf.parts.data(), buf.parts.size(), has_value)) {
        py_handle_error();
        return -1;
    }
    return 0;
}

auto py_control_methods = std::array{
    PyMethodDef{
        "register_propagator",
        method_cast(py_control_register_propagator),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Register the given propagator for theory propagation."),
    },
    PyMethodDef{
        "join",
        method_cast(py_control_join),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Join with the given non-ground logic program."),
    },
    PyMethodDef{
        "parse_string",
        method_cast(py_control_parse_string),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse a logic program given as a string."),
    },
    PyMethodDef{
        "parse_files",
        method_cast(py_control_parse_files),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse the logic programs in the given files."),
    },
    PyMethodDef{
        "write_aspif",
        method_cast(py_control_write_aspif),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Write the current logic program in aspif format."),
    },
    PyMethodDef{
        "ground",
        method_cast(py_control_ground),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Ground the given program parts."),
    },
    PyMethodDef{
        "start_ground",
        method_cast(py_control_start_ground),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Start grounding the given program parts in the background."),
    },
    PyMethodDef{
        "ground_async",
        method_cast(py_control_ground_async),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Ground the given program parts in the background and return an awaitable handle."),
    },
    PyMethodDef{
        "solve",
        method_cast(py_control_solve),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Solve the current ground program."),
    },
    PyMethodDef{
        "start_solve",
        method_cast(py_control_start_solve),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Start solving the current ground program and return a handle."),
    },
    PyMethodDef{
        "solve_async",
        method_cast(py_control_solve_async),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Solve the current ground program in the background and return an awaitable handle."),
    },
    PyMethodDef{
        "main",
        py_control_main,
        METH_NOARGS,
        PyDoc_STR("Ground and solve a logic program based on the current control mode."),
    },
    PyMethodDef{"interrupt", py_control_interrupt, METH_NOARGS, PyDoc_STR("Interrupt the active solve call.")},
    PyMethodDef{
        "observe",
        method_cast(py_control_observe),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Inspect the ground program of the current step."),
    },
    PyMethodDef{
        "discard",
        method_cast(py_control_discard),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Discard statements of the selected types."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_control_getset = std::array{
    PyGetSetDef{"buffer", py_control_buffer, nullptr, PyDoc_STR("The content of the output buffer."), nullptr},
    PyGetSetDef{"base", py_control_base, nullptr, PyDoc_STR("The atom/term bases of the program."), nullptr},
    PyGetSetDef{
        "backend",
        py_control_backend,
        nullptr,
        PyDoc_STR("A backend manager to extend the ground program."),
        nullptr,
    },
    PyGetSetDef{"config", py_control_config, nullptr, PyDoc_STR("The solver config."), nullptr},
    PyGetSetDef{"stats", py_control_stats, nullptr, PyDoc_STR("The solver stats."), nullptr},
    PyGetSetDef{"mode", py_control_mode, nullptr, PyDoc_STR("The application mode."), nullptr},
    PyGetSetDef{
        "profile",
        py_control_profile,
        nullptr,
        PyDoc_STR("The profiling information as a list of nodes."),
        nullptr,
    },
    PyGetSetDef{
        "const_map",
        py_control_const_map,
        nullptr,
        PyDoc_STR("The map of constants defined by #const directives."),
        nullptr,
    },
    PyGetSetDef{
        "parts",
        py_control_get_parts,
        py_control_set_parts,
        PyDoc_STR("The program parts to ground."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_control_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A control object for grounding and solving.")}, // NOLINT
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(py_control_tp_new)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_control_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_control_clear)},
    PyType_Slot{Py_tp_methods, py_control_methods.data()},
    PyType_Slot{Py_tp_getset, py_control_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_control_spec = PyType_Spec{
    "clingo.control.Control", sizeof(PyControlObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    py_control_slots.data(),
};

// ConstMap

auto const_map_get(PyObject *self) -> clingo_const_map_t const * {
    auto *m = const_map_cast(self);
    if (!pointer_valid(m->valid)) {
        PyErr_SetString(PyExc_RuntimeError, "control is no longer valid");
        return nullptr;
    }
    return m->map;
}

auto const_map_find(PyObject *self, PyObject *key, clingo_symbol_t *sym, bool *found) -> bool {
    *found = false;
    if (PyUnicode_Check(key) == 0) {
        return true;
    }
    Py_ssize_t size = 0;
    auto const *name = PyUnicode_AsUTF8AndSize(key, &size);
    if (name == nullptr) {
        return false;
    }
    auto const *map = const_map_get(self);
    if (map == nullptr) {
        return false;
    }
    if (!clingo_const_map_find(map, name, static_cast<size_t>(size), sym, found)) {
        py_handle_error();
        return false;
    }
    return true;
}

auto py_const_map_len(PyObject *self) -> Py_ssize_t {
    auto const *map = const_map_get(self);
    if (map == nullptr) {
        return -1;
    }
    size_t size = 0;
    if (!clingo_const_map_size(map, &size)) {
        py_handle_error();
        return -1;
    }
    return static_cast<Py_ssize_t>(size);
}

auto py_const_map_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_symbol_t sym = 0;
    auto found = false;
    if (!const_map_find(self, key, &sym, &found)) {
        return nullptr;
    }
    if (!found) {
        PyErr_SetObject(PyExc_KeyError, key);
        return nullptr;
    }
    return py_symbol_new(state->symbol_type, sym, true);
}

auto py_const_map_contains(PyObject *self, PyObject *key) -> int {
    clingo_symbol_t sym = 0;
    auto found = false;
    if (!const_map_find(self, key, &sym, &found)) {
        return -1;
    }
    return found ? 1 : 0;
}

auto py_const_map_get(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *key = unset_arg();
    PyObject *def{};
    if (!parse_args("get", {"key"}, {"default"}, args, nargs, kwnames, {&key, &def})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_symbol_t sym = 0;
    auto found = false;
    if (!const_map_find(self, key, &sym, &found)) {
        return nullptr;
    }
    if (found) {
        return py_symbol_new(state->symbol_type, sym, true);
    }
    if (def == nullptr) {
        Py_RETURN_NONE;
    }
    Py_INCREF(def);
    return def;
}

auto const_map_iter_new(PyObject *self, IterType type) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *obj = alloc_object(state->const_map_iter_type);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *it = const_map_iter_cast(obj);
    Py_INCREF(self);
    it->map = self;
    it->index = 0;
    it->type = type;
    return obj;
}

auto py_const_map_iter(PyObject *self) -> PyObject * {
    return const_map_iter_new(self, IterType::keys);
}

auto const_map_keys(PyObject *self) -> PyObject * {
    return const_map_iter_new(self, IterType::keys);
}

auto const_map_values(PyObject *self) -> PyObject * {
    return const_map_iter_new(self, IterType::values);
}

auto const_map_items(PyObject *self) -> PyObject * {
    return const_map_iter_new(self, IterType::items);
}

using ConstMapProtocol = MappingProtocol<view_type, const_map_keys, const_map_values, const_map_items>;

void py_const_map_dealloc(PyObject *self) {
    Py_CLEAR(const_map_cast(self)->owner);
    free_object(self);
}

auto py_const_map_methods = std::array{
    PyMethodDef{
        "get",
        method_cast(py_const_map_get),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the value for the given key or the default if absent."),
    },
    PyMethodDef{
        "keys",
        ConstMapProtocol::keys,
        METH_NOARGS,
        PyDoc_STR("Get a view on the keys."),
    },
    PyMethodDef{
        "values",
        ConstMapProtocol::values,
        METH_NOARGS,
        PyDoc_STR("Get a view on the values."),
    },
    PyMethodDef{
        "items",
        ConstMapProtocol::items,
        METH_NOARGS,
        PyDoc_STR("Get a view on the items."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_const_map_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("The map of constants defined by #const directives.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_const_map_dealloc)},
    PyType_Slot{Py_tp_methods, py_const_map_methods.data()},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(ConstMapProtocol::richcompare)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_const_map_iter)},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_const_map_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_const_map_subscript)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_const_map_contains)},
    PyType_Slot{0, nullptr},
};

auto py_const_map_spec = PyType_Spec{
    "clingo.control.ConstMap", sizeof(PyConstMapObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_const_map_slots.data(),
};

// _ConstMapIter

auto py_const_map_iter_next(PyObject *self) -> PyObject * {
    auto *it = const_map_iter_cast(self);
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto const *map = const_map_get(it->map);
    if (map == nullptr) {
        return nullptr;
    }
    size_t size = 0;
    if (!clingo_const_map_size(map, &size)) {
        return py_handle_error();
    }
    if (it->index >= size) {
        return nullptr;
    }
    clingo_string_t name;
    clingo_symbol_t sym = 0;
    if (!clingo_const_map_at(map, it->index++, &name, &sym)) {
        return py_handle_error();
    }
    if (it->type == IterType::values) {
        return py_symbol_new(state->symbol_type, sym, true);
    }
    auto key = steal(PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size)));
    if (key.get() == nullptr || it->type == IterType::keys) {
        return key.release();
    }
    auto value = steal(py_symbol_new(state->symbol_type, sym, true));
    if (value.get() == nullptr) {
        return nullptr;
    }
    return PyTuple_Pack(2, key.get(), value.get());
}

void py_const_map_iter_dealloc(PyObject *self) {
    Py_CLEAR(const_map_iter_cast(self)->map);
    free_object(self);
}

auto py_const_map_iter_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("Iterator over the constants of a control object.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_const_map_iter_dealloc)},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(PyObject_SelfIter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_const_map_iter_next)},
    PyType_Slot{0, nullptr},
};

auto py_const_map_iter_spec = PyType_Spec{
    "clingo.control._ConstMapIter",
    sizeof(PyConstMapIterObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_const_map_iter_slots.data(),
};

// module

constexpr auto mode_members = std::array<IntEnumMember, 4>{
    IntEnumMember{"Parse", static_cast<int>(clingo_mode_parse)},
    IntEnumMember{"Rewrite", static_cast<int>(clingo_mode_rewrite)},
    IntEnumMember{"Ground", static_cast<int>(clingo_mode_ground)},
    IntEnumMember{"Solve", static_cast<int>(clingo_mode_solve)},
};

auto control_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<ControlModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_VISIT(state->*member);
    }
    return 0;
}

auto control_module_clear(PyObject *module) -> int {
    auto *state = static_cast<ControlModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_CLEAR(state->*member);
    }
    return 0;
}

void control_module_free(void *module) {
    control_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef control_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.control",
    PyDoc_STR("Functions and classes to ground and solve logic programs."),
    static_cast<Py_ssize_t>(sizeof(ControlModuleState)),
    nullptr,
    nullptr,
    control_module_traverse,
    control_module_clear,
    control_module_free,
};

} // namespace

auto py_control_type() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.control"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    if (state->control_type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.control module is not initialized");
        return nullptr;
    }
    Py_INCREF(state->control_type);
    return state->control_type;
}

auto py_control_get(PyObject *type, PyObject *obj) -> clingo_control_t * {
    if (PyObject_TypeCheck(obj, reinterpret_cast<PyTypeObject *>(type)) == 0) {
        PyErr_SetString(PyExc_TypeError, "expected clingo.control.Control");
        return nullptr;
    }
    return control_get(obj);
}

auto py_control_begin(PyObject *type, PyObject *library, clingo_control_t *ctl, ControlRef ref_type) -> PyObject * {
    auto *state = get_state_from_type(reinterpret_cast<PyTypeObject *>(type));
    if (state == nullptr) {
        return nullptr;
    }
    if (auto *owner = control_owner(state, ctl); owner != nullptr) {
        // an existing owner is reused and stays valid after the end call
        if (ref_type == ControlRef::claim) {
            PyErr_SetString(PyExc_RuntimeError, "cannot claim a control object that already has an owner");
            return nullptr;
        }
        Py_INCREF(owner);
        return owner;
    }
    if (ref_type == ControlRef::claim) {
        // the claimed control is owned until the end call
        clingo_control_acquire(ctl);
        return control_new(state, library, ctl, true);
    }
    // a non-owned control object registered as owner
    auto obj = steal(control_new(state, library, ctl, false));
    if (obj.get() == nullptr) {
        return nullptr;
    }
    // the clingo control object keeps the non-owned object alive; if this
    // fails, destroying the object removes its registration
    if (!clingo_control_add_finalizer(ctl, control_finalizer, obj.get())) {
        return py_handle_error();
    }
    Py_INCREF(obj.get());
    return obj.release();
}

void py_control_end(PyObject *obj, ControlRef ref_type) noexcept {
    auto ref = steal(obj);
    if (ref_type == ControlRef::claim) {
        // the lifetime of the claimed control object ends here
        control_invalidate(obj);
    }
}

auto create_control(PyObject *parent) -> PyObject * {
    auto control_mod = PyGuard{nullptr};
    if (!add_submodule(parent, &control_def, &control_mod.get())) {
        return nullptr;
    }
    auto *state = get_state(control_mod.get());
    if (state == nullptr || !add_int_enum(control_mod.get(), "ControlMode", mode_members, &state->mode_enum, {}) ||
        !add_type(control_mod.get(), "_ConstMapIter", &py_const_map_iter_spec, &state->const_map_iter_type) ||
        !add_type(control_mod.get(), "ConstMap", &py_const_map_spec, &state->const_map_type) ||
        !add_type(control_mod.get(), "Control", &py_control_spec, &state->control_type) ||
        !register_abc(state->const_map_type, "Mapping")) {
        return nullptr;
    }
    return control_mod.release();
}

auto link_control(PyObject *mod) -> bool {
    static constexpr auto imports = std::array{
        TypeImport<ControlModuleState>{&ControlModuleState::lib_type, &py_library_type},
        TypeImport<ControlModuleState>{&ControlModuleState::symbol_type, &py_symbol_type},
        TypeImport<ControlModuleState>{&ControlModuleState::base_type, &py_base_type},
        TypeImport<ControlModuleState>{&ControlModuleState::config_type, &py_config_type},
        TypeImport<ControlModuleState>{&ControlModuleState::stats_type, &py_stats_view_type},
        TypeImport<ControlModuleState>{&ControlModuleState::ground_handle_type, &py_ground_handle_type},
        TypeImport<ControlModuleState>{&ControlModuleState::solve_handle_type, &py_solve_handle_type},
        TypeImport<ControlModuleState>{&ControlModuleState::backend_manager_type, &py_backend_manager_type},
        TypeImport<ControlModuleState>{&ControlModuleState::propagator_type, &py_propagator_type},
        TypeImport<ControlModuleState>{&ControlModuleState::program_type, &py_ast_program_type},
        TypeImport<ControlModuleState>{&ControlModuleState::keys_view_type, &py_keys_view_type},
        TypeImport<ControlModuleState>{&ControlModuleState::values_view_type, &py_values_view_type},
        TypeImport<ControlModuleState>{&ControlModuleState::items_view_type, &py_items_view_type},
    };
    auto *state = get_state(mod);
    if (state == nullptr || !import_types(state, imports)) {
        return false;
    }
    state->backend_module = PyImport_ImportModule("clingo.backend");
    return state->backend_module != nullptr;
}

} // namespace PyClingo
