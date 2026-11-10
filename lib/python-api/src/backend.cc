#define PY_SSIZE_T_CLEAN

#include <clingo/backend.h>
#include <clingo/observe.h>

#include "backend.hh"
#include "base.hh"
#include "symbol.hh"
#include "util.hh"

#include <array>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>

namespace PyClingo {

namespace {

using PyObserverObject = struct {
    PyObject_HEAD
};

using PyBackendObject = struct {
    PyObject_HEAD clingo_backend_t *backend;
    PyObject *owner;
};

using PyBackendManagerObject = struct {
    PyObject_HEAD clingo_control_t *ctl;
    clingo_backend_t *backend;
    PyObject *owner;
    PyObject *backend_obj;
};

struct BackendModuleState {
    PyObject *symbol_type{nullptr};
    PyObject *observer_type{nullptr};
    PyObject *backend_type{nullptr};
    PyObject *backend_manager_type{nullptr};
    PyObject *heuristic_type_enum{nullptr};
    PyObject *external_type_enum{nullptr};
    PyObject *theory_sequence_type_enum{nullptr};
};

constexpr auto heuristic_type_members = std::array<IntEnumMember, 6>{
    IntEnumMember{"Level", static_cast<int>(clingo_heuristic_type_level)},
    IntEnumMember{"Factor", static_cast<int>(clingo_heuristic_type_factor)},
    IntEnumMember{"True_", static_cast<int>(clingo_heuristic_type_true)},
    IntEnumMember{"False_", static_cast<int>(clingo_heuristic_type_false)},
    IntEnumMember{"Init", static_cast<int>(clingo_heuristic_type_init)},
    IntEnumMember{"Sign", static_cast<int>(clingo_heuristic_type_sign)},
};

constexpr auto external_type_members = std::array<IntEnumMember, 4>{
    IntEnumMember{"True_", static_cast<int>(clingo_external_type_true)},
    IntEnumMember{"False_", static_cast<int>(clingo_external_type_false)},
    IntEnumMember{"Free", static_cast<int>(clingo_external_type_free)},
    IntEnumMember{"Release", static_cast<int>(clingo_external_type_release)},
};

constexpr auto theory_sequence_type_members = std::array<IntEnumMember, 3>{
    IntEnumMember{"Tuple", static_cast<int>(clingo_theory_sequence_type_tuple)},
    IntEnumMember{"List", static_cast<int>(clingo_theory_sequence_type_list)},
    IntEnumMember{"Set", static_cast<int>(clingo_theory_sequence_type_set)},
};

auto get_state(PyObject *module) -> BackendModuleState * {
    auto *state = static_cast<BackendModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "backend module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_self(PyObject *self) -> BackendModuleState * {
    auto *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve backend module");
        return nullptr;
    }
    return get_state(mod);
}

template <std::integral T> auto parse_num(PyObject *obj, T *out, char const *what) -> bool {
    if (obj == nullptr) {
        return true;
    }
    if constexpr (std::is_unsigned_v<T>) {
        auto i = steal(PyNumber_Index(obj));
        if (i.get() == nullptr) {
            return false;
        }
        auto v = PyLong_AsUnsignedLong(i.get());
        if (PyErr_Occurred() != nullptr) {
            PyErr_Format(PyExc_TypeError, "%s must be a non-negative integer", what);
            return false;
        }
        if (std::cmp_greater(v, std::numeric_limits<T>::max())) {
            PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            return false;
        }
        *out = static_cast<T>(v);
    } else {
        auto v = PyLong_AsLong(obj);
        if (PyErr_Occurred() != nullptr) {
            PyErr_Format(PyExc_TypeError, "%s must be an integer", what);
            return false;
        }
        if (std::cmp_less(v, std::numeric_limits<T>::min()) || std::cmp_greater(v, std::numeric_limits<T>::max())) {
            PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            return false;
        }
        *out = static_cast<T>(v);
    }
    return true;
}

auto parse_symbol(BackendModuleState *state, PyObject *obj, clingo_symbol_t *sym, char const *what) -> bool {
    if (state == nullptr) {
        return false;
    }
    auto res = py_symbol_check(state->symbol_type, obj, sym);
    if (res < 0) {
        return false;
    }
    if (res == 0) {
        PyErr_Format(PyExc_TypeError, "%s must be a Symbol", what);
        return false;
    }
    return true;
}

template <class T> struct PyBuffer {
    PyBuffer() = default;
    PyBuffer(PyBuffer const &) = delete;
    auto operator=(PyBuffer const &) -> PyBuffer & = delete;
    PyBuffer(PyBuffer &&) = delete;
    auto operator=(PyBuffer &&) -> PyBuffer & = delete;

    ~PyBuffer() {
        if (ptr_ != nullptr) {
            PyMem_Free(ptr_);
        }
    }

    auto alloc(size_t n) -> bool {
        T *new_ptr = nullptr;
        if (n > 0) {
            new_ptr = static_cast<T *>(PyMem_Calloc(n, sizeof(T)));
            if (new_ptr == nullptr) {
                PyErr_NoMemory();
                return false;
            }
        }
        if (ptr_ != nullptr) {
            PyMem_Free(ptr_);
        }
        ptr_ = new_ptr;
        n_ = n;
        return true;
    }

    auto data() -> T * { return ptr_; }
    [[nodiscard]] auto data() const -> T const * { return ptr_; }
    [[nodiscard]] auto size() const -> size_t { return n_; }

    auto operator[](size_t i) -> T & { return ptr_[i]; }
    auto operator[](size_t i) const -> T const & { return ptr_[i]; }

  private:
    T *ptr_{nullptr};
    size_t n_{0};
};

template <class T, class F>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
auto parse_sequence(PyObject *obj, PyBuffer<T> *out, char const *what, F &&fun) -> bool {
    auto seq_fast = steal(PySequence_Fast(obj, what));
    if (seq_fast.get() == nullptr) {
        return false;
    }

    auto seq = steal(PySequence_Tuple(seq_fast.get()));
    if (seq.get() == nullptr) {
        return false;
    }

    auto n = PyTuple_GET_SIZE(seq.get());
    if (n < 0) {
        return false;
    }

    auto count = static_cast<size_t>(n);
    if (!out->alloc(count)) {
        return false;
    }

    auto *it = out->data();
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *item = PyTuple_GET_ITEM(seq.get(), i); // borrowed, tuple-owned
        if (!std::invoke(fun, item, it++)) {
            return false;
        }
    }
    return true;
}

auto parse_atoms(PyObject *obj, PyBuffer<clingo_atom_t> *out, char const *what) -> bool {
    return parse_sequence<clingo_atom_t>(
        obj, out, what, [](PyObject *item, clingo_atom_t *val) { return parse_num(item, val, "atom"); });
}

auto parse_ids(PyObject *obj, PyBuffer<clingo_id_t> *out, char const *what) -> bool {
    return parse_sequence<clingo_id_t>(obj, out, what,
                                       [](PyObject *item, clingo_id_t *val) { return parse_num(item, val, "id"); });
}

auto parse_literals(PyObject *obj, PyBuffer<clingo_literal_t> *out, char const *what) -> bool {
    return parse_sequence<clingo_literal_t>(
        obj, out, what, [](PyObject *item, clingo_literal_t *val) { return parse_num(item, val, "literal"); });
}

auto parse_weighted_literals(PyObject *obj, PyBuffer<clingo_weighted_literal_t> *out, char const *what) -> bool {
    return parse_sequence<clingo_weighted_literal_t>(
        obj, out, what, [](PyObject *item, clingo_weighted_literal_t *val) {
            auto pair = steal(PySequence_Fast(item, "weighted literal must be a pair (literal, weight)"));
            if (pair.get() == nullptr) {
                return false;
            }
            if (PySequence_Fast_GET_SIZE(pair.get()) != 2) {
                PyErr_SetString(PyExc_TypeError, "weighted literal must be a pair (literal, weight)");
                return false;
            }
            auto **items = PySequence_Fast_ITEMS(pair.get());
            auto lit_obj = borrow(items[0]);
            auto weight_obj = borrow(items[1]);
            if (!parse_num(lit_obj.get(), &val->literal, "literal")) {
                return false;
            }
            if (!parse_num(weight_obj.get(), &val->weight, "weight")) {
                return false;
            }
            return true;
        });
}

auto py_list_from_atoms(clingo_atom_t const *data, size_t n) -> PyObject * {
    auto out = steal(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *v = PyLong_FromSize_t(static_cast<size_t>(data[i]));
        if (v == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), v);
    }
    return out.release();
}

auto py_list_from_literals(clingo_literal_t const *data, size_t n) -> PyObject * {
    auto out = steal(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto *v = PyLong_FromLong(data[i]);
        if (v == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), v);
    }
    return out.release();
}

auto py_list_from_weighted_literals(clingo_weighted_literal_t const *data, size_t n) -> PyObject * {
    auto out = steal(PyList_New(static_cast<Py_ssize_t>(n)));
    if (out.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        auto lit = steal(PyLong_FromLong(data[i].literal));
        if (lit.get() == nullptr) {
            return nullptr;
        }
        auto w = steal(PyLong_FromLong(data[i].weight));
        if (w.get() == nullptr) {
            return nullptr;
        }
        auto pair = steal(PyTuple_Pack(2, lit.get(), w.get()));
        if (pair.get() == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(out.get(), static_cast<Py_ssize_t>(i), pair.release());
    }
    return out.release();
}

using ObserverData = std::pair<PyObject *, BackendModuleState *>;

auto call_observer(ObserverData *data, char const *name, PyObject *args) -> bool {
    auto meth = steal(PyObject_GetAttrString(data->first, name));
    if (meth.get() == nullptr) {
        return store_python_error("observer callback lookup failed");
    }
    auto ret = steal(PyObject_CallObject(meth.get(), args));
    if (ret.get() == nullptr) {
        return store_python_error("observer callback failed");
    }
    return true;
}

auto py_observer_invoke_init_program(ObserverData *data, bool incremental) -> bool {
    auto args = steal(PyTuple_Pack(1, incremental ? Py_True : Py_False));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "init_program", args.get());
}

auto py_observer_invoke_begin_step(ObserverData *data) -> bool {
    auto args = steal(PyTuple_New(0));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "begin_step", args.get());
}

auto py_observer_invoke_end_step(ObserverData *data, clingo_base_t const *base) -> bool {
    auto *base_type = py_base_type_ref();
    if (base_type == nullptr) {
        return store_python_error("failed to resolve base type");
    }
    auto base_obj = steal(py_base_new(data->first, base_type, base));
    if (base_obj.get() == nullptr) {
        return store_python_error("failed to create base object");
    }
    auto args = steal(PyTuple_Pack(1, base_obj.get()));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "end_step", args.get());
}

auto py_observer_invoke_rule(ObserverData *data, clingo_atom_t const *head, size_t head_size,
                             clingo_literal_t const *body, size_t body_size, bool choice) -> bool {
    auto head_obj = steal(py_list_from_atoms(head, head_size));
    if (head_obj.get() == nullptr) {
        return store_python_error("failed to build head list");
    }
    auto body_obj = steal(py_list_from_literals(body, body_size));
    if (body_obj.get() == nullptr) {
        return store_python_error("failed to build body list");
    }
    auto args = steal(PyTuple_Pack(3, head_obj.get(), body_obj.get(), choice ? Py_True : Py_False));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "rule", args.get());
}

auto py_observer_invoke_weight_rule(ObserverData *data, clingo_atom_t const *head, size_t head_size,
                                    clingo_weight_t lower, clingo_weighted_literal_t const *body, size_t body_size,
                                    bool choice) -> bool {
    auto head_obj = steal(py_list_from_atoms(head, head_size));
    if (head_obj.get() == nullptr) {
        return store_python_error("failed to build head list");
    }
    auto lower_obj = steal(PyLong_FromLong(lower));
    if (lower_obj.get() == nullptr) {
        return store_python_error("failed to build lower bound");
    }
    auto body_obj = steal(py_list_from_weighted_literals(body, body_size));
    if (body_obj.get() == nullptr) {
        return store_python_error("failed to build body list");
    }
    auto args = steal(PyTuple_Pack(4, head_obj.get(), lower_obj.get(), body_obj.get(), choice ? Py_True : Py_False));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "weight_rule", args.get());
}

auto py_observer_invoke_minimize(ObserverData *data, clingo_weighted_literal_t const *body, size_t body_size,
                                 clingo_weight_t priority) -> bool {
    auto body_obj = steal(py_list_from_weighted_literals(body, body_size));
    if (body_obj.get() == nullptr) {
        return store_python_error("failed to build body list");
    }
    auto p_obj = steal(PyLong_FromLong(priority));
    if (p_obj.get() == nullptr) {
        return store_python_error("failed to build priority");
    }
    auto args = steal(PyTuple_Pack(2, body_obj.get(), p_obj.get()));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "minimize", args.get());
}

auto py_observer_invoke_project(ObserverData *data, clingo_atom_t const *atoms, size_t size) -> bool {
    auto atoms_obj = steal(py_list_from_atoms(atoms, size));
    if (atoms_obj.get() == nullptr) {
        return store_python_error("failed to build atom list");
    }
    auto args = steal(PyTuple_Pack(1, atoms_obj.get()));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "project", args.get());
}

auto py_observer_invoke_external(ObserverData *data, clingo_atom_t atom, clingo_external_type_t type) -> bool {
    auto atom_obj = steal(PyLong_FromSize_t(static_cast<size_t>(atom)));
    if (atom_obj.get() == nullptr) {
        return store_python_error("failed to build atom argument");
    }
    auto type_obj = steal(PyObject_CallFunction(data->second->external_type_enum, "i", static_cast<int>(type)));
    if (type_obj.get() == nullptr) {
        return store_python_error("failed to build external type argument");
    }
    auto args = steal(PyTuple_Pack(2, atom_obj.get(), type_obj.get()));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "external", args.get());
}

auto py_observer_invoke_assume(ObserverData *data, clingo_literal_t const *literals, size_t size) -> bool {
    auto lits_obj = steal(py_list_from_literals(literals, size));
    if (lits_obj.get() == nullptr) {
        return store_python_error("failed to build literal list");
    }
    auto args = steal(PyTuple_Pack(1, lits_obj.get()));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "assume", args.get());
}

auto py_observer_invoke_heuristic(ObserverData *data, clingo_atom_t atom, clingo_heuristic_type_t type, int weight,
                                  unsigned priority, clingo_literal_t const *condition, size_t size) -> bool {
    auto atom_obj = steal(PyLong_FromSize_t(static_cast<size_t>(atom)));
    if (atom_obj.get() == nullptr) {
        return store_python_error("failed to build atom argument");
    }
    auto type_obj = steal(PyObject_CallFunction(data->second->heuristic_type_enum, "i", static_cast<int>(type)));
    if (type_obj.get() == nullptr) {
        return store_python_error("failed to build heuristic type argument");
    }
    auto weight_obj = steal(PyLong_FromLong(weight));
    if (weight_obj.get() == nullptr) {
        return store_python_error("failed to build weight argument");
    }
    auto prio_obj = steal(PyLong_FromUnsignedLong(priority));
    if (prio_obj.get() == nullptr) {
        return store_python_error("failed to build priority argument");
    }
    auto cond_obj = steal(py_list_from_literals(condition, size));
    if (cond_obj.get() == nullptr) {
        return store_python_error("failed to build condition list");
    }
    auto args = steal(PyTuple_Pack(
        // NOLINTNEXTLINE(readability-magic-numbers)
        5, atom_obj.get(), type_obj.get(), weight_obj.get(), prio_obj.get(), cond_obj.get()));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "heuristic", args.get());
}

auto py_observer_invoke_edge(ObserverData *data, int node_u, int node_v, clingo_literal_t const *condition, size_t size)
    -> bool {
    auto u_obj = steal(PyLong_FromLong(node_u));
    if (u_obj.get() == nullptr) {
        return store_python_error("failed to build edge argument");
    }
    auto v_obj = steal(PyLong_FromLong(node_v));
    if (v_obj.get() == nullptr) {
        return store_python_error("failed to build edge argument");
    }
    auto cond_obj = steal(py_list_from_literals(condition, size));
    if (cond_obj.get() == nullptr) {
        return store_python_error("failed to build condition list");
    }
    auto args = steal(PyTuple_Pack(3, u_obj.get(), v_obj.get(), cond_obj.get()));
    if (args.get() == nullptr) {
        return store_python_error("failed to prepare callback arguments");
    }
    return call_observer(data, "edge", args.get());
}

auto cast_observer_data(void *data) -> std::pair<PyObject *, BackendModuleState *> * {
    return static_cast<std::pair<PyObject *, BackendModuleState *> *>(data);
}

auto observer_trampoline() -> clingo_observer_t const * {
    static constexpr auto obs = clingo_observer_t{
        [](bool incremental, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_init_program(cast_observer_data(data), incremental);
        },
        [](void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_begin_step(cast_observer_data(data));
        },
        [](clingo_base_t const *base, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_end_step(cast_observer_data(data), base);
        },
        [](bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size,
           void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_rule(cast_observer_data(data), head, head_size, body, body_size, choice);
        },
        [](bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower,
           clingo_weighted_literal_t const *body, size_t body_size, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_weight_rule(cast_observer_data(data), head, head_size, lower, body, body_size,
                                                  choice);
        },
        [](clingo_weight_t priority, clingo_weighted_literal_t const *body, size_t body_size, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_minimize(cast_observer_data(data), body, body_size, priority);
        },
        [](clingo_atom_t const *atoms, size_t size, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_project(cast_observer_data(data), atoms, size);
        },
        [](clingo_atom_t atom, clingo_external_type_t type, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_external(cast_observer_data(data), atom, type);
        },
        [](clingo_literal_t const *literals, size_t size, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_assume(cast_observer_data(data), literals, size);
        },
        [](clingo_atom_t atom, clingo_heuristic_type_t type, int weight, unsigned priority,
           clingo_literal_t const *condition, size_t size, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_heuristic(cast_observer_data(data), atom, type, weight, priority, condition,
                                                size);
        },
        [](int node_u, int node_v, clingo_literal_t const *condition, size_t size, void *data) -> bool {
            auto gil = GILGuard{};
            return py_observer_invoke_edge(cast_observer_data(data), node_u, node_v, condition, size);
        },
    };
    return &obs;
}

auto py_observer_init_program([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                              [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_begin_step([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_end_step([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                          [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_rule([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                      [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_weight_rule([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                             [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_minimize([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                          [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_project([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                         [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_external([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                          [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_assume([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                        [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_heuristic([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                           [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_edge([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args,
                      [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_observer_methods = std::array{
    PyMethodDef{
        "init_program",
        py_c_function_cast(py_observer_init_program),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "begin_step",
        py_observer_begin_step,
        METH_NOARGS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "end_step",
        py_c_function_cast(py_observer_end_step),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "rule",
        py_c_function_cast(py_observer_rule),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "weight_rule",
        py_c_function_cast(py_observer_weight_rule),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "minimize",
        py_c_function_cast(py_observer_minimize),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "project",
        py_c_function_cast(py_observer_project),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "external",
        py_c_function_cast(py_observer_external),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "assume",
        py_c_function_cast(py_observer_assume),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "heuristic",
        py_c_function_cast(py_observer_heuristic),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "edge",
        py_c_function_cast(py_observer_edge),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_observer_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("ABC to inspect aspif directives."))},
    PyType_Slot{Py_tp_methods, py_observer_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_observer_spec = PyType_Spec{
    "clingo.backend.Observer", sizeof(PyObserverObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    py_observer_slots.data(),
};

auto py_backend_cast(PyObject *obj) -> PyBackendObject * {
    return reinterpret_cast<PyBackendObject *>(obj);
}

auto py_backend_new(PyObject *type, clingo_backend_t *backend, PyObject *owner) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    auto *obj = to->tp_alloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *b = py_backend_cast(obj);
    b->backend = backend;
    b->owner = owner;
    Py_XINCREF(owner);
    return obj;
}

auto py_backend_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *b = py_backend_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(b->owner);
    return 0;
}

auto py_backend_clear(PyObject *self) -> int {
    auto *b = py_backend_cast(self);
    Py_CLEAR(b->owner);
    return 0;
}

auto ensure_backend_open(PyObject *self) -> clingo_backend_t * {
    auto *b = py_backend_cast(self);
    if (b->backend == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "backend is closed");
        return nullptr;
    }
    return b->backend;
}

auto py_backend_atom(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"symbol", nullptr};
    PyObject *sym_obj = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O:atom", cast_kw(kwlist), &sym_obj) == 0) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    clingo_symbol_t const *sym_ptr = nullptr;
    if (sym_obj != Py_None) {
        if (!parse_symbol(state, sym_obj, &sym, "symbol")) {
            return nullptr;
        }
        sym_ptr = &sym;
    }

    clingo_atom_t atom = 0;
    if (!clingo_backend_add_atom(backend, sym_ptr, &atom)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(atom));
}

auto py_backend_rule(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 4>{"head", "body", "choice", nullptr};
    PyObject *head_obj = nullptr;
    PyObject *body_obj = Py_None;
    int choice = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|Op:rule", cast_kw(kwlist), &head_obj, &body_obj, &choice) == 0) {
        return nullptr;
    }

    PyBuffer<clingo_atom_t> head;
    PyBuffer<clingo_literal_t> body;
    if (!parse_atoms(head_obj, &head, "head must be a sequence of atoms")) {
        return nullptr;
    }
    if (body_obj != Py_None && !parse_literals(body_obj, &body, "body must be a sequence of literals")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_rule(backend, choice != 0, head.data(), head.size(), body.data(), body.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_weight_rule(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 5>{"head", "lower_bound", "body", "choice", nullptr};
    PyObject *head_obj = nullptr;
    PyObject *lower_obj = nullptr;
    PyObject *body_obj = nullptr;
    int choice = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OOO|p:weight_rule", cast_kw(kwlist), &head_obj, &lower_obj,
                                    &body_obj, &choice) == 0) {
        return nullptr;
    }

    PyBuffer<clingo_atom_t> head;
    PyBuffer<clingo_weighted_literal_t> body;
    if (!parse_atoms(head_obj, &head, "head must be a sequence of atoms")) {
        return nullptr;
    }
    if (!parse_weighted_literals(body_obj, &body, "body must be a sequence of (literal, weight) pairs")) {
        return nullptr;
    }
    int lower_i = 0;
    if (!parse_num(lower_obj, &lower_i, "lower_bound")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_weight_rule(backend, choice != 0, head.data(), head.size(),
                                    static_cast<clingo_weight_t>(lower_i), body.data(), body.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_minimize(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"literals", "priority", nullptr};
    PyObject *lits_obj = nullptr;
    int priority = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|i:minimize", cast_kw(kwlist), &lits_obj, &priority) == 0) {
        return nullptr;
    }

    PyBuffer<clingo_weighted_literal_t> lits;
    if (!parse_weighted_literals(lits_obj, &lits, "literals must be a sequence of (literal, weight) pairs")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_minimize(backend, static_cast<clingo_weight_t>(priority), lits.data(), lits.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_project(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"atoms", nullptr};
    PyObject *atoms_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:project", cast_kw(kwlist), &atoms_obj) == 0) {
        return nullptr;
    }

    PyBuffer<clingo_atom_t> atoms;
    if (!parse_atoms(atoms_obj, &atoms, "atoms must be a sequence of atoms")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_project(backend, atoms.data(), atoms.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_external(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"atom", "type", nullptr};
    PyObject *atom_obj = nullptr;
    PyObject *type_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO:external", cast_kw(kwlist), &atom_obj, &type_obj) == 0) {
        return nullptr;
    }

    clingo_atom_t atom = 0;
    if (!parse_num(atom_obj, &atom, "atom")) {
        return nullptr;
    }
    clingo_external_type_t type = 0;
    if (!parse_num(type_obj, &type, "type")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_external(backend, atom, type)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_assume(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"literals", nullptr};
    PyObject *lits_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:assume", cast_kw(kwlist), &lits_obj) == 0) {
        return nullptr;
    }

    PyBuffer<clingo_literal_t> lits;
    if (!parse_literals(lits_obj, &lits, "literals must be a sequence of literals")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_assume(backend, lits.data(), lits.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_heuristic(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist =
        std::array<char const *, 6>{"atom", "type", "weight", "priority", "condition", nullptr};
    PyObject *atom_obj = nullptr;
    PyObject *type_obj = nullptr;
    PyObject *weight_obj = nullptr;
    PyObject *priority_obj = nullptr;
    PyObject *cond_obj = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OOO|OO:heuristic", cast_kw(kwlist), &atom_obj, &type_obj,
                                    &weight_obj, &priority_obj, &cond_obj) == 0) {
        return nullptr;
    }

    clingo_atom_t atom = 0;
    clingo_heuristic_type_t type = 0;
    int weight = 0;
    unsigned priority = 0;
    if (!parse_num(atom_obj, &atom, "atom") || !parse_num(type_obj, &type, "type") ||
        !parse_num(weight_obj, &weight, "weight") || !parse_num(priority_obj, &priority, "priority")) {
        return nullptr;
    }

    PyBuffer<clingo_literal_t> cond;
    if (cond_obj != Py_None && !parse_literals(cond_obj, &cond, "condition must be a sequence of literals")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_heuristic(backend, atom, type, weight, priority, cond.data(), cond.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_edge(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 4>{"node_u", "node_v", "condition", nullptr};
    PyObject *u_obj = nullptr;
    PyObject *v_obj = nullptr;
    PyObject *cond_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OOO:edge", cast_kw(kwlist), &u_obj, &v_obj, &cond_obj) == 0) {
        return nullptr;
    }

    int u = 0;
    int v = 0;
    if (!parse_num(u_obj, &u, "node_u") || !parse_num(v_obj, &v, "node_v")) {
        return nullptr;
    }

    PyBuffer<clingo_literal_t> cond;
    if (!parse_literals(cond_obj, &cond, "condition must be a sequence of literals")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }
    if (!clingo_backend_acyc_edge(backend, u, v, cond.data(), cond.size())) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_backend_theory_number(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"number", nullptr};
    int number = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "i:theory_number", cast_kw(kwlist), &number) == 0) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    clingo_id_t id = 0;
    if (!clingo_backend_theory_term_number(backend, number, &id)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(id));
}

auto py_backend_theory_string(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"string", nullptr};
    char const *text = nullptr;
    Py_ssize_t n = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "s#:theory_string", cast_kw(kwlist), &text, &n) == 0) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    clingo_id_t id = 0;
    if (!clingo_backend_theory_term_string(backend, text, static_cast<size_t>(n), &id)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(id));
}

auto py_backend_theory_symbol(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"symbol", nullptr};
    PyObject *sym_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:theory_symbol", cast_kw(kwlist), &sym_obj) == 0) {
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!parse_symbol(state, sym_obj, &sym, "symbol")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    clingo_id_t id = 0;
    if (!clingo_backend_theory_term_symbol(backend, sym, &id)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(id));
}

auto py_backend_theory_sequence(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"type", "elements", nullptr};
    PyObject *type_obj = nullptr;
    PyObject *elems_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO:theory_sequence", cast_kw(kwlist), &type_obj, &elems_obj) == 0) {
        return nullptr;
    }

    int type = 0;
    if (!parse_num(type_obj, &type, "type")) {
        return nullptr;
    }
    PyBuffer<clingo_id_t> elems;
    if (!parse_ids(elems_obj, &elems, "elements must be a sequence of ids")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    clingo_id_t id = 0;
    if (!clingo_backend_theory_term_sequence(backend, static_cast<clingo_theory_sequence_type_t>(type), elems.data(),
                                             elems.size(), &id)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(id));
}

auto py_backend_theory_function(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"name", "arguments", nullptr};
    char const *name = nullptr;
    Py_ssize_t name_n = 0;
    PyObject *args_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "s#O:theory_function", cast_kw(kwlist), &name, &name_n, &args_obj) ==
        0) {
        return nullptr;
    }

    PyBuffer<clingo_id_t> arg_ids;
    if (!parse_ids(args_obj, &arg_ids, "arguments must be a sequence of ids")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    clingo_id_t id = 0;
    if (!clingo_backend_theory_term_function(backend, name, static_cast<size_t>(name_n), arg_ids.data(), arg_ids.size(),
                                             &id)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(id));
}

auto py_backend_theory_element(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 3>{"tuple", "condition", nullptr};
    PyObject *tuple_obj = nullptr;
    PyObject *cond_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO:theory_element", cast_kw(kwlist), &tuple_obj, &cond_obj) == 0) {
        return nullptr;
    }

    PyBuffer<clingo_id_t> tuple;
    if (!parse_ids(tuple_obj, &tuple, "tuple must be a sequence of ids")) {
        return nullptr;
    }
    PyBuffer<clingo_literal_t> cond;
    if (!parse_literals(cond_obj, &cond, "condition must be a sequence of literals")) {
        return nullptr;
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    clingo_id_t id = 0;
    if (!clingo_backend_theory_element(backend, tuple.data(), tuple.size(), cond.data(), cond.size(), &id)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(id));
}

auto py_backend_theory_atom(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 5>{"atom", "name", "elements", "guard", nullptr};
    PyObject *atom_obj = nullptr;
    PyObject *name_obj = nullptr;
    PyObject *elems_obj = nullptr;
    PyObject *guard_obj = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OOO|O:theory_atom", cast_kw(kwlist), &atom_obj, &name_obj,
                                    &elems_obj, &guard_obj) == 0) {
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    clingo_symbol_t name = 0;
    if (!parse_symbol(state, name_obj, &name, "name")) {
        return nullptr;
    }

    PyBuffer<clingo_id_t> elems;
    if (!parse_ids(elems_obj, &elems, "elements must be a sequence of ids")) {
        return nullptr;
    }

    std::optional<clingo_atom_t> atom = std::nullopt;
    if (atom_obj != Py_None) {
        clingo_atom_t a = 0;
        if (!parse_num(atom_obj, &a, "atom")) {
            return nullptr;
        }
        atom = a;
    }

    std::optional<std::tuple<PyGuard, clingo_string_t, clingo_id_t>> guard = std::nullopt;
    if (guard_obj != Py_None) {
        auto guard_seq = steal(PySequence_Fast(guard_obj, "guard must be a pair (operator, term_id) or None"));
        if (guard_seq.get() == nullptr) {
            return nullptr;
        }
        if (PySequence_Fast_GET_SIZE(guard_seq.get()) != 2) {
            PyErr_SetString(PyExc_TypeError, "guard must be a pair (operator, term_id) or None");
            return nullptr;
        }
        auto **items = PySequence_Fast_ITEMS(guard_seq.get());
        auto op_obj = borrow(items[0]);
        auto term_obj = borrow(items[1]);

        if (PyUnicode_Check(op_obj.get()) == 0) {
            PyErr_SetString(PyExc_TypeError, "guard operator must be a string");
            return nullptr;
        }

        Py_ssize_t op_n = 0;
        char const *op = PyUnicode_AsUTF8AndSize(op_obj.get(), &op_n);
        if (op == nullptr) {
            return nullptr;
        }

        clingo_id_t term = 0;
        if (!parse_num(term_obj.get(), &term, "guard term id")) {
            return nullptr;
        }

        auto op_string = clingo_string_t{op, static_cast<size_t>(op_n)};
        guard.emplace(std::move(op_obj), op_string, term);
    }

    auto *backend = ensure_backend_open(self);
    if (backend == nullptr) {
        return nullptr;
    }

    clingo_atom_t out = 0;
    clingo_string_t *rhs_rel = guard ? &std::get<1>(*guard) : nullptr;
    clingo_id_t rhs_term = guard ? std::get<2>(*guard) : 0;
    clingo_atom_t atom_value = atom.value_or(0);
    if (!clingo_backend_theory_atom(backend, name, elems.data(), elems.size(), rhs_rel, rhs_term,
                                    atom ? &atom_value : nullptr, &out)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(static_cast<size_t>(out));
}

auto py_backend_methods = std::array{
    PyMethodDef{
        "atom",
        py_c_function_cast(py_backend_atom),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "rule",
        py_c_function_cast(py_backend_rule),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "weight_rule",
        py_c_function_cast(py_backend_weight_rule),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "minimize",
        py_c_function_cast(py_backend_minimize),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "project",
        py_c_function_cast(py_backend_project),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "external",
        py_c_function_cast(py_backend_external),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "assume",
        py_c_function_cast(py_backend_assume),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "heuristic",
        py_c_function_cast(py_backend_heuristic),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "edge",
        py_c_function_cast(py_backend_edge),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "theory_number",
        py_c_function_cast(py_backend_theory_number),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "theory_string",
        py_c_function_cast(py_backend_theory_string),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "theory_symbol",
        py_c_function_cast(py_backend_theory_symbol),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "theory_sequence",
        py_c_function_cast(py_backend_theory_sequence),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "theory_function",
        py_c_function_cast(py_backend_theory_function),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "theory_element",
        py_c_function_cast(py_backend_theory_element),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "theory_atom",
        py_c_function_cast(py_backend_theory_atom),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_backend_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Provides an interface to extend a logic program."))},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_backend_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_backend_clear)},
    PyType_Slot{Py_tp_methods, py_backend_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_backend_spec = PyType_Spec{
    "clingo.backend.Backend",
    sizeof(PyBackendObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_backend_slots.data(),
};

auto py_backend_manager_cast(PyObject *obj) -> PyBackendManagerObject * {
    return reinterpret_cast<PyBackendManagerObject *>(obj);
}

void py_backend_manager_invalidate(PyBackendManagerObject *mgr) {
    if (mgr->backend_obj != nullptr) {
        auto *backend = py_backend_cast(mgr->backend_obj);
        backend->backend = nullptr;
        Py_CLEAR(backend->owner);
    }
    Py_CLEAR(mgr->backend_obj);
}

auto py_backend_manager_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *mgr = py_backend_manager_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(mgr->owner);
    Py_VISIT(mgr->backend_obj);
    return 0;
}

auto py_backend_manager_clear(PyObject *self) -> int {
    auto *mgr = py_backend_manager_cast(self);
    if (mgr->backend != nullptr) {
        clingo_backend_close(mgr->backend);
        mgr->backend = nullptr;
    }
    py_backend_manager_invalidate(mgr);
    Py_CLEAR(mgr->owner);
    return 0;
}

auto py_backend_manager_enter(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *mgr = py_backend_manager_cast(self);
    if (mgr->backend != nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "backend is already active");
        return nullptr;
    }
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    if (!clingo_control_backend(mgr->ctl, &mgr->backend)) {
        return py_handle_error();
    }
    auto obj = steal(py_backend_new(state->backend_type, mgr->backend, self));
    if (obj.get() == nullptr) {
        clingo_backend_close(mgr->backend);
        mgr->backend = nullptr;
        return nullptr;
    }

    Py_INCREF(obj.get());
    mgr->backend_obj = obj.get();
    return obj.release();
}

auto py_backend_manager_exit(PyObject *self, PyObject *args) -> PyObject * {
    PyObject *type = nullptr;
    PyObject *value = nullptr;
    PyObject *traceback = nullptr;
    if (PyArg_ParseTuple(args, "OOO:__exit__", &type, &value, &traceback) == 0) {
        return nullptr;
    }
    (void)type;
    (void)value;
    (void)traceback;

    auto *mgr = py_backend_manager_cast(self);
    auto *old = std::exchange(mgr->backend, nullptr);
    py_backend_manager_invalidate(mgr);
    if (old != nullptr && !clingo_backend_close(old)) {
        return py_handle_error();
    }
    Py_RETURN_FALSE;
}

auto py_backend_manager_methods = std::array{
    PyMethodDef{"__enter__", py_backend_manager_enter, METH_NOARGS, PyDoc_STR("")},
    PyMethodDef{"__exit__", py_backend_manager_exit, METH_VARARGS, PyDoc_STR("")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_backend_manager_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("A context manager to initialize and finalize a backend."))},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_backend_manager_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_backend_manager_clear)},
    PyType_Slot{Py_tp_methods, py_backend_manager_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_backend_manager_spec = PyType_Spec{
    "clingo.backend.BackendManager",
    sizeof(PyBackendManagerObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC,
    py_backend_manager_slots.data(),
};

auto backend_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<BackendModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->symbol_type);
    Py_VISIT(state->observer_type);
    Py_VISIT(state->backend_type);
    Py_VISIT(state->backend_manager_type);
    Py_VISIT(state->heuristic_type_enum);
    Py_VISIT(state->external_type_enum);
    Py_VISIT(state->theory_sequence_type_enum);
    return 0;
}

auto backend_module_clear(PyObject *module) -> int {
    auto *state = static_cast<BackendModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->symbol_type);
    Py_CLEAR(state->observer_type);
    Py_CLEAR(state->backend_type);
    Py_CLEAR(state->backend_manager_type);
    Py_CLEAR(state->heuristic_type_enum);
    Py_CLEAR(state->external_type_enum);
    Py_CLEAR(state->theory_sequence_type_enum);
    return 0;
}

void backend_module_free(void *module) {
    backend_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef backend_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.backend",
    PyDoc_STR("Functions and classes to observe or add ground statements."),
    static_cast<Py_ssize_t>(sizeof(BackendModuleState)),
    nullptr,
    nullptr,
    backend_module_traverse,
    backend_module_clear,
    backend_module_free,
};

} // namespace

auto py_backend_manager_new(PyObject *type, clingo_control_t *ctl, PyObject *owner) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    auto *obj = to->tp_alloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *mgr = py_backend_manager_cast(obj);
    mgr->ctl = ctl;
    mgr->backend = nullptr;
    mgr->owner = owner;
    mgr->backend_obj = nullptr;
    Py_XINCREF(owner);
    return obj;
}

auto py_backend_manager_type_ref() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.backend"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = get_state(mod.get());
    if (state == nullptr) {
        return nullptr;
    }
    return state->backend_manager_type;
}

auto py_observe(PyObject *module, clingo_control_t *ctl, PyObject *observer, bool preprocess) -> bool {
    auto *state = get_state(module);
    if (state == nullptr) {
        return false;
    }
    auto ctx = std::pair{observer, state};
    if (!with_gil_released([&]() { return clingo_control_observe(ctl, observer_trampoline(), &ctx, preprocess); })) {
        py_handle_error();
        return false;
    }
    return true;
}

void register_backend(pybind11::module &m) {
    auto backend_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &backend_def, &backend_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(backend_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    state->symbol_type = py_symbol_type_ref();
    if (state->symbol_type == nullptr) {
        throw py::error_already_set();
    }
    Py_INCREF(state->symbol_type);

    if (!add_int_enum(backend_mod.get(), "HeuristicType", heuristic_type_members, &state->heuristic_type_enum, {}) ||
        !add_int_enum(backend_mod.get(), "ExternalType", external_type_members, &state->external_type_enum, {}) ||
        !add_int_enum(backend_mod.get(), "TheorySequenceType", theory_sequence_type_members,
                      &state->theory_sequence_type_enum, {}) ||
        !add_type(backend_mod.get(), "Observer", &py_observer_spec, &state->observer_type) ||
        !add_type(backend_mod.get(), "Backend", &py_backend_spec, &state->backend_type) ||
        !add_type(backend_mod.get(), "BackendManager", &py_backend_manager_spec, &state->backend_manager_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
