#define PY_SSIZE_T_CLEAN

#include "app.hh"
#include "ast.hh"
#include "control.hh"
#include "solve.hh"
#include "stats.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/theory.h>

#include <array>
#include <cstring>

namespace PyClingo {

namespace {

using PyTheoryObject = struct {
    PyObject_HEAD clingo_theory_t theory;
    bool initialized;
};

using PyTheoryAssignmentObject = struct {
    PyObject_HEAD PyObject *owner;
    uint32_t thread_id;
};

using PyTheoryAssignmentIterObject = struct {
    PyObject_HEAD PyObject *owner;
    size_t index;
    uint32_t thread_id;
    bool init;
    bool has_value;
};

struct TheoryModuleState {
    PyObject *symbol_type{nullptr};
    PyObject *theory_type{nullptr};
    PyObject *assignment_type{nullptr};
    PyObject *assignment_iter_type{nullptr};
};

// NOTE: this is fine because the default constructed symbol does not need to be released.
using SymbolGuard = Guard<clingo_symbol_t, clingo_symbol_release>;

class ValueGuard {
  public:
    ValueGuard() = default;
    ~ValueGuard() {
        if (value_.type == clingo_theory_value_type_symbol) {
            clingo_symbol_release(value_.symbol);
        }
    }
    ValueGuard(ValueGuard const &) = delete;
    auto operator=(ValueGuard const &) -> ValueGuard & = delete;
    ValueGuard(ValueGuard &&other) noexcept : value_{std::exchange(other.value_, default_value_)} {}
    auto operator=(ValueGuard &&other) noexcept -> ValueGuard & {
        if (this != &other) {
            if (value_.type == clingo_theory_value_type_symbol) {
                clingo_symbol_release(value_.symbol);
            }
            value_ = std::exchange(other.value_, default_value_);
        }
        return *this;
    }
    auto get() -> clingo_theory_value_t & { return value_; }
    [[nodiscard]] auto get() const -> clingo_theory_value_t const & { return value_; }
    void destroy() {
        if (value_.type == clingo_theory_value_type_symbol) {
            clingo_symbol_release(value_.symbol);
        }
        value_ = default_value_;
    }

  private:
    static constexpr auto default_value_ = clingo_theory_value_t{clingo_theory_value_type_int, {0}};
    clingo_theory_value_t value_{default_value_};
};

auto get_state(PyObject *module) -> TheoryModuleState * {
    auto *state = static_cast<TheoryModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "theory module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_self(PyObject *self) -> TheoryModuleState * {
    auto *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve theory module");
        return nullptr;
    }
    return get_state(mod);
}

auto py_theory_cast(PyObject *obj) -> PyTheoryObject * {
    return reinterpret_cast<PyTheoryObject *>(obj);
}

auto py_theory_check(PyObject *obj) -> PyTheoryObject * {
    auto *ret = py_theory_cast(obj);
    if (!ret->initialized) {
        PyErr_SetString(PyExc_RuntimeError, "theory is not initialized");
        return nullptr;
    }
    return ret;
}

auto py_theory_assignment_cast(PyObject *obj) -> PyTheoryAssignmentObject * {
    return reinterpret_cast<PyTheoryAssignmentObject *>(obj);
}

auto py_theory_assignment_iter_cast(PyObject *obj) -> PyTheoryAssignmentIterObject * {
    return reinterpret_cast<PyTheoryAssignmentIterObject *>(obj);
}

auto parse_symbol(TheoryModuleState *state, PyObject *obj, clingo_symbol_t *sym, char const *what) -> bool {
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

auto parse_string_sequence(PyObject *obj, PyGuard &seq, PyBuffer<clingo_string_t> &out) -> bool {
    char const *what = "files must be a sequence of strings";

    auto seq_fast = steal(PySequence_Fast(obj, what));
    if (seq_fast.get() == nullptr) {
        return false;
    }

    seq = steal(PySequence_Tuple(seq_fast.get()));
    if (seq.get() == nullptr) {
        return false;
    }

    auto n = PyTuple_GET_SIZE(seq.get());
    if (n < 0) {
        return false;
    }

    auto count = static_cast<size_t>(n);
    if (!out.alloc(count)) {
        return false;
    }

    auto *it = out.data();
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *item = PyTuple_GET_ITEM(seq.get(), i);
        if (PyUnicode_Check(item) == 0) {
            PyErr_SetString(PyExc_TypeError, "files must be a sequence of strings");
            return false;
        }
        Py_ssize_t len = 0;
        const auto *s = PyUnicode_AsUTF8AndSize(item, &len);
        if (s == nullptr) {
            return false;
        }
        *it++ = clingo_string_t{s, static_cast<size_t>(len)};
    }
    return true;
}

auto py_value_to_object(TheoryModuleState *state, clingo_theory_value_t const &value) -> PyObject * {
    switch (static_cast<clingo_theory_value_type_e>(value.type)) {
        case clingo_theory_value_type_int: {
            return PyLong_FromLong(value.int_number);
        }
        case clingo_theory_value_type_double: {
            return PyFloat_FromDouble(value.double_number);
        }
        case clingo_theory_value_type_symbol: {
            return py_symbol_new(state->symbol_type, value.symbol, true);
        }
    }
    PyErr_SetString(PyExc_RuntimeError, "invalid theory value type");
    return nullptr;
}

auto py_symbol_value_pair(TheoryModuleState *state, clingo_symbol_t symbol, clingo_theory_value_t const &value)
    -> PyObject * {
    auto sym_obj = steal(py_symbol_new(state->symbol_type, symbol, true));
    if (sym_obj.get() == nullptr) {
        return nullptr;
    }
    auto val_obj = steal(py_value_to_object(state, value));
    if (val_obj == nullptr) {
        return nullptr;
    }
    auto ret = steal(PyTuple_New(2));
    if (ret == nullptr) {
        return nullptr;
    }
    PyTuple_SET_ITEM(ret.get(), 0, sym_obj.release());
    PyTuple_SET_ITEM(ret.get(), 1, val_obj.release());
    return ret.release();
}

// TheoryAssignmentIter

auto py_theory_assignment_iter_new(PyObject *type, PyObject *owner, uint32_t thread_id) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    auto *obj = to->tp_alloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = py_theory_assignment_iter_cast(obj);
    a->owner = owner;
    a->thread_id = thread_id;
    a->init = true;
    a->has_value = true;
    a->index = 0;
    Py_INCREF(owner);
    return obj;
}

auto py_theory_assignment_iter_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *a = py_theory_assignment_iter_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(a->owner);
    return 0;
}

auto py_theory_assignment_iter_clear(PyObject *self) -> int {
    auto *a = py_theory_assignment_iter_cast(self);
    Py_CLEAR(a->owner);
    return 0;
}

auto py_theory_assignment_iter_next(PyObject *self) -> PyObject * {
    auto *a = py_theory_assignment_iter_cast(self);
    if (a->owner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment iterator is invalid");
        return nullptr;
    }
    auto *o = py_theory_check(a->owner);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.assignment_next == nullptr || o->theory.assignment_get_value == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "assignment not implemented");
        return nullptr;
    }

    if (a->has_value) {
        if (!o->theory.assignment_next(o->theory.self, a->thread_id, &a->init, &a->index, &a->has_value)) {
            return py_handle_error();
        }
        if (a->has_value) {
            auto symbol = Guard<clingo_symbol_t, clingo_symbol_release>{};
            auto value = ValueGuard{};
            bool has_value = true;
            if (!o->theory.assignment_get_value(o->theory.self, a->thread_id, a->index, &symbol.get(), &value.get(),
                                                &has_value)) {
                return py_handle_error();
            }
            if (!has_value) {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
            auto *state = get_state_from_self(self);
            if (state == nullptr) {
                return nullptr;
            }
            return py_symbol_value_pair(state, symbol.get(), value.get());
        }
    }

    PyErr_SetNone(PyExc_StopIteration);
    return nullptr;
}

auto py_theory_assignment_iter_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Iterator over theory assignment values."))},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(PyObject_SelfIter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_theory_assignment_iter_next)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_theory_assignment_iter_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_theory_assignment_iter_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{0, nullptr},
};

auto py_theory_assignment_iter_spec = PyType_Spec{
    "clingo.theory._TheoryAssignmentIter",
    sizeof(PyTheoryAssignmentIterObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_assignment_iter_slots.data(),
};

// TheoryAssignment

auto py_theory_assignment_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *a = py_theory_assignment_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(a->owner);
    return 0;
}

auto py_theory_assignment_clear(PyObject *self) -> int {
    auto *a = py_theory_assignment_cast(self);
    Py_CLEAR(a->owner);
    return 0;
}

auto py_theory_assignment_new(PyObject *type, PyObject *owner, uint32_t thread_id) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    auto *obj = to->tp_alloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = py_theory_assignment_cast(obj);
    a->owner = owner;
    a->thread_id = thread_id;
    Py_INCREF(owner);
    return obj;
}

auto py_theory_assignment_get(PyObject *self, PyObject *arg) -> PyObject * {
    auto *a = py_theory_assignment_cast(self);
    if (a->owner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment is invalid");
        return nullptr;
    }
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *o = py_theory_check(a->owner);
    if (o == nullptr) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!parse_symbol(state, arg, &sym, "symbol")) {
        return nullptr;
    }

    if (o->theory.lookup_symbol == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "lookup_symbol not implemented");
        return nullptr;
    }

    size_t index = 0;
    bool found = false;
    if (!o->theory.lookup_symbol(o->theory.self, sym, &index, &found)) {
        return py_handle_error();
    }
    if (!found) {
        Py_RETURN_NONE;
    }
    return PyLong_FromSize_t(index);
}

auto py_theory_assignment_at(PyObject *self, PyObject *arg) -> PyObject * {
    auto *a = py_theory_assignment_cast(self);
    if (a->owner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment is invalid");
        return nullptr;
    }
    auto *o = py_theory_check(a->owner);
    if (o == nullptr) {
        return nullptr;
    }

    size_t index = 0;
    bool is_index = PyIndex_Check(arg) != 0;
    if (is_index) {
        if (!parse_num(arg, &index, "index")) {
            return nullptr;
        }
    } else {
        auto *state = get_state_from_self(self);
        if (state == nullptr) {
            return nullptr;
        }

        clingo_symbol_t symbol = 0;
        auto is_symbol = py_symbol_check(state->symbol_type, arg, &symbol);
        if (is_symbol < 0) {
            return nullptr;
        }
        if (is_symbol == 0) {
            PyErr_SetString(PyExc_TypeError, "index must be an integer or Symbol");
            return nullptr;
        }

        if (o->theory.lookup_symbol == nullptr) {
            PyErr_SetString(PyExc_NotImplementedError, "lookup_symbol not implemented");
            return nullptr;
        }

        bool found = false;
        if (!o->theory.lookup_symbol(o->theory.self, symbol, &index, &found)) {
            return py_handle_error();
        }
        if (!found) {
            PyErr_SetString(PyExc_IndexError, "invalid symbol");
            return nullptr;
        }
    }

    if (o->theory.assignment_get_value == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "assignment_get_value not implemented");
        return nullptr;
    }

    SymbolGuard symbol{};
    ValueGuard value{};
    bool has_value = true;
    if (!o->theory.assignment_get_value(o->theory.self, a->thread_id, index, &symbol.get(), &value.get(), &has_value)) {
        return py_handle_error();
    }
    if (!has_value) {
        PyErr_SetString(PyExc_IndexError, "invalid index");
        return nullptr;
    }
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return py_value_to_object(state, value.get());
}

auto py_theory_assignment_iter(PyObject *self) -> PyObject * {
    auto *a = py_theory_assignment_cast(self);
    if (a->owner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment is invalid");
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    return py_theory_assignment_iter_new(state->assignment_iter_type, a->owner, a->thread_id);
}

auto py_theory_init(PyObject *self, PyObject *args, PyObject *kwargs) -> int {
    PyObject *lib_obj = nullptr;
    PyObject *create_obj = nullptr;
    static constexpr auto kwlist = std::array<char const *, 3>{"", "", nullptr};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO|:Theory", cast_kw(kwlist), &lib_obj, &create_obj) == 0) {
        return -1;
    }

    auto *o = py_theory_cast(self);
    if (o->initialized) {
        PyErr_SetString(PyExc_RuntimeError, "theory is already initialized");
        return -1;
    }
    if (!PyCapsule_CheckExact(create_obj)) {
        PyErr_SetString(PyExc_TypeError, "create must be a capsule");
        return -1;
    }
    const auto *cap_name = PyCapsule_GetName(create_obj);
    if (cap_name == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_ValueError, "theory initialization function expected");
        }
        return -1;
    }
    if (std::strcmp(cap_name, "clingo_theory_create") != 0) {
        PyErr_SetString(PyExc_ValueError, "theory initialization function expected");
        return -1;
    }
    auto *ptr = PyCapsule_GetPointer(create_obj, "clingo_theory_create");
    if (ptr == nullptr) {
        return -1;
    }

    auto create = reinterpret_cast<bool (*)(clingo_lib_t *, clingo_theory_t *)>(ptr);
    if (create == nullptr) {
        PyErr_SetString(PyExc_ValueError, "create function must not be null");
        return -1;
    }

    auto *lib = py_cast<Library>(lib_obj);
    if (lib == nullptr) {
        return -1;
    }
    if (!create(*lib, &o->theory)) {
        py_handle_error();
        return -1;
    }
    o->initialized = true;
    return 0;
}

auto py_theory_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    return 0;
}

auto py_theory_clear([[maybe_unused]] PyObject *self) -> int {
    auto *o = py_theory_cast(self);
    if (o->theory.destroy != nullptr && o->theory.self != nullptr) {
        o->theory.destroy(o->theory.self);
    }
    std::memset(&o->theory, 0, sizeof(o->theory));
    return 0;
}

auto py_theory_register(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.register_theory == nullptr) {
        Py_RETURN_NONE;
    }

    auto *ctl = py_cast<Control>(arg);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!o->theory.register_theory(o->theory.self, ctl->c_ptr())) {
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

auto rewrite_ast_cb(clingo_ast_t *ast, void *data) -> bool {
    auto *callback = static_cast<PyObject *>(data);
    py::object stm;
    try {
        stm = AST::convert_stm(ast);
    } catch (py::error_already_set &e) {
        e.restore();
        return store_python_error("converting AST to Python object failed");
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return false;
    }
    auto ret = steal(PyObject_CallFunctionObjArgs(callback, stm.ptr(), nullptr));
    if (ret.get() == nullptr) {
        return store_python_error("rewrite callback failed");
    }
    return true;
}

auto py_theory_rewrite(PyObject *self, PyObject *args) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }

    PyObject *stm_obj = nullptr;
    PyObject *callback = nullptr;
    if (PyArg_ParseTuple(args, "OO:rewrite", &stm_obj, &callback) == 0) {
        return nullptr;
    }
    if (PyCallable_Check(callback) == 0) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }

    if (o->theory.rewrite_ast == nullptr) {
        auto ret = steal(PyObject_CallFunctionObjArgs(callback, stm_obj, nullptr));
        if (ret.get() == nullptr) {
            return nullptr;
        }
        Py_RETURN_NONE;
    }

    clingo_ast_t *stm = nullptr;
    try {
        stm = AST::convert_stm(stm_obj);
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return nullptr;
    }
    if (!o->theory.rewrite_ast(o->theory.self, stm, rewrite_ast_cb, callback)) {
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

struct ParseData {
    clingo_program_t *prg;
    clingo_theory_t const *theory;
};

auto parse_rewrite_cb(clingo_ast_t *ast, void *data) -> bool {
    auto *d = static_cast<ParseData *>(data);
    if (d->theory->rewrite_ast == nullptr) {
        return clingo_program_add(d->prg, ast);
    }
    return d->theory->rewrite_ast(
        d->theory->self, ast,
        [](clingo_ast_t *rewritten, void *dst) -> bool {
            return clingo_program_add(static_cast<clingo_program_t *>(dst), rewritten);
        },
        d->prg);
}

auto py_theory_rewrite_string(PyObject *self, PyObject *args) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }

    PyObject *lib_obj = nullptr;
    PyObject *ctl_obj = nullptr;
    char const *text = nullptr;
    Py_ssize_t n = 0;
    if (PyArg_ParseTuple(args, "OOs#:rewrite_string", &lib_obj, &ctl_obj, &text, &n) == 0) {
        return nullptr;
    }

    auto *lib = py_cast<Library>(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }
    auto *ctl = py_cast<Control>(ctl_obj);
    if (ctl == nullptr) {
        return nullptr;
    }

    clingo_program_t *prg = nullptr;
    if (!clingo_program_new(*lib, &prg)) {
        return py_handle_error();
    }
    auto guard = Guard<clingo_program_t *, clingo_program_free>(prg);

    ParseData data{prg, &o->theory};
    if (!clingo_ast_parse_string(*lib, text, static_cast<size_t>(n), ctl->c_ptr(), parse_rewrite_cb, &data)) {
        return py_handle_error();
    }
    if (!clingo_control_join(ctl->c_ptr(), prg)) {
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

auto py_theory_rewrite_files(PyObject *self, PyObject *args) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }

    PyObject *lib_obj = nullptr;
    PyObject *ctl_obj = nullptr;
    PyObject *files_obj = nullptr;
    if (PyArg_ParseTuple(args, "OOO:rewrite_files", &lib_obj, &ctl_obj, &files_obj) == 0) {
        return nullptr;
    }

    PyGuard seq;
    PyBuffer<clingo_string_t> files;
    if (!parse_string_sequence(files_obj, seq, files)) {
        return nullptr;
    }

    auto *lib = py_cast<Library>(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }
    auto *ctl = py_cast<Control>(ctl_obj);
    if (ctl == nullptr) {
        return nullptr;
    }

    clingo_program_t *prg = nullptr;
    if (!clingo_program_new(*lib, &prg)) {
        return py_handle_error();
    }
    auto guard = Guard<clingo_program_t *, clingo_program_free>(prg);

    ParseData data{prg, &o->theory};
    if (!clingo_ast_parse_files(*lib, files.data(), files.size(), ctl->c_ptr(), parse_rewrite_cb, &data)) {
        return py_handle_error();
    }
    if (!clingo_control_join(ctl->c_ptr(), prg)) {
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

auto py_theory_register_options(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.register_options == nullptr) {
        Py_RETURN_NONE;
    }

    clingo_options_t *opt = nullptr;
    try {
        opt = convert_options(arg);
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_TypeError, e.what());
        return nullptr;
    }
    if (!o->theory.register_options(o->theory.self, opt)) {
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

auto py_theory_validate_options(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.validate_options != nullptr) {
        if (!o->theory.validate_options(o->theory.self)) {
            return py_handle_error();
        }
    }
    Py_RETURN_NONE;
}

auto py_theory_prepare(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.prepare == nullptr) {
        Py_RETURN_NONE;
    }

    auto *ctl = py_cast<Control>(arg);
    if (ctl == nullptr) {
        return nullptr;
    }
    if (!o->theory.prepare(o->theory.self, ctl->c_ptr())) {
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

auto py_theory_on_model(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.on_model == nullptr) {
        Py_RETURN_NONE;
    }

    auto *model = py_cast<Model>(arg);
    if (model == nullptr) {
        return nullptr;
    }
    if (!o->theory.on_model(o->theory.self, const_cast<clingo_model_t *>(model->c_ptr()))) {
        return py_handle_error();
    }

    Py_RETURN_NONE;
}

auto py_theory_on_stats(PyObject *self, PyObject *args) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.on_stats == nullptr) {
        Py_RETURN_NONE;
    }

    PyObject *step = nullptr;
    PyObject *accu = nullptr;
    if (PyArg_ParseTuple(args, "OO:on_stats", &step, &accu) == 0) {
        return nullptr;
    }
    (void)accu;

    auto *type = py_stats_type_ref();
    if (type == nullptr) {
        return nullptr;
    }

    clingo_stats_t *stats = nullptr;
    if (!py_stats_get(type, step, &stats, nullptr)) {
        return nullptr;
    }

    if (!o->theory.on_stats(o->theory.self, stats)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_theory_value(PyObject *self, PyObject *args) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }

    PyObject *thread_id_obj = nullptr;
    PyObject *symbol_obj = nullptr;
    if (PyArg_ParseTuple(args, "OO:value", &thread_id_obj, &symbol_obj) == 0) {
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    uint32_t thread_id = 0;
    if (!parse_num(thread_id_obj, &thread_id, "thread_id")) {
        return nullptr;
    }

    clingo_symbol_t symbol = 0;
    if (!parse_symbol(state, symbol_obj, &symbol, "symbol")) {
        return nullptr;
    }

    if (o->theory.lookup_symbol == nullptr || o->theory.assignment_get_value == nullptr) {
        Py_RETURN_NONE;
    }

    size_t index = 0;
    bool found = false;
    if (!o->theory.lookup_symbol(o->theory.self, symbol, &index, &found)) {
        return py_handle_error();
    }
    if (!found) {
        Py_RETURN_NONE;
    }

    auto value = ValueGuard{};
    if (!o->theory.assignment_get_value(o->theory.self, thread_id, index, nullptr, &value.get(), &found)) {
        return py_handle_error();
    }
    if (!found) {
        Py_RETURN_NONE;
    }
    return py_value_to_object(state, value.get());
}

auto py_theory_assignment(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    uint32_t thread_id = 0;
    if (!parse_num(arg, &thread_id, "thread_id")) {
        return nullptr;
    }

    if (o->theory.assignment_next == nullptr || o->theory.assignment_get_value == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "assignment not implemented");
        return nullptr;
    }

    return py_theory_assignment_new(state->assignment_type, self, thread_id);
}

auto py_theory_get_version(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.info == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
        return nullptr;
    }

    int major = 0;
    int minor = 0;
    int revision = 0;
    if (!o->theory.info(o->theory.self, nullptr, &major, &minor, &revision, nullptr)) {
        return py_handle_error();
    }
    return Py_BuildValue("(iii)", major, minor, revision);
}

auto py_theory_get_name(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.info == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
        return nullptr;
    }

    clingo_string_t name;
    if (!o->theory.info(o->theory.self, &name, nullptr, nullptr, nullptr, nullptr)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size));
}

auto py_theory_get_commit_hash(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    if (o->theory.info == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
        return nullptr;
    }

    clingo_string_t hash;
    if (!o->theory.info(o->theory.self, nullptr, nullptr, nullptr, nullptr, &hash)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(hash.data, static_cast<Py_ssize_t>(hash.size));
}

auto py_theory_get_has_assignment(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    auto ok = o->theory.assignment_next != nullptr && o->theory.assignment_get_value != nullptr;
    if (ok) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_assignment_methods = std::array{
    PyMethodDef{"get", py_theory_assignment_get, METH_O, PyDoc_STR("")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_theory_assignment_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Assignment of theory values."))},
    PyType_Slot{Py_tp_methods, py_theory_assignment_methods.data()},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_theory_assignment_iter)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_theory_assignment_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_theory_assignment_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_mp_subscript, reinterpret_cast<void *>(py_theory_assignment_at)},
    PyType_Slot{0, nullptr},
};

auto py_theory_assignment_spec = PyType_Spec{
    "clingo.theory.TheoryAssignment",
    sizeof(PyTheoryAssignmentObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_assignment_slots.data(),
};

auto py_theory_methods = std::array{
    PyMethodDef{"register", py_theory_register, METH_O, PyDoc_STR("")},
    PyMethodDef{"rewrite", py_theory_rewrite, METH_VARARGS, PyDoc_STR("")},
    PyMethodDef{
        "rewrite_string",
        py_theory_rewrite_string,
        METH_VARARGS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "rewrite_files",
        py_theory_rewrite_files,
        METH_VARARGS,
        PyDoc_STR(""),
    },
    PyMethodDef{"register_options", py_theory_register_options, METH_O, PyDoc_STR("")},
    PyMethodDef{"validate_options", py_theory_validate_options, METH_NOARGS, PyDoc_STR("")},
    PyMethodDef{"prepare", py_theory_prepare, METH_O, PyDoc_STR("")},
    PyMethodDef{"on_model", py_theory_on_model, METH_O, PyDoc_STR("")},
    PyMethodDef{"on_stats", py_theory_on_stats, METH_VARARGS, PyDoc_STR("")},
    PyMethodDef{"value", py_theory_value, METH_VARARGS, PyDoc_STR("")},
    PyMethodDef{"assignment", py_theory_assignment, METH_O, PyDoc_STR("")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_theory_getset = std::array{
    PyGetSetDef{"version", py_theory_get_version, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{"name", py_theory_get_name, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{"commit_hash", py_theory_get_commit_hash, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{"has_assignment", py_theory_get_has_assignment, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_slots = std::array{
    PyType_Slot{
        Py_tp_doc,
        PyDoc_STR(const_cast<char *>("Object to call functions from a C-library implementing a custom theory.")),
    },
    PyType_Slot{Py_tp_init, reinterpret_cast<void *>(py_theory_init)},
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(PyType_GenericNew)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_theory_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_theory_clear)},
    PyType_Slot{Py_tp_methods, py_theory_methods.data()},
    PyType_Slot{Py_tp_getset, py_theory_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_spec = PyType_Spec{
    "clingo.theory.Theory", sizeof(PyTheoryObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, py_theory_slots.data(),
};

auto theory_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<TheoryModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->symbol_type);
    Py_VISIT(state->theory_type);
    Py_VISIT(state->assignment_type);
    Py_VISIT(state->assignment_iter_type);
    return 0;
}

auto theory_module_clear(PyObject *module) -> int {
    auto *state = static_cast<TheoryModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->symbol_type);
    Py_CLEAR(state->theory_type);
    Py_CLEAR(state->assignment_type);
    Py_CLEAR(state->assignment_iter_type);
    return 0;
}

void theory_module_free(void *module) {
    theory_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef theory_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.theory",
    PyDoc_STR("This module allows for using theories implemented in C from Python."),
    static_cast<Py_ssize_t>(sizeof(TheoryModuleState)),
    nullptr,
    nullptr,
    theory_module_traverse,
    theory_module_clear,
    theory_module_free,
};

} // namespace

void register_theory(pybind11::module &m) {
    auto theory_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &theory_def, &theory_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(theory_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    state->symbol_type = py_symbol_type_ref();
    if (state->symbol_type == nullptr) {
        throw py::error_already_set();
    }
    Py_INCREF(state->symbol_type);

    if (!add_type(theory_mod.get(), "TheoryAssignmentIter", &py_theory_assignment_iter_spec,
                  &state->assignment_iter_type) ||
        !add_type(theory_mod.get(), "TheoryAssignment", &py_theory_assignment_spec, &state->assignment_type) ||
        !add_type(theory_mod.get(), "Theory", &py_theory_spec, &state->theory_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
