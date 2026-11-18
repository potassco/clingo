#define PY_SSIZE_T_CLEAN

#include "propagate.hh"

#include "base.hh"
#include "core.hh"
#include "util.hh"

#include <clingo/propagate.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <utility>
#include <vector>

namespace PyClingo {

namespace {

// objects

struct PropagateModuleState {
    PyObject *base_type{nullptr};
    PyObject *weight_constraint_type_enum{nullptr};
    PyObject *check_mode_enum{nullptr};
    PyObject *undo_mode_enum{nullptr};
    PyObject *assignment_type{nullptr};
    PyObject *trail_type{nullptr};
    PyObject *trail_view_type{nullptr};
    PyObject *control_type{nullptr};
    PyObject *init_type{nullptr};
    PyObject *propagator_type{nullptr};
    PyObject *handle_type{nullptr};
    PyObject *str_init{nullptr};
    PyObject *str_attach{nullptr};
    PyObject *str_propagate{nullptr};
    PyObject *str_undo{nullptr};
    PyObject *str_check{nullptr};
    PyObject *str_decide{nullptr};
};

constexpr auto state_members = std::array{
    &PropagateModuleState::base_type,       &PropagateModuleState::weight_constraint_type_enum,
    &PropagateModuleState::check_mode_enum, &PropagateModuleState::undo_mode_enum,
    &PropagateModuleState::assignment_type, &PropagateModuleState::trail_type,
    &PropagateModuleState::trail_view_type, &PropagateModuleState::control_type,
    &PropagateModuleState::init_type,       &PropagateModuleState::propagator_type,
    &PropagateModuleState::handle_type,     &PropagateModuleState::str_init,
    &PropagateModuleState::str_attach,      &PropagateModuleState::str_propagate,
    &PropagateModuleState::str_undo,        &PropagateModuleState::str_check,
    &PropagateModuleState::str_decide,
};

struct PyAssignmentObject {
    PyObject_HEAD clingo_assignment_t const *ass;
    PyObject *trail;
};

struct PyTrailObject {
    PyObject_HEAD PyObject *assignment;
};

struct PyTrailViewObject {
    PyObject_HEAD PyObject *assignment;
    Py_ssize_t start;
    Py_ssize_t step;
    Py_ssize_t size;
};

struct PyPropagateControlObject {
    PyObject_HEAD clingo_propagate_control_t *ctl;
};

struct PyPropagateInitObject {
    PyPropagateControlObject base;
    clingo_propagate_init_t *init;
    PyObject *library;
};

struct PyPropagatorHandleObject {
    PyObject_HEAD PyObject *propagator;
    PyObject *library;
    // NOTE: per solver thread wrappers stored as [assignment, control, ...]
    PyObject **cache;
    size_t cache_size;
    bool has_init;
    bool has_attach;
    bool has_propagate;
    bool has_undo;
    bool has_check;
    bool has_decide;
};

template <class T> auto as(PyObject *obj) -> T * {
    return reinterpret_cast<T *>(obj);
}

auto get_state(PyObject *module) -> PropagateModuleState * {
    auto *state = static_cast<PropagateModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "propagate module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_type(PyObject *type) -> PropagateModuleState * {
    auto *state = static_cast<PropagateModuleState *>(PyType_GetModuleState(reinterpret_cast<PyTypeObject *>(type)));
    if (state == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "propagate module state is unavailable");
        }
        return nullptr;
    }
    return state;
}

auto state_of(PyObject *self) -> PropagateModuleState * {
    return get_state_from_type(reinterpret_cast<PyObject *>(Py_TYPE(self)));
}

auto alloc_object(PyObject *type) -> PyObject * {
    return PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(type), 0);
}

// argument parsing

using PyCFunctionFastKw = PyObject *(*)(PyObject *, PyObject *const *, Py_ssize_t, PyObject *);

auto fast_cast(PyCFunctionFastKw fun) noexcept -> PyCFunction {
    return reinterpret_cast<PyCFunction>(reinterpret_cast<void *>(fun)); // NOLINT
}

//! Parse arguments passed with the METH_FASTCALL | METH_KEYWORDS convention.
template <size_t N>
auto parse_args(char const *fname, std::array<char const *, N> const &names, size_t required, PyObject *const *args,
                Py_ssize_t nargs, PyObject *kwnames, std::array<PyObject *, N> &out) -> bool {
    out.fill(nullptr);
    if (std::cmp_greater(nargs, N)) {
        PyErr_Format(PyExc_TypeError, "%s() takes at most %zu arguments (%zd given)", fname, N, nargs);
        return false;
    }
    for (Py_ssize_t i = 0; i < nargs; ++i) {
        out[static_cast<size_t>(i)] = args[i];
    }
    if (kwnames != nullptr) {
        auto nkw = PyTuple_Size(kwnames);
        if (nkw < 0) {
            return false;
        }
        for (Py_ssize_t k = 0; k < nkw; ++k) {
            auto *key = PyTuple_GetItem(kwnames, k);
            if (key == nullptr) {
                return false;
            }
            size_t j = 0;
            while (j < N && PyUnicode_CompareWithASCIIString(key, names[j]) != 0) {
                ++j;
            }
            if (j == N) {
                PyErr_Format(PyExc_TypeError, "%s() got an unexpected keyword argument '%U'", fname, key);
                return false;
            }
            if (out[j] != nullptr) {
                PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", fname, names[j]);
                return false;
            }
            out[j] = args[nargs + k];
        }
    }
    for (size_t j = 0; j < required; ++j) {
        if (out[j] == nullptr) {
            PyErr_Format(PyExc_TypeError, "%s() missing required argument '%s'", fname, names[j]);
            return false;
        }
    }
    return true;
}

auto parse_bool(PyObject *obj, bool *out) -> bool {
    if (obj == nullptr) {
        return true;
    }
    auto res = PyObject_IsTrue(obj);
    if (res < 0) {
        return false;
    }
    *out = res != 0;
    return true;
}

auto parse_literal_arg(char const *fname, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames,
                       clingo_literal_t *lit) -> bool {
    static constexpr auto names = std::array<char const *, 1>{"literal"};
    auto out = std::array<PyObject *, 1>{};
    return parse_args(fname, names, 1, args, nargs, kwnames, out) && parse_num(out[0], lit, "literal");
}

auto parse_level_arg(char const *fname, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames, uint32_t *level)
    -> bool {
    static constexpr auto names = std::array<char const *, 1>{"level"};
    auto out = std::array<PyObject *, 1>{};
    return parse_args(fname, names, 1, args, nargs, kwnames, out) && parse_num(out[0], level, "level");
}

//! Temporarily take a thread local buffer.
//!
//! Reentrant calls (e.g., via `__index__`) see an empty buffer and allocate
//! their own one.
template <class T> class BufferLease {
  public:
    explicit BufferLease(std::vector<T> &pool) : pool_{pool}, buf_{std::move(pool)} { buf_.clear(); }
    BufferLease(BufferLease const &) = delete;
    BufferLease(BufferLease &&) = delete;
    auto operator=(BufferLease const &) -> BufferLease & = delete;
    auto operator=(BufferLease &&) -> BufferLease & = delete;
    ~BufferLease() {
        if (buf_.capacity() > pool_.capacity()) {
            pool_ = std::move(buf_);
        }
    }

    auto get() -> std::vector<T> & { return buf_; }

  private:
    std::vector<T> &pool_;
    std::vector<T> buf_;
};

thread_local auto lit_pool = LitVec{};
thread_local auto wlit_pool = WeightLitVec{};

// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
template <class F> auto for_each_item(PyObject *obj, F &&fun) -> bool {
    try {
        if (PyList_Check(obj) != 0) {
            auto n = PyList_Size(obj);
            for (Py_ssize_t i = 0; i < n; ++i) {
                // NOTE: borrow because item conversion may run Python code
                auto item = borrow(PyList_GetItem(obj, i));
                if (item.get() == nullptr || !fun(item.get(), n)) {
                    return false;
                }
            }
            return true;
        }
        auto seq = steal(PySequence_Tuple(obj));
        if (seq.get() == nullptr) {
            return false;
        }
        auto n = PyTuple_Size(seq.get());
        for (Py_ssize_t i = 0; i < n; ++i) {
            auto *item = PyTuple_GetItem(seq.get(), i);
            if (item == nullptr || !fun(item, n)) {
                return false;
            }
        }
        return true;
    } catch (std::bad_alloc const &) {
        PyErr_NoMemory();
        return false;
    }
}

auto parse_literals(PyObject *obj, bool negate, LitVec &out) -> bool {
    return for_each_item(obj, [&](PyObject *item, Py_ssize_t n) {
        if (out.empty()) {
            out.reserve(static_cast<size_t>(n));
        }
        clingo_literal_t lit = 0;
        if (!parse_num(item, &lit, "literal")) {
            return false;
        }
        out.push_back(negate ? -lit : lit);
        return true;
    });
}

auto parse_weighted_literals(PyObject *obj, WeightLitVec &out) -> bool {
    static constexpr char const *msg = "weighted literal must be a pair (literal, weight)";
    return for_each_item(obj, [&](PyObject *item, Py_ssize_t n) {
        if (out.empty()) {
            out.reserve(static_cast<size_t>(n));
        }
        auto pair = steal(PySequence_Tuple(item));
        if (pair.get() == nullptr) {
            PyErr_Clear();
            PyErr_SetString(PyExc_TypeError, msg);
            return false;
        }
        if (PyTuple_Size(pair.get()) != 2) {
            PyErr_SetString(PyExc_TypeError, msg);
            return false;
        }
        auto wlit = clingo_weighted_literal_t{0, 0};
        if (!parse_num(PyTuple_GetItem(pair.get(), 0), &wlit.literal, "literal") ||
            !parse_num(PyTuple_GetItem(pair.get(), 1), &wlit.weight, "weight")) {
            return false;
        }
        out.push_back(wlit);
        return true;
    });
}

auto int_list(clingo_literal_t const *values, size_t size) -> PyObject * {
    auto list = steal(PyList_New(static_cast<Py_ssize_t>(size)));
    if (list.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < size; ++i) {
        auto *value = PyLong_FromLong(values[i]);
        if (value == nullptr || PyList_SetItem(list.get(), static_cast<Py_ssize_t>(i), value) < 0) {
            return nullptr;
        }
    }
    return list.release();
}

// NOTE: res is taken by reference because it is usually written by the call producing ok
auto bool_result(bool ok, bool const &res) -> PyObject * {
    if (!ok) {
        return py_handle_error();
    }
    return PyBool_FromLong(res ? 1 : 0);
}

auto none_result(bool ok) -> PyObject * {
    if (!ok) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

// sequence helpers

auto normalize_index(PyObject *key, Py_ssize_t size, Py_ssize_t *index) -> bool {
    auto i = PyNumber_AsSsize_t(key, PyExc_IndexError);
    if (i == -1 && PyErr_Occurred() != nullptr) {
        return false;
    }
    if (i < 0) {
        i += size;
    }
    if (i < 0 || i >= size) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return false;
    }
    *index = i;
    return true;
}

auto check_index(Py_ssize_t index, Py_ssize_t size) -> bool {
    if (index < 0 || index >= size) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return false;
    }
    return true;
}

auto unpack_slice(PyObject *slice, Py_ssize_t size, Py_ssize_t *start, Py_ssize_t *step, Py_ssize_t *len) -> bool {
    Py_ssize_t stop = 0;
    if (PySlice_Unpack(slice, start, &stop, step) < 0) {
        return false;
    }
    *len = PySlice_AdjustIndices(size, start, &stop, *step);
    return true;
}

auto invalid_key() -> PyObject * {
    PyErr_SetString(PyExc_TypeError, "indices must be integers or slices");
    return nullptr;
}

// Assignment

auto assignment_get(PyObject *self) -> clingo_assignment_t const * {
    auto const *ass = as<PyAssignmentObject>(self)->ass;
    if (ass == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment is no longer valid");
    }
    return ass;
}

auto assignment_size(clingo_assignment_t const *ass) -> Py_ssize_t {
    size_t size = 0;
    if (!clingo_assignment_size(ass, &size)) {
        py_handle_error();
        return -1;
    }
    return static_cast<Py_ssize_t>(size);
}

auto assignment_at(clingo_assignment_t const *ass, Py_ssize_t index) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!clingo_assignment_at(ass, static_cast<size_t>(index), &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

using LitFlagFun = bool (*)(clingo_assignment_t const *, clingo_literal_t, bool *);

auto assignment_lit_flag(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames, char const *fname,
                         LitFlagFun fun) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!parse_literal_arg(fname, args, nargs, kwnames, &lit)) {
        return nullptr;
    }
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    bool res = false;
    return bool_result(fun(ass, lit, &res), res);
}

auto assignment_truth_value(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames,
                            char const *fname, clingo_truth_value_t *value) -> bool {
    clingo_literal_t lit = 0;
    if (!parse_literal_arg(fname, args, nargs, kwnames, &lit)) {
        return false;
    }
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return false;
    }
    if (!clingo_assignment_truth_value(ass, lit, value)) {
        py_handle_error();
        return false;
    }
    return true;
}

auto py_assignment_is_true(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    return assignment_lit_flag(self, args, nargs, kwnames, "is_true", clingo_assignment_is_true);
}

auto py_assignment_is_false(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    return assignment_lit_flag(self, args, nargs, kwnames, "is_false", clingo_assignment_is_false);
}

auto py_assignment_is_fixed(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    return assignment_lit_flag(self, args, nargs, kwnames, "is_fixed", clingo_assignment_is_fixed);
}

auto py_assignment_is_free(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    clingo_truth_value_t value = 0;
    if (!assignment_truth_value(self, args, nargs, kwnames, "is_free", &value)) {
        return nullptr;
    }
    return PyBool_FromLong(value == clingo_truth_value_free ? 1 : 0);
}

auto py_assignment_value(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    clingo_truth_value_t value = 0;
    if (!assignment_truth_value(self, args, nargs, kwnames, "value", &value)) {
        return nullptr;
    }
    switch (value) {
        case clingo_truth_value_true: {
            Py_RETURN_TRUE;
        }
        case clingo_truth_value_false: {
            Py_RETURN_FALSE;
        }
        default: {
            Py_RETURN_NONE;
        }
    }
}

auto py_assignment_level(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!parse_literal_arg("level", args, nargs, kwnames, &lit)) {
        return nullptr;
    }
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    uint32_t level = 0;
    if (!clingo_assignment_level(ass, lit, &level)) {
        return py_handle_error();
    }
    return PyLong_FromUnsignedLong(level);
}

auto py_assignment_decision(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    uint32_t level = 0;
    if (!parse_level_arg("decision", args, nargs, kwnames, &level)) {
        return nullptr;
    }
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    clingo_literal_t lit = 0;
    if (!clingo_assignment_decision(ass, level, &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

auto py_assignment_thread_id(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    clingo_id_t id = 0;
    if (!clingo_assignment_thread_id(ass, &id)) {
        return py_handle_error();
    }
    return PyLong_FromUnsignedLong(id);
}

auto py_assignment_decision_level(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    uint32_t level = 0;
    if (!clingo_assignment_decision_level(ass, &level)) {
        return py_handle_error();
    }
    return PyLong_FromUnsignedLong(level);
}

auto py_assignment_root_level(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    uint32_t level = 0;
    if (!clingo_assignment_root_level(ass, &level)) {
        return py_handle_error();
    }
    return PyLong_FromUnsignedLong(level);
}

auto py_assignment_has_conflict(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    bool res = false;
    return bool_result(clingo_assignment_has_conflict(ass, &res), res);
}

auto py_assignment_is_total(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    bool res = false;
    return bool_result(clingo_assignment_is_total(ass, &res), res);
}

auto py_assignment_trail(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    if (assignment_get(self) == nullptr) {
        return nullptr;
    }
    auto *a = as<PyAssignmentObject>(self);
    if (a->trail == nullptr) {
        auto *state = state_of(self);
        if (state == nullptr) {
            return nullptr;
        }
        auto *trail = alloc_object(state->trail_type);
        if (trail == nullptr) {
            return nullptr;
        }
        Py_INCREF(self);
        as<PyTrailObject>(trail)->assignment = self;
        a->trail = trail;
    }
    Py_INCREF(a->trail);
    return a->trail;
}

auto py_assignment_len(PyObject *self) -> Py_ssize_t {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return -1;
    }
    return assignment_size(ass);
}

auto py_assignment_item(PyObject *self, Py_ssize_t index) -> PyObject * {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    auto size = assignment_size(ass);
    if (size < 0 || !check_index(index, size)) {
        return nullptr;
    }
    return assignment_at(ass, index);
}

auto py_assignment_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    auto size = assignment_size(ass);
    if (size < 0) {
        return nullptr;
    }
    if (PySlice_Check(key) != 0) {
        Py_ssize_t start = 0;
        Py_ssize_t step = 0;
        Py_ssize_t len = 0;
        if (!unpack_slice(key, size, &start, &step, &len)) {
            return nullptr;
        }
        auto list = steal(PyList_New(len));
        if (list.get() == nullptr) {
            return nullptr;
        }
        for (Py_ssize_t i = 0; i < len; ++i) {
            auto *lit = assignment_at(ass, start + (i * step));
            if (lit == nullptr || PyList_SetItem(list.get(), i, lit) < 0) {
                return nullptr;
            }
        }
        return list.release();
    }
    if (PyIndex_Check(key) == 0) {
        return invalid_key();
    }
    Py_ssize_t index = 0;
    if (!normalize_index(key, size, &index)) {
        return nullptr;
    }
    return assignment_at(ass, index);
}

auto py_assignment_contains(PyObject *self, PyObject *value) -> int {
    auto const *ass = assignment_get(self);
    if (ass == nullptr) {
        return -1;
    }
    if (PyLong_Check(value) == 0) {
        return 0;
    }
    clingo_literal_t lit = 0;
    if (!parse_num(value, &lit, "literal")) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError) != 0) {
            PyErr_Clear();
            return 0;
        }
        return -1;
    }
    bool res = false;
    if (!clingo_assignment_has_literal(ass, lit, &res)) {
        py_handle_error();
        return -1;
    }
    return res ? 1 : 0;
}

auto py_assignment_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(as<PyAssignmentObject>(self)->trail);
    return 0;
}

auto py_assignment_clear(PyObject *self) -> int {
    Py_CLEAR(as<PyAssignmentObject>(self)->trail);
    return 0;
}

auto py_assignment_methods = std::array{
    PyMethodDef{
        "decision",
        fast_cast(py_assignment_decision),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the decision literal of the given level."),
    },
    PyMethodDef{
        "is_false",
        fast_cast(py_assignment_is_false),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if the given literal is false."),
    },
    PyMethodDef{
        "is_true",
        fast_cast(py_assignment_is_true),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if the given literal is true."),
    },
    PyMethodDef{
        "is_free",
        fast_cast(py_assignment_is_free),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if the given literal is free."),
    },
    PyMethodDef{
        "is_fixed",
        fast_cast(py_assignment_is_fixed),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if the truth value of the given literal is fixed."),
    },
    PyMethodDef{
        "level",
        fast_cast(py_assignment_level),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the decision level of the given literal."),
    },
    PyMethodDef{
        "value",
        fast_cast(py_assignment_value),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the truth value of the given literal or None if it is unassigned."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_assignment_getset = std::array{
    PyGetSetDef{
        "thread_id",
        py_assignment_thread_id,
        nullptr,
        PyDoc_STR("The id of the thread the assignment belongs to."),
        nullptr,
    },
    PyGetSetDef{
        "decision_level",
        py_assignment_decision_level,
        nullptr,
        PyDoc_STR("The current decision level."),
        nullptr,
    },
    PyGetSetDef{
        "has_conflict",
        py_assignment_has_conflict,
        nullptr,
        PyDoc_STR("Whether the assignment is conflicting."),
        nullptr,
    },
    PyGetSetDef{"is_total", py_assignment_is_total, nullptr, PyDoc_STR("Whether all literals are assigned."), nullptr},
    PyGetSetDef{
        "root_level",
        py_assignment_root_level,
        nullptr,
        PyDoc_STR("The lowest decision level that can be backtracked to."),
        nullptr,
    },
    PyGetSetDef{"trail", py_assignment_trail, nullptr, PyDoc_STR("The trail of assigned literals."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_assignment_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("The state of the literals in the solver.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_assignment_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_assignment_clear)},
    PyType_Slot{Py_tp_methods, py_assignment_methods.data()},
    PyType_Slot{Py_tp_getset, py_assignment_getset.data()},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(py_assignment_len)},
    PyType_Slot{Py_sq_item, reinterpret_cast<void *>(py_assignment_item)},
    PyType_Slot{Py_sq_contains, reinterpret_cast<void *>(py_assignment_contains)},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_assignment_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_assignment_subscript)},
    PyType_Slot{0, nullptr},
};

auto py_assignment_spec = PyType_Spec{
    "clingo.propagate.Assignment",
    sizeof(PyAssignmentObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_assignment_slots.data(),
};

// Trail and _TrailView

auto view_new(PropagateModuleState *state, PyObject *assignment, Py_ssize_t start, Py_ssize_t step, Py_ssize_t size)
    -> PyObject * {
    auto *obj = alloc_object(state->trail_view_type);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *v = as<PyTrailViewObject>(obj);
    Py_INCREF(assignment);
    v->assignment = assignment;
    v->start = start;
    v->step = step;
    v->size = size;
    return obj;
}

auto trail_size(clingo_assignment_t const *ass) -> Py_ssize_t {
    uint32_t size = 0;
    if (!clingo_assignment_trail_size(ass, &size)) {
        py_handle_error();
        return -1;
    }
    return static_cast<Py_ssize_t>(size);
}

auto trail_at(clingo_assignment_t const *ass, Py_ssize_t offset) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!clingo_assignment_trail_at(ass, static_cast<uint32_t>(offset), &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

auto trail_get(PyObject *self) -> clingo_assignment_t const * {
    return assignment_get(as<PyTrailObject>(self)->assignment);
}

auto py_trail_len(PyObject *self) -> Py_ssize_t {
    auto const *ass = trail_get(self);
    if (ass == nullptr) {
        return -1;
    }
    return trail_size(ass);
}

auto py_trail_item(PyObject *self, Py_ssize_t index) -> PyObject * {
    auto const *ass = trail_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    auto size = trail_size(ass);
    if (size < 0 || !check_index(index, size)) {
        return nullptr;
    }
    return trail_at(ass, index);
}

auto py_trail_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto const *ass = trail_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    auto size = trail_size(ass);
    if (size < 0) {
        return nullptr;
    }
    if (PySlice_Check(key) != 0) {
        auto *state = state_of(self);
        if (state == nullptr) {
            return nullptr;
        }
        Py_ssize_t start = 0;
        Py_ssize_t step = 0;
        Py_ssize_t len = 0;
        if (!unpack_slice(key, size, &start, &step, &len)) {
            return nullptr;
        }
        return view_new(state, as<PyTrailObject>(self)->assignment, start, step, len);
    }
    if (PyIndex_Check(key) == 0) {
        return invalid_key();
    }
    Py_ssize_t index = 0;
    if (!normalize_index(key, size, &index)) {
        return nullptr;
    }
    return trail_at(ass, index);
}

auto trail_level(PyObject *self, char const *fname, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames,
                 uint32_t *begin, uint32_t *end) -> bool {
    uint32_t level = 0;
    if (!parse_level_arg(fname, args, nargs, kwnames, &level)) {
        return false;
    }
    auto const *ass = trail_get(self);
    if (ass == nullptr) {
        return false;
    }
    if ((begin != nullptr && !clingo_assignment_trail_begin(ass, level, begin)) ||
        (end != nullptr && !clingo_assignment_trail_end(ass, level, end))) {
        py_handle_error();
        return false;
    }
    return true;
}

auto py_trail_level(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    uint32_t begin = 0;
    uint32_t end = 0;
    if (!trail_level(self, "level", args, nargs, kwnames, &begin, &end)) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto size = end > begin ? static_cast<Py_ssize_t>(end - begin) : 0;
    return view_new(state, as<PyTrailObject>(self)->assignment, static_cast<Py_ssize_t>(begin), 1, size);
}

auto py_trail_begin(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    uint32_t begin = 0;
    if (!trail_level(self, "begin", args, nargs, kwnames, &begin, nullptr)) {
        return nullptr;
    }
    return PyLong_FromUnsignedLong(begin);
}

auto py_trail_end(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    uint32_t end = 0;
    if (!trail_level(self, "end", args, nargs, kwnames, nullptr, &end)) {
        return nullptr;
    }
    return PyLong_FromUnsignedLong(end);
}

auto py_trail_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(as<PyTrailObject>(self)->assignment);
    return 0;
}

auto py_trail_clear(PyObject *self) -> int {
    Py_CLEAR(as<PyTrailObject>(self)->assignment);
    return 0;
}

auto py_trail_methods = std::array{
    PyMethodDef{
        "level",
        fast_cast(py_trail_level),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the literals assigned at the given decision level."),
    },
    PyMethodDef{
        "begin",
        fast_cast(py_trail_begin),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the index of the first literal on the given level."),
    },
    PyMethodDef{
        "end",
        fast_cast(py_trail_end),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Get the index after the last literal on the given level."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_trail_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("The sequence of literals assigned by the solver.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_trail_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_trail_clear)},
    PyType_Slot{Py_tp_methods, py_trail_methods.data()},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(py_trail_len)},
    PyType_Slot{Py_sq_item, reinterpret_cast<void *>(py_trail_item)},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_trail_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_trail_subscript)},
    PyType_Slot{0, nullptr},
};

auto py_trail_spec = PyType_Spec{
    "clingo.propagate.Trail",
    sizeof(PyTrailObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_trail_slots.data(),
};

auto view_get(PyObject *self) -> clingo_assignment_t const * {
    return assignment_get(as<PyTrailViewObject>(self)->assignment);
}

auto view_at(PyObject *self, clingo_assignment_t const *ass, Py_ssize_t index) -> PyObject * {
    auto *v = as<PyTrailViewObject>(self);
    return trail_at(ass, v->start + (index * v->step));
}

auto py_view_len(PyObject *self) -> Py_ssize_t {
    if (view_get(self) == nullptr) {
        return -1;
    }
    return as<PyTrailViewObject>(self)->size;
}

auto py_view_item(PyObject *self, Py_ssize_t index) -> PyObject * {
    auto const *ass = view_get(self);
    if (ass == nullptr || !check_index(index, as<PyTrailViewObject>(self)->size)) {
        return nullptr;
    }
    return view_at(self, ass, index);
}

auto py_view_subscript(PyObject *self, PyObject *key) -> PyObject * {
    auto const *ass = view_get(self);
    if (ass == nullptr) {
        return nullptr;
    }
    auto *v = as<PyTrailViewObject>(self);
    if (PySlice_Check(key) != 0) {
        auto *state = state_of(self);
        if (state == nullptr) {
            return nullptr;
        }
        Py_ssize_t start = 0;
        Py_ssize_t step = 0;
        Py_ssize_t len = 0;
        if (!unpack_slice(key, v->size, &start, &step, &len)) {
            return nullptr;
        }
        return view_new(state, v->assignment, v->start + (start * v->step), v->step * step, len);
    }
    if (PyIndex_Check(key) == 0) {
        return invalid_key();
    }
    Py_ssize_t index = 0;
    if (!normalize_index(key, v->size, &index)) {
        return nullptr;
    }
    return view_at(self, ass, index);
}

auto py_view_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(as<PyTrailViewObject>(self)->assignment);
    return 0;
}

auto py_view_clear(PyObject *self) -> int {
    Py_CLEAR(as<PyTrailViewObject>(self)->assignment);
    return 0;
}

auto py_view_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A view on a subrange of the solver's trail.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_view_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_view_clear)},
    PyType_Slot{Py_sq_length, reinterpret_cast<void *>(py_view_len)},
    PyType_Slot{Py_sq_item, reinterpret_cast<void *>(py_view_item)},
    PyType_Slot{Py_mp_length, reinterpret_cast<void *>(py_view_len)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_view_subscript)},
    PyType_Slot{0, nullptr},
};

auto py_view_spec = PyType_Spec{
    "clingo.propagate._TrailView",
    sizeof(PyTrailViewObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_view_slots.data(),
};

// PropagateControl

auto control_get(PyObject *self) -> clingo_propagate_control_t * {
    auto *ctl = as<PyPropagateControlObject>(self)->ctl;
    if (ctl == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "propagate control is no longer valid");
    }
    return ctl;
}

auto control_add_clause(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames, char const *fname,
                        bool negate) -> PyObject * {
    static constexpr auto names = std::array<char const *, 3>{"literals", "tag", "lock"};
    auto out = std::array<PyObject *, 3>{};
    if (!parse_args(fname, names, 1, args, nargs, kwnames, out)) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto tag = false;
    auto lock = false;
    if (!parse_bool(out[1], &tag) || !parse_bool(out[2], &lock)) {
        return nullptr;
    }
    auto lease = BufferLease{lit_pool};
    auto &lits = lease.get();
    if (!parse_literals(out[0], negate, lits)) {
        return nullptr;
    }
    clingo_clause_type_t type = 0;
    if (tag) {
        type |= clingo_clause_type_volatile;
    }
    if (lock) {
        type |= clingo_clause_type_static;
    }
    auto res = false;
    return bool_result(clingo_propagate_control_add_clause(ctl, lits.data(), lits.size(), type, &res), res);
}

auto py_control_add_clause(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    return control_add_clause(self, args, nargs, kwnames, "add_clause", false);
}

auto py_control_add_nogood(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    return control_add_clause(self, args, nargs, kwnames, "add_nogood", true);
}

auto py_control_add_weight_constraint(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    -> PyObject * {
    static constexpr auto names = std::array<char const *, 4>{"literal", "literals", "bound", "type"};
    auto out = std::array<PyObject *, 4>{};
    if (!parse_args("add_weight_constraint", names, 3, args, nargs, kwnames, out)) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    clingo_literal_t lit = 0;
    clingo_weight_t bound = 0;
    int type = clingo_weight_constraint_type_equivalence;
    if (!parse_num(out[0], &lit, "literal") || !parse_num(out[2], &bound, "bound") ||
        !parse_num(out[3], &type, "type")) {
        return nullptr;
    }
    auto lease = BufferLease{wlit_pool};
    auto &wlits = lease.get();
    if (!parse_weighted_literals(out[1], wlits)) {
        return nullptr;
    }
    auto res = false;
    return bool_result(
        clingo_propagate_control_add_weight_constraint(ctl, lit, wlits.data(), wlits.size(), bound,
                                                       static_cast<clingo_weight_constraint_type_e>(type), &res),
        res);
}

auto py_control_add_literal(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    static constexpr auto names = std::array<char const *, 1>{"freeze"};
    auto out = std::array<PyObject *, 1>{};
    if (!parse_args("add_literal", names, 0, args, nargs, kwnames, out)) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto freeze = true;
    if (!parse_bool(out[0], &freeze)) {
        return nullptr;
    }
    clingo_literal_t lit = 0;
    if (!clingo_propagate_control_add_literal(ctl, freeze, &lit)) {
        return py_handle_error();
    }
    return PyLong_FromLong(lit);
}

auto py_control_add_watch(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!parse_literal_arg("add_watch", args, nargs, kwnames, &lit)) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    return none_result(clingo_propagate_control_add_watch(ctl, lit));
}

auto py_control_has_watch(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!parse_literal_arg("has_watch", args, nargs, kwnames, &lit)) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto res = false;
    return bool_result(clingo_propagate_control_has_watch(ctl, lit, &res), res);
}

auto py_control_remove_watch(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!parse_literal_arg("remove_watch", args, nargs, kwnames, &lit)) {
        return nullptr;
    }
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    return none_result(clingo_propagate_control_remove_watch(ctl, lit));
}

auto py_control_propagate(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *ctl = control_get(self);
    if (ctl == nullptr) {
        return nullptr;
    }
    auto res = false;
    return bool_result(clingo_propagate_control_propagate(ctl, &res), res);
}

auto py_control_methods = std::array{
    PyMethodDef{
        "add_clause",
        fast_cast(py_control_add_clause),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a clause and return whether it could be integrated without conflict."),
    },
    PyMethodDef{
        "add_weight_constraint",
        fast_cast(py_control_add_weight_constraint),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a weight constraint and return whether it could be added without conflict."),
    },
    PyMethodDef{
        "add_literal",
        fast_cast(py_control_add_literal),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a fresh literal to the solver."),
    },
    PyMethodDef{
        "add_nogood",
        fast_cast(py_control_add_nogood),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("A shortcut for `add_clause([-literal for literal in literals], tag, lock)`."),
    },
    PyMethodDef{
        "add_watch",
        fast_cast(py_control_add_watch),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a watch for the given solver literal."),
    },
    PyMethodDef{
        "has_watch",
        fast_cast(py_control_has_watch),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check if a watch exists for the given solver literal."),
    },
    PyMethodDef{
        "propagate",
        py_control_propagate,
        METH_NOARGS,
        PyDoc_STR("Propagate and return False if the propagator must return immediately."),
    },
    PyMethodDef{
        "remove_watch",
        fast_cast(py_control_remove_watch),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Remove the watch for the given solver literal."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_control_slots = std::array{
    PyType_Slot{Py_tp_doc,
                const_cast<char *>("An object to add clauses, literals, and watches while solving.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_methods, py_control_methods.data()},
    PyType_Slot{0, nullptr},
};

// NOTE: BASETYPE is required to derive PropagateInit from PropagateControl
auto py_control_spec = PyType_Spec{
    "clingo.propagate.PropagateControl",
    sizeof(PyPropagateControlObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_BASETYPE,
    py_control_slots.data(),
};

// PropagateInit

auto init_get(PyObject *self) -> clingo_propagate_init_t * {
    auto *init = as<PyPropagateInitObject>(self)->init;
    if (init == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "propagate init is no longer valid");
    }
    return init;
}

auto py_init_add_minimize(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    static constexpr auto names = std::array<char const *, 3>{"literal", "weight", "priority"};
    auto out = std::array<PyObject *, 3>{};
    if (!parse_args("add_minimize", names, 2, args, nargs, kwnames, out)) {
        return nullptr;
    }
    auto *init = init_get(self);
    if (init == nullptr) {
        return nullptr;
    }
    clingo_literal_t lit = 0;
    clingo_weight_t weight = 0;
    clingo_weight_t priority = 0;
    if (!parse_num(out[0], &lit, "literal") || !parse_num(out[1], &weight, "weight") ||
        !parse_num(out[2], &priority, "priority")) {
        return nullptr;
    }
    return none_result(clingo_propagate_init_add_minimize(init, lit, weight, priority));
}

auto py_init_freeze_literal(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    clingo_literal_t lit = 0;
    if (!parse_literal_arg("freeze_literal", args, nargs, kwnames, &lit)) {
        return nullptr;
    }
    auto *init = init_get(self);
    if (init == nullptr) {
        return nullptr;
    }
    return none_result(clingo_propagate_init_freeze_literal(init, lit));
}

auto py_init_solver_literal(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    static constexpr auto names = std::array<char const *, 1>{"literal"};
    auto out = std::array<PyObject *, 1>{};
    if (!parse_args("solver_literal", names, 1, args, nargs, kwnames, out)) {
        return nullptr;
    }
    auto *init = init_get(self);
    if (init == nullptr) {
        return nullptr;
    }
    clingo_literal_t lit = 0;
    if (out[0] != Py_None && !parse_num(out[0], &lit, "literal")) {
        return nullptr;
    }
    clingo_literal_t res = 0;
    if (!clingo_propagate_init_solver_literal(init, lit, &res)) {
        return py_handle_error();
    }
    return PyLong_FromLong(res);
}

auto py_init_library(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *library = as<PyPropagateInitObject>(self)->library;
    if (library == nullptr) {
        Py_RETURN_NONE;
    }
    Py_INCREF(library);
    return library;
}

auto py_init_base(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *init = init_get(self);
    if (init == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_base_t const *base = nullptr;
    if (!clingo_propagate_init_base(init, &base)) {
        return py_handle_error();
    }
    return py_base_new(self, nullptr, state->base_type, base);
}

auto py_init_get_check_mode(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *init = init_get(self);
    if (init == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_propagator_check_mode_t mode = 0;
    if (!clingo_propagate_init_get_check_mode(init, &mode)) {
        return py_handle_error();
    }
    return PyObject_CallFunction(state->check_mode_enum, "i", static_cast<int>(mode));
}

auto py_init_set_check_mode(PyObject *self, PyObject *value, [[maybe_unused]] void *closure) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "cannot delete check_mode");
        return -1;
    }
    auto *init = init_get(self);
    if (init == nullptr) {
        return -1;
    }
    int mode = 0;
    if (!parse_num(value, &mode, "check_mode")) {
        return -1;
    }
    if (!clingo_propagate_init_set_check_mode(init, static_cast<clingo_propagator_check_mode_e>(mode))) {
        py_handle_error();
        return -1;
    }
    return 0;
}

auto py_init_get_undo_mode(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *init = init_get(self);
    if (init == nullptr) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    clingo_propagator_check_mode_t mode = 0;
    if (!clingo_propagate_init_get_undo_mode(init, &mode)) {
        return py_handle_error();
    }
    return PyObject_CallFunction(state->undo_mode_enum, "i", static_cast<int>(mode));
}

auto py_init_set_undo_mode(PyObject *self, PyObject *value, [[maybe_unused]] void *closure) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "cannot delete undo_mode");
        return -1;
    }
    auto *init = init_get(self);
    if (init == nullptr) {
        return -1;
    }
    int mode = 0;
    if (!parse_num(value, &mode, "undo_mode")) {
        return -1;
    }
    if (!clingo_propagate_init_set_undo_mode(init, static_cast<clingo_propagator_undo_mode_e>(mode))) {
        py_handle_error();
        return -1;
    }
    return 0;
}

auto py_init_number_of_threads(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *init = init_get(self);
    if (init == nullptr) {
        return nullptr;
    }
    clingo_id_t res = 0;
    if (!clingo_propagate_init_number_of_threads(init, &res)) {
        return py_handle_error();
    }
    return PyLong_FromUnsignedLong(res);
}

auto py_init_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(as<PyPropagateInitObject>(self)->library);
    return 0;
}

auto py_init_clear(PyObject *self) -> int {
    Py_CLEAR(as<PyPropagateInitObject>(self)->library);
    return 0;
}

auto py_init_methods = std::array{
    PyMethodDef{
        "add_minimize",
        fast_cast(py_init_add_minimize),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a weighted literal to minimize to the solver."),
    },
    PyMethodDef{
        "freeze_literal",
        fast_cast(py_init_freeze_literal),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Freeze the given literal to exempt it from simplification."),
    },
    PyMethodDef{
        "solver_literal",
        fast_cast(py_init_solver_literal),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Map the given program literal or None for true to a solver literal."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_init_getset = std::array{
    PyGetSetDef{"library", py_init_library, nullptr, PyDoc_STR("The library object managing symbols."), nullptr},
    PyGetSetDef{"base", py_init_base, nullptr, PyDoc_STR("The base object to inspect the grounder's base."), nullptr},
    PyGetSetDef{
        "check_mode",
        py_init_get_check_mode,
        py_init_set_check_mode,
        PyDoc_STR("Get/set the check mode for the propagator."),
        nullptr,
    },
    PyGetSetDef{
        "number_of_threads",
        py_init_number_of_threads,
        nullptr,
        PyDoc_STR("The number of solver threads."),
        nullptr,
    },
    PyGetSetDef{
        "undo_mode",
        py_init_get_undo_mode,
        py_init_set_undo_mode,
        PyDoc_STR("Get/set the undo mode for the propagator."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_init_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("An object to initialize a propagator.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_init_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_init_clear)},
    PyType_Slot{Py_tp_methods, py_init_methods.data()},
    PyType_Slot{Py_tp_getset, py_init_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_init_spec = PyType_Spec{
    "clingo.propagate.PropagateInit",
    sizeof(PyPropagateInitObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_init_slots.data(),
};

// Propagator

template <size_t N>
auto propagator_default(char const *fname, std::array<char const *, N> const &names, PyObject *const *args,
                        Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    auto out = std::array<PyObject *, N>{};
    if (!parse_args(fname, names, N, args, nargs, kwnames, out)) {
        return nullptr;
    }
    Py_RETURN_NONE;
}

auto py_propagator_init([[maybe_unused]] PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    -> PyObject * {
    static constexpr auto names = std::array<char const *, 2>{"assignment", "init"};
    return propagator_default("init", names, args, nargs, kwnames);
}

auto py_propagator_attach([[maybe_unused]] PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    -> PyObject * {
    static constexpr auto names = std::array<char const *, 2>{"assignment", "control"};
    return propagator_default("attach", names, args, nargs, kwnames);
}

auto py_propagator_propagate([[maybe_unused]] PyObject *self, PyObject *const *args, Py_ssize_t nargs,
                             PyObject *kwnames) -> PyObject * {
    static constexpr auto names = std::array<char const *, 3>{"assignment", "control", "changes"};
    return propagator_default("propagate", names, args, nargs, kwnames);
}

auto py_propagator_undo([[maybe_unused]] PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    -> PyObject * {
    static constexpr auto names = std::array<char const *, 2>{"assignment", "changes"};
    return propagator_default("undo", names, args, nargs, kwnames);
}

auto py_propagator_check([[maybe_unused]] PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    -> PyObject * {
    static constexpr auto names = std::array<char const *, 2>{"assignment", "control"};
    return propagator_default("check", names, args, nargs, kwnames);
}

auto py_propagator_decide([[maybe_unused]] PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
    -> PyObject * {
    static constexpr auto names = std::array<char const *, 2>{"assignment", "fallback"};
    auto out = std::array<PyObject *, 2>{};
    if (!parse_args("decide", names, 2, args, nargs, kwnames, out)) {
        return nullptr;
    }
    Py_INCREF(out[1]);
    return out[1];
}

auto py_propagator_methods = std::array{
    PyMethodDef{
        "init",
        fast_cast(py_propagator_init),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Initialize the propagator before each solving step."),
    },
    PyMethodDef{
        "attach",
        fast_cast(py_propagator_attach),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Initialize a solver thread before solving starts."),
    },
    PyMethodDef{
        "propagate",
        fast_cast(py_propagator_propagate),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Propagate the given non-empty list of changed watched literals."),
    },
    PyMethodDef{
        "undo",
        fast_cast(py_propagator_undo),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Undo the assignment of the given literals."),
    },
    PyMethodDef{
        "check",
        fast_cast(py_propagator_check),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Check the assignment on fixpoints or total assignments."),
    },
    PyMethodDef{
        "decide",
        fast_cast(py_propagator_decide),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Return the literal to decide on next or 0 to make no decision."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_propagator_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("Base class for custom propagators.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_methods, py_propagator_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_propagator_spec = PyType_Spec{
    "clingo.propagate.Propagator", sizeof(PyObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    py_propagator_slots.data(),
};

// _PropagatorHandle

auto handle_cast(void *data) -> PyPropagatorHandleObject * {
    return as<PyPropagatorHandleObject>(static_cast<PyObject *>(data));
}

void handle_clear_cache(PyPropagatorHandleObject *h) {
    auto *cache = std::exchange(h->cache, nullptr);
    auto size = std::exchange(h->cache_size, 0);
    for (size_t i = 0; i < size; ++i) {
        Py_XDECREF(cache[i]);
    }
    PyMem_Free(static_cast<void *>(cache));
}

auto py_handle_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *h = as<PyPropagatorHandleObject>(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(h->propagator);
    Py_VISIT(h->library);
    for (size_t i = 0; i < h->cache_size; ++i) {
        Py_VISIT(h->cache[i]);
    }
    return 0;
}

auto py_handle_clear(PyObject *self) -> int {
    auto *h = as<PyPropagatorHandleObject>(self);
    // NOTE: running callbacks hold their own references
    Py_CLEAR(h->propagator);
    Py_CLEAR(h->library);
    handle_clear_cache(h);
    return 0;
}

auto py_handle_slots = std::array{
    PyType_Slot{Py_tp_doc,
                const_cast<char *>("Internal object connecting a propagator to a control object.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_handle_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_handle_clear)},
    PyType_Slot{0, nullptr},
};

auto py_handle_spec = PyType_Spec{
    "clingo.propagate._PropagatorHandle",
    sizeof(PyPropagatorHandleObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_handle_slots.data(),
};

//! Get the cached object at the given index creating it if necessary (borrowed).
auto handle_cached(PyPropagatorHandleObject *h, PyObject *type, size_t index) -> PyObject * {
    if (index >= h->cache_size) {
        auto size = std::max(index + 1, 2 * h->cache_size);
        auto *cache = static_cast<PyObject **>(PyMem_Realloc(static_cast<void *>(h->cache), size * sizeof(PyObject *)));
        if (cache == nullptr) {
            PyErr_NoMemory();
            return nullptr;
        }
        std::fill(cache + h->cache_size, cache + size, nullptr);
        h->cache = cache;
        h->cache_size = size;
    }
    auto *&slot = h->cache[index];
    if (slot == nullptr) {
        slot = alloc_object(type);
    }
    return slot;
}

// callbacks

//! Bind a wrapper to a clingo object for the duration of a callback.
//!
//! Restoring the previous pointer (instead of clearing it) keeps nested
//! callbacks of the same thread working.
template <class T, class P, P *T::*Member> class Binding {
  public:
    Binding(PyObject *obj, P *ptr) : obj_{borrow(obj)}, prev_{std::exchange(as<T>(obj)->*Member, ptr)} {}
    Binding(Binding const &) = delete;
    Binding(Binding &&) = delete;
    auto operator=(Binding const &) -> Binding & = delete;
    auto operator=(Binding &&) -> Binding & = delete;
    ~Binding() { as<T>(obj_.get())->*Member = prev_; }

  private:
    PyGuard obj_;
    P *prev_;
};

using AssignmentBinding = Binding<PyAssignmentObject, clingo_assignment_t const, &PyAssignmentObject::ass>;
using ControlBinding = Binding<PyPropagateControlObject, clingo_propagate_control_t, &PyPropagateControlObject::ctl>;
using InitBinding = Binding<PyPropagateInitObject, clingo_propagate_init_t, &PyPropagateInitObject::init>;

struct Call {
    PyPropagatorHandleObject *handle{nullptr};
    PropagateModuleState *state{nullptr};
    PyGuard propagator{nullptr};
};

//! Acquire the propagator and module state; must be called with the GIL held.
auto call_begin(void *data, Call &call) -> bool {
    auto *self = static_cast<PyObject *>(data);
    call.handle = handle_cast(data);
    call.propagator = borrow(call.handle->propagator);
    if (call.propagator.get() == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "propagator has been released");
        return false;
    }
    call.state = state_of(self);
    return call.state != nullptr;
}

//! Get the cached assignment and (optionally) control wrappers of the thread (borrowed).
auto thread_objects(Call &call, clingo_assignment_t const *ass, PyObject **ass_obj, PyObject **ctl_obj) -> bool {
    clingo_id_t id = 0;
    if (!clingo_assignment_thread_id(ass, &id)) {
        py_handle_error();
        return false;
    }
    auto index = 2 * static_cast<size_t>(id);
    *ass_obj = handle_cached(call.handle, call.state->assignment_type, index);
    if (*ass_obj == nullptr) {
        return false;
    }
    if (ctl_obj != nullptr) {
        *ctl_obj = handle_cached(call.handle, call.state->control_type, index + 1);
        if (*ctl_obj == nullptr) {
            return false;
        }
    }
    return true;
}

auto init_cb(clingo_assignment_t const *ass, clingo_propagate_init_t *init, void *data) -> bool {
    if (!handle_cast(data)->has_init) {
        return true;
    }
    auto gil = GILGuard{};
    auto call = Call{};
    if (!call_begin(data, call)) {
        return store_python_error("propagator init failed");
    }
    clingo_propagate_control_t *ctl = nullptr;
    if (!clingo_propagate_init_control(init, &ctl)) {
        return false;
    }
    auto ass_obj = steal(alloc_object(call.state->assignment_type));
    auto init_obj = steal(alloc_object(call.state->init_type));
    if (ass_obj.get() == nullptr || init_obj.get() == nullptr) {
        return store_python_error("propagator init failed");
    }
    auto *library = call.handle->library;
    Py_XINCREF(library);
    as<PyPropagateInitObject>(init_obj.get())->library = library;
    auto ass_bind = AssignmentBinding{ass_obj.get(), ass};
    auto ctl_bind = ControlBinding{init_obj.get(), ctl};
    auto init_bind = InitBinding{init_obj.get(), init};
    auto res = steal(PyObject_CallMethodObjArgs(call.propagator.get(), call.state->str_init, ass_obj.get(),
                                                init_obj.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("propagator init failed");
    }
    return true;
}

auto control_cb(void *data, clingo_assignment_t const *ass, clingo_propagate_control_t *ctl,
                PyObject *PropagateModuleState::*name, char const *what) -> bool {
    auto gil = GILGuard{};
    auto call = Call{};
    PyObject *ass_obj = nullptr;
    PyObject *ctl_obj = nullptr;
    if (!call_begin(data, call) || !thread_objects(call, ass, &ass_obj, &ctl_obj)) {
        return store_python_error(what);
    }
    auto ass_bind = AssignmentBinding{ass_obj, ass};
    auto ctl_bind = ControlBinding{ctl_obj, ctl};
    auto res = steal(PyObject_CallMethodObjArgs(call.propagator.get(), call.state->*name, ass_obj, ctl_obj, nullptr));
    if (res.get() == nullptr) {
        return store_python_error(what);
    }
    return true;
}

auto attach_cb(clingo_assignment_t const *ass, clingo_propagate_control_t *ctl, void *data) -> bool {
    if (!handle_cast(data)->has_attach) {
        return true;
    }
    return control_cb(data, ass, ctl, &PropagateModuleState::str_attach, "propagator attach failed");
}

auto check_cb(clingo_assignment_t const *ass, clingo_propagate_control_t *ctl, void *data) -> bool {
    if (!handle_cast(data)->has_check) {
        return true;
    }
    return control_cb(data, ass, ctl, &PropagateModuleState::str_check, "propagator check failed");
}

auto propagate_cb(clingo_assignment_t const *ass, clingo_propagate_control_t *ctl, clingo_literal_t const *changes,
                  size_t size, void *data) -> bool {
    if (!handle_cast(data)->has_propagate) {
        return true;
    }
    auto gil = GILGuard{};
    auto call = Call{};
    PyObject *ass_obj = nullptr;
    PyObject *ctl_obj = nullptr;
    if (!call_begin(data, call) || !thread_objects(call, ass, &ass_obj, &ctl_obj)) {
        return store_python_error("propagator propagate failed");
    }
    auto ass_bind = AssignmentBinding{ass_obj, ass};
    auto ctl_bind = ControlBinding{ctl_obj, ctl};
    auto lits = steal(int_list(changes, size));
    if (lits.get() == nullptr) {
        return store_python_error("propagator propagate failed");
    }
    auto res = steal(PyObject_CallMethodObjArgs(call.propagator.get(), call.state->str_propagate, ass_obj, ctl_obj,
                                                lits.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("propagator propagate failed");
    }
    return true;
}

void undo_cb(clingo_assignment_t const *ass, clingo_literal_t const *changes, size_t size, void *data) {
    if (!handle_cast(data)->has_undo) {
        return;
    }
    auto gil = GILGuard{};
    if (handle_cast(data)->propagator == nullptr) {
        // NOTE: there is nothing to undo for a released propagator
        return;
    }
    auto call = Call{};
    PyObject *ass_obj = nullptr;
    if (call_begin(data, call) && thread_objects(call, ass, &ass_obj, nullptr)) {
        auto ass_bind = AssignmentBinding{ass_obj, ass};
        auto lits = steal(int_list(changes, size));
        if (lits.get() != nullptr) {
            auto res = steal(
                PyObject_CallMethodObjArgs(call.propagator.get(), call.state->str_undo, ass_obj, lits.get(), nullptr));
            if (res.get() != nullptr) {
                return;
            }
        }
    }
    PyErr_Print();
    fprintf(stderr, "panic: propagator undo failed\n");
    std::abort();
}

auto decide_cb(clingo_assignment_t const *ass, clingo_literal_t fallback, void *data, clingo_literal_t *decision)
    -> bool {
    auto gil = GILGuard{};
    auto call = Call{};
    PyObject *ass_obj = nullptr;
    if (!call_begin(data, call) || !thread_objects(call, ass, &ass_obj, nullptr)) {
        return store_python_error("propagator decide failed");
    }
    auto ass_bind = AssignmentBinding{ass_obj, ass};
    auto lit = steal(PyLong_FromLong(fallback));
    if (lit.get() == nullptr) {
        return store_python_error("propagator decide failed");
    }
    auto res =
        steal(PyObject_CallMethodObjArgs(call.propagator.get(), call.state->str_decide, ass_obj, lit.get(), nullptr));
    if (res.get() == nullptr || !parse_num(res.get(), decision, "decision")) {
        return store_python_error("propagator decide failed");
    }
    return true;
}

constexpr auto c_prop = clingo_propagator_t{
    init_cb, attach_cb, propagate_cb, undo_cb, check_cb, nullptr, nullptr,
};

constexpr auto c_heu = clingo_propagator_t{
    init_cb, attach_cb, propagate_cb, undo_cb, check_cb, decide_cb, nullptr,
};

//! Check whether the type of the propagator overrides the given method.
auto is_overridden(PyObject *base_type, PyObject *propagator, PyObject *name, bool *res) -> bool {
    auto impl = steal(PyObject_GetAttr(reinterpret_cast<PyObject *>(Py_TYPE(propagator)), name));
    if (impl.get() == nullptr) {
        return false;
    }
    auto def = steal(PyObject_GetAttr(base_type, name));
    if (def.get() == nullptr) {
        return false;
    }
    *res = impl.get() != def.get();
    return true;
}

// module

constexpr auto weight_constraint_type_members = std::array<IntEnumMember, 3>{
    IntEnumMember{"Equivalence", static_cast<int>(clingo_weight_constraint_type_equivalence)},
    IntEnumMember{"LeftImplication", static_cast<int>(clingo_weight_constraint_type_implication_left)},
    IntEnumMember{"RightImplication", static_cast<int>(clingo_weight_constraint_type_implication_right)},
};

constexpr auto check_mode_members = std::array<IntEnumMember, 4>{
    IntEnumMember{"Off", static_cast<int>(clingo_propagator_check_mode_none)},
    IntEnumMember{"Fixpoint", static_cast<int>(clingo_propagator_check_mode_fixpoint)},
    IntEnumMember{"Total", static_cast<int>(clingo_propagator_check_mode_total)},
    IntEnumMember{"Both", static_cast<int>(clingo_propagator_check_mode_both)},
};

constexpr auto undo_mode_members = std::array<IntEnumMember, 2>{
    IntEnumMember{"Default", static_cast<int>(clingo_propagator_undo_mode_default)},
    IntEnumMember{"Always", static_cast<int>(clingo_propagator_undo_mode_always)},
};

auto propagate_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<PropagateModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_VISIT(state->*member);
    }
    return 0;
}

auto propagate_module_clear(PyObject *module) -> int {
    auto *state = static_cast<PropagateModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_CLEAR(state->*member);
    }
    return 0;
}

void propagate_module_free(void *module) {
    propagate_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef propagate_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.propagate",
    PyDoc_STR("Functions and classes to implement custom propagators."),
    static_cast<Py_ssize_t>(sizeof(PropagateModuleState)),
    nullptr,
    nullptr,
    propagate_module_traverse,
    propagate_module_clear,
    propagate_module_free,
};

auto register_sequences(PropagateModuleState *state) -> bool {
    auto abc = steal(PyImport_ImportModule("collections.abc"));
    if (abc.get() == nullptr) {
        return false;
    }
    auto seq = steal(PyObject_GetAttrString(abc.get(), "Sequence"));
    if (seq.get() == nullptr) {
        return false;
    }
    for (auto *type : {state->assignment_type, state->trail_type, state->trail_view_type}) {
        auto res = steal(PyObject_CallMethod(seq.get(), "register", "O", type));
        if (res.get() == nullptr) {
            return false;
        }
    }
    return true;
}

auto intern_names(PropagateModuleState *state) -> bool {
    auto names = std::array{
        std::pair{&state->str_init, "init"},           std::pair{&state->str_attach, "attach"},
        std::pair{&state->str_propagate, "propagate"}, std::pair{&state->str_undo, "undo"},
        std::pair{&state->str_check, "check"},         std::pair{&state->str_decide, "decide"},
    };
    // NOLINTNEXTLINE(readability-use-anyofallof)
    for (auto [store, name] : names) {
        *store = PyUnicode_InternFromString(name);
        if (*store == nullptr) {
            return false;
        }
    }
    return true;
}

} // namespace

auto py_propagator_type() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.propagate"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    if (state->propagator_type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.propagate module is not initialized");
        return nullptr;
    }
    Py_INCREF(state->propagator_type);
    return state->propagator_type;
}

auto py_register_propagator(PyObject *type, clingo_control_t *ctl, PyObject *keeper, PyObject *library,
                            PyObject *propagator) -> bool {
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return false;
    }
    auto is_prop = PyObject_IsInstance(propagator, state->propagator_type);
    if (is_prop < 0) {
        return false;
    }
    if (is_prop == 0) {
        PyErr_SetString(PyExc_TypeError, "propagator must be an instance of clingo.propagate.Propagator");
        return false;
    }
    auto obj = steal(alloc_object(state->handle_type));
    if (obj.get() == nullptr) {
        return false;
    }
    auto *h = as<PyPropagatorHandleObject>(obj.get());
    Py_INCREF(propagator);
    h->propagator = propagator;
    Py_XINCREF(library);
    h->library = library;
    auto overrides = std::array{
        std::pair{&h->has_init, state->str_init},           std::pair{&h->has_attach, state->str_attach},
        std::pair{&h->has_propagate, state->str_propagate}, std::pair{&h->has_undo, state->str_undo},
        std::pair{&h->has_check, state->str_check},         std::pair{&h->has_decide, state->str_decide},
    };
    for (auto [flag, name] : overrides) {
        if (!is_overridden(state->propagator_type, propagator, name, flag)) {
            return false;
        }
    }
    // NOTE: keep the handle alive before clingo gets hold of it
    if (PyList_Append(keeper, obj.get()) < 0) {
        return false;
    }
    if (!clingo_control_register_propagator(ctl, h->has_decide ? &c_heu : &c_prop, static_cast<void *>(obj.get()))) {
        py_handle_error();
        return false;
    }
    return true;
}

void register_propagate(pybind11::module &m) {
    auto propagate_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &propagate_def, &propagate_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(propagate_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    state->base_type = py_base_type();
    if (state->base_type == nullptr || !intern_names(state)) {
        throw py::error_already_set();
    }

    auto *mod = propagate_mod.get();
    if (!add_int_enum(mod, "WeightConstraintType", weight_constraint_type_members, &state->weight_constraint_type_enum,
                      {}) ||
        !add_int_enum(mod, "CheckMode", check_mode_members, &state->check_mode_enum, {}) ||
        !add_int_enum(mod, "UndoMode", undo_mode_members, &state->undo_mode_enum, {}) ||
        !add_type(mod, "_TrailView", &py_view_spec, &state->trail_view_type) ||
        !add_type(mod, "Trail", &py_trail_spec, &state->trail_type) ||
        !add_type(mod, "Assignment", &py_assignment_spec, &state->assignment_type) ||
        !add_type(mod, "PropagateControl", &py_control_spec, &state->control_type) ||
        !add_type(mod, "PropagateInit", &py_init_spec, &state->init_type, state->control_type) ||
        !add_type(mod, "Propagator", &py_propagator_spec, &state->propagator_type) ||
        !add_type(mod, "_PropagatorHandle", &py_handle_spec, &state->handle_type) || !register_sequences(state)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
