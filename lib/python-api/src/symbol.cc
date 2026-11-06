#define PY_SSIZE_T_CLEAN

#include "symbol.hh"

#include <utility>

namespace PyClingo {

namespace {

constexpr auto symbol_type_members = std::array<IntEnumMember, 6>{
    IntEnumMember{"Number", static_cast<int>(clingo_symbol_type_number)},
    IntEnumMember{"Infimum", static_cast<int>(clingo_symbol_type_infimum)},
    IntEnumMember{"Supremum", static_cast<int>(clingo_symbol_type_supremum)},
    IntEnumMember{"String", static_cast<int>(clingo_symbol_type_string)},
    IntEnumMember{"Tuple", static_cast<int>(clingo_symbol_type_tuple)},
    IntEnumMember{"Function", static_cast<int>(clingo_symbol_type_function)},
};

constexpr size_t symbol_type_count = symbol_type_members.size();

using PySymbolObject = struct {
    PyObject_HEAD clingo_symbol_t sym;
};

struct SymbolModuleState {
    PyObject *symbol_type{nullptr};
    std::array<PyObject *, symbol_type_count> vals{};
};

auto get_symbol_module_state(PyObject *module) -> SymbolModuleState * {
    auto *state = static_cast<SymbolModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "symbol module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_symbol_type(SymbolModuleState *state) -> PyTypeObject * {
    return reinterpret_cast<PyTypeObject *>(state->symbol_type);
}

auto get_symbol_module_state_from_self(PyObject *self) -> SymbolModuleState * {
    PyObject *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve symbol module");
        return nullptr;
    }
    return get_symbol_module_state(mod);
}

// ---------- helpers ----------

auto py_symbol_from_clingo(clingo_symbol_t sym, PyTypeObject *symbol_type, bool acquire) -> PyObject * {
    auto *obj = PyObject_New(PySymbolObject, symbol_type);
    if (obj == nullptr) {
        if (!acquire) {
            clingo_symbol_release(sym);
        }
        return nullptr;
    }
    if (acquire) {
        clingo_symbol_acquire(sym);
    }
    obj->sym = sym;
    return reinterpret_cast<PyObject *>(obj);
}

auto symbol_type(clingo_symbol_t sym) -> clingo_symbol_type_e {
    return static_cast<clingo_symbol_type_e>(clingo_symbol_type(sym));
}

auto symbol_args_list(clingo_symbol_t sym, PyTypeObject *symbol_type) -> PyObject * {
    clingo_symbol_t const *args = nullptr;
    size_t size = 0;
    if (!clingo_symbol_arguments(sym, &args, &size)) {
        return py_handle_error();
    }
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(size)));
    if (list == nullptr) {
        return nullptr;
    }
    size_t i = 0;
    for (clingo_symbol_t arg : std::span{args, size}) {
        PyObject *it = py_symbol_from_clingo(arg, symbol_type, true);
        if (it == nullptr) {
            return nullptr;
        }
        PyList_SET_ITEM(list, static_cast<Py_ssize_t>(i), it);
        ++i;
    }
    return list.release();
}

auto cast_library(PyObject *obj) -> Library * {
    try {
        return &py::cast<Library &>(py::reinterpret_borrow<py::object>(obj));
    } catch (py::cast_error const &) {
        PyErr_SetString(PyExc_TypeError, "lib must be a Library");
        return nullptr;
    }
}

auto cast_symbol(PyObject *self) -> PySymbolObject * {
    return reinterpret_cast<PySymbolObject *>(self);
}

auto check_symbol(PyObject *obj, PyTypeObject *symbol_type) -> PySymbolObject * {
    if (!PyObject_TypeCheck(obj, symbol_type)) {
        return nullptr;
    }
    return cast_symbol(obj);
}

void py_symbol_dealloc(PyObject *self) {
    auto *s = cast_symbol(self);
    clingo_symbol_release(s->sym);
    auto *type = Py_TYPE(self);
    type->tp_free(self);
    Py_DECREF(type);
}

auto py_symbol_str(PyObject *self) -> PyObject * {
    auto *s = cast_symbol(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!clingo_symbol_to_string(s->sym, bld)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_string_escape(clingo_string_builder_t *bld, clingo_string_t value) -> bool {
    auto str =
        Guard<PyObject *, Py_DECREF>(PyUnicode_FromStringAndSize(value.data, static_cast<Py_ssize_t>(value.size)));
    if (str == nullptr) {
        auto msg = std::string_view{"failed to create string"};
        clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
        PyErr_Clear();
        return false;
    }
    auto rep = Guard<PyObject *, Py_DECREF>(PyObject_Repr(str));
    if (rep == nullptr) {
        auto msg = std::string_view{"failed to create string representation"};
        clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
        PyErr_Clear();
        return false;
    }
    Py_ssize_t size = 0;
    auto const *val = PyUnicode_AsUTF8AndSize(rep, &size);
    if (val == nullptr) {
        auto msg = std::string_view{"failed to convert string representation to UTF-8"};
        clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
        PyErr_Clear();
        return false;
    }
    return clingo_string_builder_append(bld, val, static_cast<size_t>(size));
}

auto py_symbol_build_repr(clingo_string_builder_t *bld, clingo_symbol_t sym) -> bool;

auto py_symbol_build_repr_tuple(clingo_string_builder_t *bld, clingo_symbol_t sym) -> bool {
    clingo_symbol_t const *args = nullptr;
    size_t size = 0;
    if (!clingo_symbol_arguments(sym, &args, &size)) {
        return false;
    }
    auto pre = std::string_view{"["};
    auto pst = std::string_view{"]"};
    auto cmm = std::string_view{","};
    if (!clingo_string_builder_append(bld, pre.data(), pre.size())) {
        return false;
    }
    bool sep = false;
    for (auto arg : std::span{args, size}) {
        if (sep) {
            if (!clingo_string_builder_append(bld, cmm.data(), cmm.size())) {
                return false;
            }
        } else {
            sep = true;
        }
        if (!py_symbol_build_repr(bld, arg)) {
            return false;
        }
    }
    return clingo_string_builder_append(bld, pst.data(), pst.size());
};

auto py_symbol_build_repr(clingo_string_builder_t *bld, clingo_symbol_t sym) -> bool {
    switch (symbol_type(sym)) {
        case clingo_symbol_type_infimum: {
            auto val = std::string_view{"Infimum"};
            return clingo_string_builder_append(bld, val.data(), val.size());
        }
        case clingo_symbol_type_supremum: {
            auto val = std::string_view{"Supremum"};
            return clingo_string_builder_append(bld, val.data(), val.size());
        }
        case clingo_symbol_type_number: {
            auto pre = std::string_view{"Number("};
            auto pst = std::string_view{")"};
            return clingo_string_builder_append(bld, pre.data(), pre.size()) && clingo_symbol_to_string(sym, bld) &&
                   clingo_string_builder_append(bld, pst.data(), pst.size());
        }
        case clingo_symbol_type_string: {
            auto pre = std::string_view{"String("};
            auto pst = std::string_view{")"};
            clingo_string_t str;
            if (!clingo_symbol_string(sym, &str)) {
                return false;
            }
            return clingo_string_builder_append(bld, pre.data(), pre.size()) && py_string_escape(bld, str) &&
                   clingo_string_builder_append(bld, pst.data(), pst.size());
        }
        case clingo_symbol_type_tuple: {
            auto pre = std::string_view{"Tuple_("};
            auto pst = std::string_view{")"};
            return clingo_string_builder_append(bld, pre.data(), pre.size()) && py_symbol_build_repr_tuple(bld, sym) &&
                   clingo_string_builder_append(bld, pst.data(), pst.size());
        }
        case clingo_symbol_type_function: {
            auto pre = std::string_view{"Function("};
            auto pst = std::string_view{")"};
            auto cmm = std::string_view{","};
            clingo_string_t name;
            if (!clingo_symbol_name(sym, &name)) {
                return false;
            }
            bool pos = false;
            if (!clingo_symbol_is_positive(sym, &pos)) {
                return false;
            }
            auto sgn = std::string_view{pos ? "True" : "False"};
            return clingo_string_builder_append(bld, pre.data(), pre.size()) && //
                   py_string_escape(bld, name) &&                               //
                   clingo_string_builder_append(bld, cmm.data(), cmm.size()) && //
                   py_symbol_build_repr_tuple(bld, sym) &&                      //
                   clingo_string_builder_append(bld, cmm.data(), cmm.size()) && //
                   clingo_string_builder_append(bld, sgn.data(), sgn.size()) && //
                   clingo_string_builder_append(bld, pst.data(), pst.size());
        }
    }
    // cannot happen
    Py_RETURN_NONE;
};

auto py_symbol_repr(PyObject *self) -> PyObject * {
    auto *s = cast_symbol(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>();
    if (!clingo_string_builder_new(&bld.get())) {
        return py_handle_error();
    }
    if (!py_symbol_build_repr(bld.get(), s->sym)) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_symbol_hash(PyObject *self) -> Py_hash_t {
    auto *s = cast_symbol(self);
    return static_cast<Py_hash_t>(clingo_symbol_hash(s->sym));
}

auto py_symbol_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    if (!PyObject_TypeCheck(b, Py_TYPE(a))) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    auto *sa = cast_symbol(a);
    auto *sb = cast_symbol(b);

    bool res = false;
    switch (op) {
        case Py_EQ:
            res = clingo_symbol_equal(sa->sym, sb->sym);
            break;
        case Py_NE:
            res = !clingo_symbol_equal(sa->sym, sb->sym);
            break;
        case Py_LT:
            res = clingo_symbol_compare(sa->sym, sb->sym) < 0;
            break;
        case Py_LE:
            res = clingo_symbol_compare(sa->sym, sb->sym) <= 0;
            break;
        case Py_GT:
            res = clingo_symbol_compare(sa->sym, sb->sym) > 0;
            break;
        case Py_GE:
            res = clingo_symbol_compare(sa->sym, sb->sym) >= 0;
            break;
        default:
            Py_RETURN_NOTIMPLEMENTED;
    }
    if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_symbol_match(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 4>{"name", "arity", "is_positive", nullptr};

    const char *name = nullptr;
    Py_ssize_t name_len = 0;
    Py_ssize_t arity = 0;
    int is_positive = 1;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "s#|np:match", cast_kw(kwlist), &name, &name_len, &arity,
                                    &is_positive) != 0) {
        auto *s = cast_symbol(self);
        if (symbol_type(s->sym) != clingo_symbol_type_function) {
            Py_RETURN_FALSE;
        }

        clingo_string_t n;
        clingo_symbol_t const *fn_args = nullptr;
        size_t fn_arity = 0;
        bool fn_pos = false;
        if (!clingo_symbol_name(s->sym, &n) || !clingo_symbol_arguments(s->sym, &fn_args, &fn_arity) ||
            !clingo_symbol_is_positive(s->sym, &fn_pos)) {
            return py_handle_error();
        }

        bool ok = (std::string_view(n.data, n.size) == std::string_view(name, static_cast<size_t>(name_len))) &&
                  std::cmp_equal(arity, fn_arity) && (fn_pos == (is_positive != 0));
        if (ok) {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
    PyErr_Clear();
    {
        static constexpr auto kwlist = std::array<char const *, 2>{"arity", nullptr};
        arity = 0;
        if (PyArg_ParseTupleAndKeywords(args, kwargs, "|n:match", cast_kw(kwlist), &arity) == 0) {
            return nullptr;
        }
        auto *s = cast_symbol(self);
        if (symbol_type(s->sym) != clingo_symbol_type_tuple) {
            Py_RETURN_FALSE;
        }
        clingo_symbol_t const *tuple_args = nullptr;
        size_t tuple_arity = 0;
        if (!clingo_symbol_arguments(s->sym, &tuple_args, &tuple_arity)) {
            return py_handle_error();
        }
        if (std::cmp_equal(tuple_arity, arity)) {
            Py_RETURN_TRUE;
        }
        Py_RETURN_FALSE;
    }
}

auto py_symbol_get_signature(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *s = cast_symbol(self);
    if (symbol_type(s->sym) != clingo_symbol_type_function) {
        Py_RETURN_NONE;
    }
    clingo_string_t name;
    clingo_symbol_t const *args = nullptr;
    size_t arity = 0;
    bool pos = false;
    if (!clingo_symbol_name(s->sym, &name) || !clingo_symbol_arguments(s->sym, &args, &arity) ||
        !clingo_symbol_is_positive(s->sym, &pos)) {
        return py_handle_error();
    }
    return Py_BuildValue("(s#nO)", name.data, static_cast<Py_ssize_t>(name.size), static_cast<Py_ssize_t>(arity),
                         pos ? Py_True : Py_False);
}

auto py_symbol_get_type(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = get_symbol_module_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto idx = static_cast<size_t>(symbol_type(cast_symbol(self)->sym));
    assert(idx < state->vals.size());
    auto *obj = state->vals[idx];
    if (obj == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "symbol type cache missing");
        return nullptr;
    }
    Py_INCREF(obj);
    return obj;
}

auto py_symbol_get_number(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *s = cast_symbol(self);
    if (symbol_type(s->sym) != clingo_symbol_type_number) {
        PyErr_SetString(PyExc_ValueError, "symbol is not a number");
        return nullptr;
    }
    int32_t num = 0;
    if (clingo_symbol_number(s->sym, &num)) {
        return PyLong_FromLong(num);
    }
    clingo_clear_error();
    auto str = Guard<PyObject *, Py_DECREF>(py_symbol_str(self));
    if (str == nullptr) {
        return nullptr;
    }
    return PyNumber_Long(str);
}

auto py_symbol_get_string(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *s = cast_symbol(self);
    if (symbol_type(s->sym) != clingo_symbol_type_string) {
        PyErr_SetString(PyExc_ValueError, "symbol is not a string");
        return nullptr;
    }
    clingo_string_t str;
    if (!clingo_symbol_string(s->sym, &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_symbol_get_name(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *s = cast_symbol(self);
    if (symbol_type(s->sym) != clingo_symbol_type_function) {
        PyErr_SetString(PyExc_ValueError, "symbol is not a function");
        return nullptr;
    }
    clingo_string_t name;
    if (!clingo_symbol_name(s->sym, &name)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size));
}

auto py_symbol_get_arguments(PyObject *self, [[maybe_unused]] void *closule) -> PyObject * {
    auto *s = cast_symbol(self);
    auto t = symbol_type(s->sym);
    if (t != clingo_symbol_type_tuple && t != clingo_symbol_type_function) {
        PyErr_SetString(PyExc_ValueError, "symbol is not a tuple or function");
        return nullptr;
    }
    auto *state = get_symbol_module_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    return symbol_args_list(s->sym, get_symbol_type(state));
}

auto py_symbol_get_arity(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *s = cast_symbol(self);
    auto t = symbol_type(s->sym);
    if (t != clingo_symbol_type_tuple && t != clingo_symbol_type_function) {
        PyErr_SetString(PyExc_ValueError, "symbol is not a tuple or function");
        return nullptr;
    }
    clingo_symbol_t const *args = nullptr;
    size_t size = 0;
    if (!clingo_symbol_arguments(s->sym, &args, &size)) {
        return py_handle_error();
    }
    return PyLong_FromSize_t(size);
}

auto py_symbol_get_is_positive(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *s = cast_symbol(self);
    auto t = symbol_type(s->sym);
    if (t != clingo_symbol_type_number && t != clingo_symbol_type_function) {
        PyErr_SetString(PyExc_ValueError, "symbol is not a number or function");
        return nullptr;
    }
    bool pos = false;
    if (!clingo_symbol_is_positive(s->sym, &pos)) {
        return py_handle_error();
    }
    if (pos) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_symbol_get_is_negative(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    PyObject *pos = py_symbol_get_is_positive(self, closure);
    if (pos == nullptr) {
        return nullptr;
    }
    int truth = PyObject_IsTrue(pos);
    Py_DECREF(pos);
    if (truth < 0) {
        return nullptr;
    }
    if (truth == 0) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_number([[maybe_unused]] PyObject *module, PyObject *args) -> PyObject * {
    PyObject *lib_obj = nullptr;
    PyObject *num_obj = nullptr;
    if (PyArg_ParseTuple(args, "OO:Number", &lib_obj, &num_obj) == 0) {
        return nullptr;
    }

    auto *lib = cast_library(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }

    int overflow = 0;
    long val = PyLong_AsLongAndOverflow(num_obj, &overflow);
    if (PyErr_Occurred() != nullptr) {
        return nullptr;
    }

    auto *state = get_symbol_module_state(module);
    if (state == nullptr) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (overflow == 0 && std::numeric_limits<int32_t>::min() <= val && val <= std::numeric_limits<int32_t>::max()) {
        sym = clingo_symbol_create_number(static_cast<int32_t>(val));
    } else {
        auto num_str = Guard<PyObject *, Py_DECREF>(PyObject_Str(num_obj));
        if (num_str == nullptr) {
            return nullptr;
        }
        Py_ssize_t n = 0;
        const char *s = PyUnicode_AsUTF8AndSize(num_str, &n);
        if (s == nullptr) {
            return nullptr;
        }

        if (!clingo_symbol_create_number_str(*lib, s, static_cast<size_t>(n), &sym)) {
            return py_handle_error();
        }
    }

    return py_symbol_from_clingo(sym, get_symbol_type(state), false);
}

auto py_string([[maybe_unused]] PyObject *module, PyObject *args) -> PyObject * {
    PyObject *lib_obj = nullptr;
    const char *s = nullptr;
    Py_ssize_t n = 0;
    if (PyArg_ParseTuple(args, "Os#:String", &lib_obj, &s, &n) == 0) {
        return nullptr;
    }

    auto *lib = cast_library(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }

    auto *state = get_symbol_module_state(module);
    if (state == nullptr) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!clingo_symbol_create_string(*lib, s, static_cast<size_t>(n), &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo(sym, get_symbol_type(state), false);
}

struct SymbolSeq {
    SymbolSeq() = default;
    SymbolSeq(SymbolSeq const &) = delete;
    auto operator=(SymbolSeq const &) -> SymbolSeq & = delete;
    SymbolSeq(SymbolSeq &&other) noexcept = delete;
    auto operator=(SymbolSeq &&other) noexcept -> SymbolSeq & = delete;

    ~SymbolSeq() {
        if (data != nullptr) {
            PyMem_Free(data);
        }
    }

    clingo_symbol_t *data{nullptr};
    size_t size{0};
};

auto py_symbol_sequence(PyObject *obj, PyTypeObject *symbol_type, SymbolSeq &out) -> bool {
    auto seq = Guard<PyObject *, Py_DECREF>(PySequence_Fast(obj, "arguments must be a sequence of Symbol"));
    if (seq == nullptr) {
        return false;
    }

    auto n = PySequence_Fast_GET_SIZE(seq);
    out.size = static_cast<size_t>(n);

    if (out.size > 0) {
        out.data = static_cast<clingo_symbol_t *>(PyMem_Malloc(sizeof(clingo_symbol_t) * out.size));
        if (out.data == nullptr) {
            PyErr_NoMemory();
            return false;
        }
    }

    auto **items = PySequence_Fast_ITEMS(seq.get());
    auto *it = out.data;
    for (auto *item : std::span{items, out.size}) {
        auto *s = check_symbol(item, symbol_type);
        if (s == nullptr) {
            PyErr_SetString(PyExc_TypeError, "arguments must contain only Symbol");
            return false;
        }
        *it++ = s->sym;
    }
    return true;
}

auto py_tuple([[maybe_unused]] PyObject *module, PyObject *args) -> PyObject * {
    PyObject *lib_obj = nullptr;
    PyObject *args_obj = nullptr;
    if (PyArg_ParseTuple(args, "OO:Tuple_", &lib_obj, &args_obj) == 0) {
        return nullptr;
    }

    auto *lib = cast_library(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }

    auto *state = get_symbol_module_state(module);
    if (state == nullptr) {
        return nullptr;
    }

    SymbolSeq syms;
    if (!py_symbol_sequence(args_obj, get_symbol_type(state), syms)) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!clingo_symbol_create_tuple(*lib, syms.data, syms.size, &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo(sym, get_symbol_type(state), false);
}

auto py_function([[maybe_unused]] PyObject *module, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 5>{"lib", "name", "arguments", "is_positive", nullptr};

    PyObject *lib_obj = nullptr;
    const char *name = nullptr;
    Py_ssize_t name_len = 0;
    PyObject *args_obj = nullptr;
    int is_positive = 1;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "Os#|Op:Function", cast_kw(kwlist), &lib_obj, &name, &name_len,
                                    &args_obj, &is_positive) == 0) {
        return nullptr;
    }

    auto *lib = cast_library(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }

    auto *state = get_symbol_module_state(module);
    if (state == nullptr) {
        return nullptr;
    }

    SymbolSeq syms;
    if (args_obj != nullptr && !py_symbol_sequence(args_obj, get_symbol_type(state), syms)) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!clingo_symbol_create_function(*lib, name, static_cast<size_t>(name_len), syms.data, syms.size,
                                       is_positive != 0, &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo(sym, get_symbol_type(state), false);
}

auto py_parse_term([[maybe_unused]] PyObject *module, PyObject *args) -> PyObject * {
    PyObject *lib_obj = nullptr;
    const char *s = nullptr;
    Py_ssize_t n = 0;
    if (PyArg_ParseTuple(args, "Os#:parse_term", &lib_obj, &s, &n) == 0) {
        return nullptr;
    }

    auto *lib = cast_library(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }

    auto *state = get_symbol_module_state(module);
    if (state == nullptr) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!clingo_parse_term(*lib, s, static_cast<size_t>(n), &sym)) {
        return py_handle_error();
    }
    return py_symbol_from_clingo(sym, get_symbol_type(state), false);
}

auto py_symbol_methods = std::array{
    PyMethodDef{
        "match",
        py_c_function_cast(py_symbol_match),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Match functions or tuples against a given signature."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_symbol_getset = std::array{
    PyGetSetDef{
        "signature",
        py_symbol_get_signature,
        nullptr,
        PyDoc_STR("The signature of the symbol."),
        nullptr,
    },
    PyGetSetDef{
        "type",
        py_symbol_get_type,
        nullptr,
        PyDoc_STR("The type of the symbol."),
        nullptr,
    },
    PyGetSetDef{
        "number",
        py_symbol_get_number,
        nullptr,
        PyDoc_STR("The numeric value of the symbol."),
        nullptr,
    },
    PyGetSetDef{
        "string",
        py_symbol_get_string,
        nullptr,
        PyDoc_STR("The string value of the symbol."),
        nullptr,
    },
    PyGetSetDef{
        "name",
        py_symbol_get_name,
        nullptr,
        PyDoc_STR("The name of a function symbol."),
        nullptr,
    },
    PyGetSetDef{
        "arguments",
        py_symbol_get_arguments,
        nullptr,
        PyDoc_STR("The arguments of a function or tuple symbol."),
        nullptr,
    },
    PyGetSetDef{
        "arity",
        py_symbol_get_arity,
        nullptr,
        PyDoc_STR("The arity of a function or tuple symbol."),
        nullptr,
    },
    PyGetSetDef{
        "is_positive",
        py_symbol_get_is_positive,
        nullptr,
        PyDoc_STR("Whether the symbol is positive, e.g., `1` or `p`."),
        nullptr,
    },
    PyGetSetDef{
        "is_negative",
        py_symbol_get_is_negative,
        nullptr,
        PyDoc_STR("Whether the symbol is negative, e.g., `-1` or `-p`."),
        nullptr,
    },
    PyGetSetDef{
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
    },
};

auto py_symbol_slots = std::array{
    PyType_Slot{
        Py_tp_doc,
        PyDoc_STR(const_cast<char *>("Class representing a clingo symbol.")),
    },
    PyType_Slot{
        Py_tp_dealloc,
        reinterpret_cast<void *>(py_symbol_dealloc),
    },
    PyType_Slot{
        Py_tp_str,
        reinterpret_cast<void *>(py_symbol_str),
    },
    PyType_Slot{
        Py_tp_repr,
        reinterpret_cast<void *>(py_symbol_repr),
    },
    PyType_Slot{
        Py_tp_methods,
        py_symbol_methods.data(),
    },
    PyType_Slot{
        Py_tp_getset,
        py_symbol_getset.data(),
    },
    PyType_Slot{
        Py_tp_hash,
        reinterpret_cast<void *>(py_symbol_hash),
    },
    PyType_Slot{
        Py_tp_richcompare,
        reinterpret_cast<void *>(py_symbol_richcompare),
    },
    PyType_Slot{
        0,
        nullptr,
    },
};

auto py_symbol_spec = PyType_Spec{
    "clingo.symbol.Symbol", sizeof(PySymbolObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_symbol_slots.data(),
};

auto symbol_module_methods = std::array{
    PyMethodDef{
        "Number",
        py_number,
        METH_VARARGS,
        PyDoc_STR("Construct a numeric symbol given a number."),
    },
    PyMethodDef{
        "String",
        py_string,
        METH_VARARGS,
        PyDoc_STR("Construct a string symbol given a string."),
    },
    PyMethodDef{
        "Tuple_",
        py_tuple,
        METH_VARARGS,
        PyDoc_STR("Construct a tuple symbol."),
    },
    PyMethodDef{
        "Function",
        py_c_function_cast(py_function),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Construct a function symbol."),
    },
    PyMethodDef{
        "parse_term",
        py_parse_term,
        METH_VARARGS,
        PyDoc_STR("Parse the given string using clingo's term parser for ground terms."),
    },
    PyMethodDef{
        nullptr,
        nullptr,
        0,
        nullptr,
    },
};

auto symbol_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<SymbolModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto &obj : state->vals) {
        Py_VISIT(obj);
    }
    Py_VISIT(state->symbol_type);
    return 0;
}

auto symbol_module_clear(PyObject *module) -> int {
    auto *state = static_cast<SymbolModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto &obj : state->vals) {
        Py_CLEAR(obj);
    }
    Py_CLEAR(state->symbol_type);
    return 0;
}

void symbol_module_free(void *module) {
    symbol_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef symbol_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.symbol",
    PyDoc_STR("Functions and classes for symbol manipulation."),
    static_cast<Py_ssize_t>(sizeof(SymbolModuleState)),
    symbol_module_methods.data(),
    nullptr,
    symbol_module_traverse,
    symbol_module_clear,
    symbol_module_free,
};

} // namespace

void register_symbol(pybind11::module &m) {
    auto symbol = Guard<PyObject *, Py_DECREF>(PyModule_Create(&symbol_def));
    if (symbol == nullptr) {
        throw py::error_already_set();
    }
    auto *state = get_symbol_module_state(symbol);
    if (state == nullptr) {
        throw py::error_already_set();
    }

    auto symbol_type_enum =
        Guard<PyObject *, Py_DECREF>(create_int_enum("clingo.symbol", "SymbolType", symbol_type_members, state->vals));
    if (symbol_type_enum == nullptr || PyModule_AddObjectRef(symbol, "SymbolType", symbol_type_enum) < 0) {
        throw py::error_already_set();
    }

    auto type_obj = Guard<PyObject *, Py_DECREF>(PyType_FromModuleAndSpec(symbol, &py_symbol_spec, nullptr));
    if (type_obj == nullptr || PyModule_AddObjectRef(symbol, "Symbol", type_obj) < 0) {
        throw py::error_already_set();
    }
    state->symbol_type = type_obj;

    auto inf = Guard<PyObject *, Py_DECREF>(
        py_symbol_from_clingo(clingo_symbol_create_infimum(), get_symbol_type(state), false));
    if (inf == nullptr || PyModule_AddObjectRef(symbol, "Infimum", inf) < 0) {
        throw py::error_already_set();
    }
    auto sup = Guard<PyObject *, Py_DECREF>(
        py_symbol_from_clingo(clingo_symbol_create_supremum(), get_symbol_type(state), false));
    if (sup == nullptr || PyModule_AddObjectRef(symbol, "Supremum", sup) < 0) {
        throw py::error_already_set();
    }

    auto *modules = PyImport_GetModuleDict();
    if (modules == nullptr || PyModule_AddObjectRef(m.ptr(), "symbol", symbol) < 0 ||
        PyDict_SetItemString(modules, "clingo.symbol", symbol) < 0) {
        throw py::error_already_set();
    }
}

auto py_symbol_type() -> PyTypeObject * {
    auto mod = Guard<PyObject *, Py_DECREF>(PyImport_ImportModule("clingo.symbol"));
    if (mod == nullptr) {
        return nullptr;
    }
    return get_symbol_type(get_symbol_module_state(mod.get())); // new ref
}

auto py_symbol_from_clingo_object(clingo_symbol_t sym, bool acquire) -> PyObject * {
    auto *type = py_symbol_type();
    if (type == nullptr) {
        return nullptr;
    }
    return py_symbol_from_clingo(sym, type, acquire);
}

auto py_symbol_to_clingo(PyObject *obj, clingo_symbol_t *sym) -> bool {
    if (sym == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "output symbol pointer is null");
        return false;
    }
    auto *type = py_symbol_type();
    if (type == nullptr) {
        return false;
    }
    auto *s = check_symbol(obj, type);
    if (s == nullptr) {
        PyErr_SetString(PyExc_TypeError, "expected clingo.symbol.Symbol");
        return false;
    }
    *sym = s->sym;
    return true;
}

auto py_symbol_to_clingo_check(PyObject *obj, clingo_symbol_t *sym) -> int {
    if (sym == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "output symbol pointer is null");
        return -1;
    }
    auto *type = py_symbol_type();
    if (type == nullptr) {
        return -1;
    }
    auto *s = check_symbol(obj, type);
    if (s == nullptr) {
        return 0;
    }
    *sym = s->sym;
    return 1;
}

} // namespace PyClingo

namespace pybind11::detail {

static auto get_symbol_type() -> PyTypeObject * {
    // we use a static variable here for performance reasons
    // this means the python interpreter can be loaded only once
    static auto *symbol_type = PyClingo::py_symbol_type();
    return symbol_type;
}

auto type_caster<PyClingo::Symbol>::load(handle src, [[maybe_unused]] bool noclue) -> bool {
    if (src.is_none()) {
        return false;
    }
    auto *obj = src.ptr();
    if (!PyObject_TypeCheck(obj, get_symbol_type())) {
        return false;
    }
    auto *s = PyClingo::cast_symbol(obj);
    value = PyClingo::Symbol{s->sym, true};
    return true;
}

auto type_caster<PyClingo::Symbol>::cast(PyClingo::Symbol const &src, [[maybe_unused]] return_value_policy policy,
                                         [[maybe_unused]] handle parent) -> handle {
    PyObject *obj = PyClingo::py_symbol_from_clingo(src.handle(), get_symbol_type(), true);
    if (obj == nullptr) {
        throw error_already_set();
    }
    return obj;
}

} // namespace pybind11::detail
