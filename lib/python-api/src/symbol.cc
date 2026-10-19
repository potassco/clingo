#include "symbol.hh"
#include "util.hh"

#include <pybind11/native_enum.h>

#include <sstream>
#include <utility>

namespace PyClingo {

static constexpr int decimal_base = 10;

/*
Symbol::Symbol(clingo_symbol_t sym, bool acquire) noexcept : sym_{sym} {
    if (acquire) {
        clingo_symbol_acquire(sym_);
    }
}

Symbol::Symbol(Symbol const &other) noexcept : sym_{other.sym_} {
    clingo_symbol_acquire(sym_);
}

Symbol::Symbol(Symbol &&other) noexcept : sym_{other.sym_} {
    clingo_symbol_acquire(sym_);
}

auto Symbol::operator=(Symbol const &other) noexcept -> Symbol & {
    clingo_symbol_acquire(other.sym_);
    clingo_symbol_release(sym_);
    sym_ = other.sym_;
    return *this;
}

auto Symbol::operator=(Symbol &&other) noexcept -> Symbol & {
    clingo_symbol_acquire(other.sym_);
    clingo_symbol_release(sym_);
    sym_ = other.sym_;
    return *this;
}

Symbol::~Symbol() noexcept {
    clingo_symbol_release(sym_);
}

auto Symbol::type() const -> clingo_symbol_type_e {
    return static_cast<clingo_symbol_type_e>(clingo_symbol_type(sym_));
}

auto Symbol::number() const -> py::int_ {
    if (type() != clingo_symbol_type_number) {
        throw std::invalid_argument("symbol is not a number");
    }
    int32_t num = 0;
    if (clingo_symbol_number(sym_, &num)) {
        return num;
    }
    clingo_clear_error();
    return py::reinterpret_steal<py::int_>(PyLong_FromString(std::string{str()}.c_str(), nullptr, decimal_base));
}

auto Symbol::string() const -> py::str {
    auto t = type();
    if (t != clingo_symbol_type_string) {
        throw std::invalid_argument("symbol is not a string");
    }
    clingo_string_t name;
    handle_error(clingo_symbol_string(sym_, &name));
    return {name.data, name.size};
}

auto Symbol::name() const -> std::string_view {
    auto t = type();
    if (t != clingo_symbol_type_function) {
        throw std::invalid_argument("symbol is not a function");
    }
    clingo_string_t name;
    handle_error(clingo_symbol_name(sym_, &name));
    return {name.data, name.size};
}

auto Symbol::arity() const -> size_t {
    auto t = type();
    if (t != clingo_symbol_type_tuple && t != clingo_symbol_type_function) {
        throw std::invalid_argument("symbol is not a tuple or function");
    }
    clingo_symbol_t const *args = nullptr;
    size_t size = 0;
    handle_error(clingo_symbol_arguments(sym_, &args, &size));
    return size;
}

auto Symbol::args() const -> TypeHint<"Sequence[Symbol]"> {
    auto t = type();
    if (t != clingo_symbol_type_tuple && t != clingo_symbol_type_function) {
        throw std::invalid_argument("symbol is not a tuple or function");
    }
    clingo_symbol_t const *args = nullptr;
    size_t size = 0;
    handle_error(clingo_symbol_arguments(sym_, &args, &size));
    py::list ret;
    for (size_t i = 0; i < size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ret.append(Symbol{args[i], true});
    }
    return ret;
}

auto Symbol::str() const -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_symbol_to_string(sym_, bld));
    clingo_string_t str;
    handle_error(clingo_string_builder_string(bld, &str));
    return {str.data, str.size};
}

auto Symbol::repr() const -> std::string {
    std::ostringstream oss;
    switch (type()) {
        case clingo_symbol_type_infimum: {
            return "Infimum";
        }
        case clingo_symbol_type_supremum: {
            return "Supremum";
        }
        case clingo_symbol_type_number: {
            oss << "Number(" << number() << ")";
            break;
        }
        case clingo_symbol_type_string: {
            // NOTE: clingo representation is compatible with python
            oss << str();
            break;
        }
        case clingo_symbol_type_tuple: {
            oss << "Tuple(" << args() << ")";
            break;
        }
        case clingo_symbol_type_function: {
            oss << "Function(" << name() << ", " << args() << ", " << (is_positive() ? "True" : "False") << ")";
            break;
        }
    }
    return oss.str();
}

auto Symbol::is_positive() const -> bool {
    auto t = type();
    if (t != clingo_symbol_type_number && t != clingo_symbol_type_function) {
        throw std::invalid_argument("symbol is not a number or function");
    }
    bool is_positive = false;
    handle_error(clingo_symbol_is_positive(sym_, &is_positive));
    return is_positive;
}

auto Symbol::is_negative() const -> bool {
    return !is_positive();
}

auto Symbol::match_function(std::string_view name, size_t arity, bool is_positive) const -> bool {

    return type() == clingo_symbol_type_function && this->name() == name && arity == this->arity() &&
           is_positive == this->is_positive();
}

auto Symbol::signature() const -> std::optional<std::tuple<std::string_view, size_t, bool>> {
    return type() == clingo_symbol_type_function
               ? std::make_optional<std::tuple<std::string_view, size_t, bool>>(name(), arity(), is_positive())
               : std::nullopt;
}

auto Symbol::match_tuple(size_t arity) const -> bool {
    return type() == clingo_symbol_type_tuple && arity == this->arity();
}

auto Symbol::hash() const -> size_t {
    return clingo_symbol_hash(sym_);
}

auto Symbol::handle() const -> clingo_symbol_t {
    return sym_;
}

auto operator==(Symbol const &a, Symbol const &b) -> bool {
    return clingo_symbol_equal(a.sym_, b.sym_);
}

auto operator<=>(Symbol const &a, Symbol const &b) -> std::strong_ordering {
    return clingo_symbol_compare(a.sym_, b.sym_) <=> 0;
}

auto Infimum() -> Symbol {
    return Symbol{clingo_symbol_create_infimum(), false};
}

auto Supremum() -> Symbol {
    return Symbol{clingo_symbol_create_supremum(), false};
}

auto Number(Library &lib, py::int_ num) -> Symbol {
    int overflow = 0;
    auto val = PyLong_AsLongAndOverflow(num.ptr(), &overflow);
    if (PyErr_Occurred() != nullptr) {
        throw py::error_already_set();
    }
    if (overflow == 0 && std::numeric_limits<int32_t>::min() <= val && val <= std::numeric_limits<int32_t>::max()) {
        return Symbol{clingo_symbol_create_number(static_cast<int32_t>(val)), false};
    }
    auto sym = clingo_symbol_t{0};
    auto str = static_cast<std::string>(py::str(num));
    handle_error(clingo_symbol_create_number_str(lib, str.c_str(), str.size(), &sym));
    return Symbol{sym, false};
    // try {
    //     auto val = num.cast<int32_t>();
    //     return Symbol{clingo_symbol_create_number(val)};
    // } catch (py::cast_error const &) {
    //     auto sym = clingo_symbol_t{0};
    //     auto str = static_cast<std::string>(py::str(num));
    //     handle_error(clingo_symbol_create_number_str(lib, str.c_str(), &sym));
    //     return Symbol{sym};
    // }
}

auto String(Library &lib, std::string const &str) -> Symbol {
    clingo_symbol_t sym = 0;
    handle_error(clingo_symbol_create_string(lib, str.data(), str.size(), &sym));
    return Symbol{sym, false};
}

auto Tuple(Library &lib, std::span<Symbol> const &args) -> Symbol {
    clingo_symbol_t sym = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const *syms = reinterpret_cast<clingo_symbol_t const *>(args.data());
    handle_error(clingo_symbol_create_tuple(lib, syms, args.size(), &sym));
    return Symbol{sym, false};
}

auto Function(Library &lib, std::string const &name, std::span<Symbol> const &args, bool positive) -> Symbol {
    clingo_symbol_t sym = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto const *syms = reinterpret_cast<clingo_symbol_t const *>(args.data());
    handle_error(clingo_symbol_create_function(lib, name.data(), name.size(), syms, args.size(), positive, &sym));
    return Symbol{sym, false};
}

auto parse_term(Library &lib, std::string str) -> Symbol {
    clingo_symbol_t sym = 0;
    handle_error(clingo_parse_term(lib, str.data(), str.size(), &sym));
    return Symbol{sym, false};
}
*/

namespace {

constexpr size_t symbol_type_count = 6;
constexpr const char *symbol_type_cache_name = "_symbol_type_cache";
constexpr const char *symbol_type_capsule_name = "clingo.symbol._SymbolTypeCache";

template <class T, auto Destroy> class Guard {
  public:
    constexpr explicit Guard(T v = {}) noexcept : value_{v} {}

    Guard(Guard const &other) = delete;
    auto operator=(Guard const &other) -> Guard & = delete;
    constexpr Guard(Guard &&other) noexcept = delete;
    constexpr auto operator=(Guard &&other) noexcept = delete;

    ~Guard() noexcept {
        if (value_ != T{}) {
            Destroy(value_);
        }
    }

    constexpr operator T &() noexcept { return value_; }
    [[nodiscard]] constexpr auto get() noexcept -> T & { return value_; }

    auto release() noexcept -> T {
        auto v = value_;
        value_ = T{};
        return v;
    }

  private:
    T value_{};
};

using PySymbolObject = struct {
    PyObject_HEAD clingo_symbol_t sym;
};

// NOLINTNEXTLINE
PyTypeObject *g_symbol_type = nullptr;

struct SymbolTypeCache {
    std::array<py::object, symbol_type_count> vals;
};

SymbolTypeCache *g_symbol_type_cache = nullptr;

// ---------- helpers ----------

auto throw_error() -> std::nullptr_t {
    clingo_string_t str;
    clingo_result_t rc = clingo_result_success;
    clingo_get_error(&rc, &str);
    PyObject *exc_type = PyExc_RuntimeError;
    switch (rc) {
        case clingo_result_bad_alloc:
            exc_type = PyExc_MemoryError;
            break;
        case clingo_result_invalid:
        case clingo_result_range:
            exc_type = PyExc_ValueError;
            break;
        case clingo_result_logic:
        case clingo_result_runtime:
        default:
            exc_type = PyExc_RuntimeError;
            break;
    }
    // NOLINTBEGIN
    auto *buf = static_cast<char *>(malloc(str.size + 1));
    if (buf != nullptr) {
        memcpy(buf, str.data, str.size);
        buf[str.size] = '\0';
        PyErr_SetString(exc_type, buf);
        free(buf);
    } else {
        PyErr_SetString(PyExc_RuntimeError, "unknown error");
    }
    // NOLINTEND
    return nullptr;
}

auto py_symbol_from_clingo(clingo_symbol_t sym, bool acquire) -> PyObject * {
    auto *obj = PyObject_New(PySymbolObject, g_symbol_type);
    if (obj == nullptr) {
        return nullptr;
    }
    if (acquire) {
        clingo_symbol_acquire(sym);
    }
    obj->sym = sym;
    // NOLINTNEXTLINE
    return reinterpret_cast<PyObject *>(obj);
}

auto symbol_type(clingo_symbol_t sym) -> clingo_symbol_type_e {
    return static_cast<clingo_symbol_type_e>(clingo_symbol_type(sym));
}

auto symbol_args_list(clingo_symbol_t sym) -> PyObject * {
    clingo_symbol_t const *args = nullptr;
    size_t size = 0;
    if (!clingo_symbol_arguments(sym, &args, &size)) {
        return throw_error();
    }
    auto list = Guard<PyObject *, Py_DECREF>(PyList_New(static_cast<Py_ssize_t>(size)));
    if (list == nullptr) {
        return nullptr;
    }
    size_t i = 0;
    for (clingo_symbol_t arg : std::span{args, size}) {
        PyObject *it = py_symbol_from_clingo(arg, true);
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
        return py::cast<Library *>(py::reinterpret_borrow<py::object>(obj));
    } catch (py::cast_error const &) {
        PyErr_SetString(PyExc_TypeError, "lib must be a Library");
        return nullptr;
    }
}

auto cast_symbol(PyObject *self) -> PySymbolObject * {
    // NOLINTNEXTLINE
    return reinterpret_cast<PySymbolObject *>(self);
}

// auto require_symbol(PyObject *obj) -> PySymbolObject * {
//     if (!PyObject_TypeCheck(obj, g_symbol_type)) {
//         PyErr_SetString(PyExc_TypeError, "expected Symbol");
//         return nullptr;
//     }
//     return cast_symbol(obj);
// }

auto check_symbol(PyObject *obj) -> PySymbolObject * {
    if (!PyObject_TypeCheck(obj, g_symbol_type)) {
        return nullptr;
    }
    return cast_symbol(obj);
}

void py_symbol_dealloc(PyObject *self) {
    auto *s = cast_symbol(self);
    clingo_symbol_release(s->sym);
    Py_TYPE(self)->tp_free(self);
}

auto py_symbol_str(PyObject *self) -> PyObject * {
    auto *s = cast_symbol(self);
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get())) {
        return throw_error();
    }
    if (!clingo_symbol_to_string(s->sym, bld)) {
        return throw_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return throw_error();
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
        return throw_error();
    }
    if (!py_symbol_build_repr(bld.get(), s->sym)) {
        return throw_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld, &str)) {
        return throw_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_symbol_hash(PyObject *self) -> Py_hash_t {
    auto *s = cast_symbol(self);
    return static_cast<Py_hash_t>(clingo_symbol_hash(s->sym));
}

auto py_symbol_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    auto *sa = check_symbol(a);
    auto *sb = check_symbol(b);
    if (sa == nullptr || sb == nullptr) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    bool eq = clingo_symbol_equal(sa->sym, sb->sym);
    int cmp = clingo_symbol_compare(sa->sym, sb->sym);
    bool res = false;
    switch (op) {
        case Py_EQ:
            res = eq;
            break;
        case Py_NE:
            res = !eq;
            break;
        case Py_LT:
            res = cmp < 0;
            break;
        case Py_LE:
            res = cmp <= 0;
            break;
        case Py_GT:
            res = cmp > 0;
            break;
        case Py_GE:
            res = cmp >= 0;
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

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "s#|np:match", kwlist.data(), &name, &name_len, &arity,
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
            return throw_error();
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
        if (PyArg_ParseTupleAndKeywords(args, kwargs, "|n:match", kwlist.data(), &arity) == 0) {
            return nullptr;
        }
        auto *s = cast_symbol(self);
        if (symbol_type(s->sym) != clingo_symbol_type_tuple) {
            Py_RETURN_FALSE;
        }
        clingo_symbol_t const *tuple_args = nullptr;
        size_t tuple_arity = 0;
        if (!clingo_symbol_arguments(s->sym, &tuple_args, &tuple_arity)) {
            return throw_error();
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
        return throw_error();
    }
    return Py_BuildValue("(s#nO)", name.data, static_cast<Py_ssize_t>(name.size), static_cast<Py_ssize_t>(arity),
                         pos ? Py_True : Py_False);
}

void cache_destructor(PyObject *capsule) {
    auto *cache = static_cast<SymbolTypeCache *>(PyCapsule_GetPointer(capsule, symbol_type_capsule_name));
    // NOLINTNEXTLINE
    delete cache;
}

auto py_symbol_get_type(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto idx = static_cast<size_t>(symbol_type(cast_symbol(self)->sym));
    assert(g_symbol_type_cache != nullptr && idx < g_symbol_type_cache->vals.size());
    // NOLINTNEXTLINE
    auto *obj = g_symbol_type_cache->vals[idx].ptr();
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
        return throw_error();
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
        return throw_error();
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
    return symbol_args_list(s->sym);
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
        return throw_error();
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
        return throw_error();
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
            return throw_error();
        }
    }

    return py_symbol_from_clingo(sym, false);
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

    clingo_symbol_t sym = 0;
    if (!clingo_symbol_create_string(*lib, s, static_cast<size_t>(n), &sym)) {
        return throw_error();
    }
    return py_symbol_from_clingo(sym, false);
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

auto py_symbol_sequence(PyObject *obj, SymbolSeq &out) -> bool {
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
        auto *s = check_symbol(item);
        if (s == nullptr) {
            PyErr_SetString(PyExc_TypeError, "arguments must contain only Symbol");
            return false;
        }
        // NOLINTNEXTLINE
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

    SymbolSeq syms;
    if (!py_symbol_sequence(args_obj, syms)) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!clingo_symbol_create_tuple(*lib, syms.data, syms.size, &sym)) {
        return throw_error();
    }
    return py_symbol_from_clingo(sym, false);
}

auto py_function([[maybe_unused]] PyObject *module, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 5>{"lib", "name", "arguments", "is_positive", nullptr};

    PyObject *lib_obj = nullptr;
    const char *name = nullptr;
    Py_ssize_t name_len = 0;
    PyObject *args_obj = nullptr;
    int is_positive = 1;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "Os#|Op:Function", kwlist.data(), &lib_obj, &name, &name_len,
                                    &args_obj, &is_positive) == 0) {
        return nullptr;
    }

    auto *lib = cast_library(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }

    SymbolSeq syms;
    if (args_obj != nullptr && !py_symbol_sequence(args_obj, syms)) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!clingo_symbol_create_function(*lib, name, static_cast<size_t>(name_len), syms.data, syms.size,
                                       is_positive != 0, &sym)) {
        return throw_error();
    }
    return py_symbol_from_clingo(sym, false);
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

    clingo_symbol_t sym = 0;
    if (!clingo_parse_term(*lib, s, static_cast<size_t>(n), &sym)) {
        return throw_error();
    }
    return py_symbol_from_clingo(sym, false);
}

// NOLINTNEXTLINE
auto py_symbol_methods = std::array{
    PyMethodDef{
        "match",
        reinterpret_cast<PyCFunction>(py_symbol_match),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Check if this is a function symbol with the given signature,\n"
                  "or a tuple symbol with the given arity."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

// NOLINTNEXTLINE
auto py_symbol_getset = std::array{
    PyGetSetDef{
        "signature",
        py_symbol_get_signature,
        nullptr,
        PyDoc_STR("Get the signature of function symbols."),
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
        PyDoc_STR("The numeric value."),
        nullptr,
    },
    PyGetSetDef{
        "string",
        py_symbol_get_string,
        nullptr,
        PyDoc_STR("The string value."),
        nullptr,
    },
    PyGetSetDef{
        "name",
        py_symbol_get_name,
        nullptr,
        PyDoc_STR("The name."),
        nullptr,
    },
    PyGetSetDef{
        "arguments",
        py_symbol_get_arguments,
        nullptr,
        PyDoc_STR("The list of arguments."),
        nullptr,
    },
    PyGetSetDef{
        "arity",
        py_symbol_get_arity,
        nullptr,
        PyDoc_STR("The arity of a function or tuple."),
        nullptr,
    },
    PyGetSetDef{
        "is_positive",
        py_symbol_get_is_positive,
        nullptr,
        PyDoc_STR("Whether the symbol is positive."),
        nullptr,
    },
    PyGetSetDef{
        "is_negative",
        py_symbol_get_is_negative,
        nullptr,
        PyDoc_STR("Whether the symbol is negative."),
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

// NOLINTNEXTLINE
auto py_symbol_slots = std::array{
    PyType_Slot{
        Py_tp_doc,
        const_cast<char *>(R"doc(Represents a clingo symbol (C-API variant).

This includes `#inf` and `#sup`, numbers, strings, tuples, functions.
)doc"),
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

// NOLINTNEXTLINE
auto py_symbol_spec = PyType_Spec{
    "clingo.symbol.Symbol", sizeof(PySymbolObject), 0, Py_TPFLAGS_DEFAULT, py_symbol_slots.data(),
};

// NOLINTNEXTLINE
auto symbol_module_methods = std::array{
    PyMethodDef{
        "Number",
        py_number,
        METH_VARARGS,
        "Construct a numeric symbol given a number.",
    },
    PyMethodDef{
        "String",
        py_string,
        METH_VARARGS,
        "Construct a string symbol given a string.",
    },
    PyMethodDef{
        "Tuple_",
        py_tuple,
        METH_VARARGS,
        "Construct a tuple symbol.",
    },
    PyMethodDef{
        "Function",
        reinterpret_cast<PyCFunction>(py_function),
        METH_VARARGS | METH_KEYWORDS,
        "Construct a function symbol.",
    },
    PyMethodDef{
        "parse_term",
        py_parse_term,
        METH_VARARGS,
        "Parse the given string using clingo's term parser for ground terms.",
    },
    PyMethodDef{
        nullptr,
        nullptr,
        0,
        nullptr,
    },
};

auto register_symbol2(py::module_ &symbol) -> void {
    auto cache = std::make_unique<SymbolTypeCache>();
    size_t i = 0;
    for (auto &obj : cache->vals) {
        obj = py::cast(static_cast<clingo_symbol_type_e>(i));
        ++i;
    }
    g_symbol_type_cache = cache.get();
    symbol.attr(symbol_type_cache_name) = py::capsule(cache.release(), symbol_type_capsule_name, cache_destructor);

    PyObject *type_obj = PyType_FromModuleAndSpec(symbol.ptr(), &py_symbol_spec, nullptr);
    if (type_obj == nullptr) {
        throw py::error_already_set();
    }

    // NOLINTNEXTLINE
    g_symbol_type = reinterpret_cast<PyTypeObject *>(type_obj);

#if PY_VERSION_HEX >= 0x030A0000
    if (PyModule_AddObjectRef(symbol.ptr(), "Symbol", type_obj) < 0) {
        Py_DECREF(type_obj);
        throw py::error_already_set();
    }
    Py_DECREF(type_obj);
#else
    if (PyModule_AddObject(symbol.ptr(), "Symbol", type_obj) < 0) {
        Py_DECREF(type_obj);
        throw py::error_already_set();
    }
#endif

    PyObject *inf = py_symbol_from_clingo(clingo_symbol_create_infimum(), false);
    if (inf == nullptr) {
        throw py::error_already_set();
    }
#if PY_VERSION_HEX >= 0x030A0000
    if (PyModule_AddObjectRef(symbol.ptr(), "Infimum", inf) < 0) {
        Py_DECREF(inf);
        throw py::error_already_set();
    }
    Py_DECREF(inf);
#else
    if (PyModule_AddObject(symbol.ptr(), "Infimum", inf) < 0) {
        Py_DECREF(inf);
        throw py::error_already_set();
    }
#endif

    PyObject *sup = py_symbol_from_clingo(clingo_symbol_create_supremum(), false);
    if (sup == nullptr) {
        throw py::error_already_set();
    }
#if PY_VERSION_HEX >= 0x030A0000
    if (PyModule_AddObjectRef(symbol.ptr(), "Supremum", sup) < 0) {
        Py_DECREF(sup);
        throw py::error_already_set();
    }
    Py_DECREF(sup);
#else
    if (PyModule_AddObject(symbol.ptr(), "Supremum", sup) < 0) {
        Py_DECREF(sup);
        throw py::error_already_set();
    }
#endif

    if (PyModule_AddFunctions(symbol.ptr(), symbol_module_methods.data()) < 0) {
        throw py::error_already_set();
    }
}

} // namespace

void register_symbol(pybind11::module &m) {
    auto symbol = m.def_submodule("symbol", R"(
Functions and classes for symbol manipulation.

Examples
--------

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function, Number, parse_term
>>>
>>> lib = Library()
>>>
>>> num = Number(lib, 42)
>>> num.number
42
>>> fun = Function(lib, "f", [num])
>>> fun.name
'f'
>>> [ str(arg) for arg in fun.arguments ]
['42']
>>> parse_term(lib, str(fun)) == fun
True
>>> parse_term(lib, 'p(1+2)')
p(3)
```
)"_d);

    py::native_enum<clingo_symbol_type_e>(symbol, "SymbolType", "enum.IntEnum", R"(Enumeration of symbols types.)")
        .value("Number", clingo_symbol_type_number, R"(A numeric symbol, e.g., `1`.)")
        .value("Infimum", clingo_symbol_type_infimum, R"(The `#inf` symbol.)")
        .value("Supremum", clingo_symbol_type_supremum, R"(The `#sup` symbol.)")
        .value("String", clingo_symbol_type_string, R"(A string symbol, e.g., `"a"`.)")
        .value("Tuple", clingo_symbol_type_tuple, R"("A tuple symbol `(1,a)`.")")
        .value("Function", clingo_symbol_type_function, R"(A function symbol, e.g., `c`, `-c`, or `f(1,"a")`.)")
        .finalize();

    /*
    make_comparable(py::class_<Symbol>(symbol, "Symbol", R"(
Represents a clingo symbol.

This includes `#inf` and `#sup`, numbers, strings, tuples, functions (including
constants with `len(arguments) == 0`.

Symbol objects implement Python's rich comparison operators and are ordered
like in clingo. They can also be used as keys in dictionaries. Their string
representation corresponds to their clingo representation.

Note that this class does not have a constructor. Instead there are the
preconstructed symbols `clingo.symbol.Infimum` and `clingo.symbol.Supremum` and
the functions `clingo.symbol.Number`, `clingo.symbol.String`,
`clingo.symbol.Tuple_`, and `clingo.symbol.Function` to construct symbol
objects.
)"_d))
        .def("__str__", &Symbol::str)
        .def("__repr__", &Symbol::repr)
        .def("match", &Symbol::match_function, py::arg("name"), py::arg("arity") = 0, py::arg("is_positive") = true,
             R"(
Check if this is a function symbol with the given signature.

Args:
    name: The name of the function.
    arity: The arity of the function.
    is_positive: Whether to match positive or negative signatures.

Returns:
    Whether the function matches.
)"_d)
        .def("match", &Symbol::match_tuple, py::arg("arity") = 0, R"(
Check if this is a tuple symbol with the given arity.

Args:
    arity: The arity of the function.

Returns:
    Whether the tuple matches.
)"_d)
        .def_property_readonly("signature", &Symbol::signature, R"(
Get the signature of function symbols.

The Boolean in the signature is True if the associated symbol is positive.
)"_d)
        .def_property_readonly("type", &Symbol::type, R"(The type of the symbol.)")
        .def_property_readonly("number", &Symbol::number, R"(The numeric value.)")
        .def_property_readonly("string", &Symbol::string, R"(The string value.)")
        .def_property_readonly("name", &Symbol::name, R"(The name.)")
        .def_property_readonly("arguments", &Symbol::args, R"(The list of arguments.)")
        .def_property_readonly("arity", &Symbol::arity, R"(The arity of a function or tuple.)")
        .def_property_readonly("is_positive", &Symbol::is_positive,
                               R"(Whether the symbol is positive, e.g., `1` or `p`.)")
        .def_property_readonly("is_negative", &Symbol::is_negative,
                               R"(Whether the symbol is negative, e.g, `-1` or `-p`.)");

    symbol.add_object("Infimum", py::cast(Infimum()));
    symbol.add_object("Supremum", py::cast(Supremum()));
    symbol.def("Number", &Number, py::arg("lib"), py::arg("number"), R"(
Construct a numeric symbol given a number.

Args:
    lib: A library object to store the number in.
    number: The given number.
)"_d);
    symbol.def("String", &String, py::arg("lib"), py::arg("string"), R"(
Construct a string symbol given a string.

Args:
    lib: A library object to store the string in.
    string: The given string.
)"_d);
    symbol.def("Tuple_", &Tuple, py::arg("lib"), py::arg("arguments"), R"(
Construct a tuple symbol.

This is a shortcut for `Function(lib, "", arguments)`.

Args:
    lib: A library object to store the tuple in.
    arguments: The arguments in form of a list of symbols.
)"_d);
    symbol.def("Function", &Function, py::arg("lib"), py::arg("name"), py::arg("arguments") = std::span<Symbol>{},
               py::arg("is_positive") = true, R"(
Construct a function symbol.

This includes constants and tuples. Constants have an empty argument list and
tuples have an empty name. Functions can represent classically negated atoms.
Argument `clingo.symbol.Symbol.is_positive` has to be set to `False` to
represent such atoms.

Args:
    lib: A library object to store the function in.
    name: The name of the function.
    arguments: The arguments in the form of a list of symbols.
    is_positive: Whether the function is positive.
)"_d);
    symbol.def("parse_term", &parse_term, py::arg("lib"), py::arg("string"), R"(
Parse the given string using clingo's term parser for ground terms.

The function also evaluates arithmetic functions.

Args:
    lib: A library object to store parsed symbols in.
    string: The string to be parsed.
)"_d);
    */
    register_symbol2(symbol);
}

} // namespace PyClingo

namespace pybind11::detail {

auto type_caster<PyClingo::Symbol>::load(handle src, [[maybe_unused]] bool noclue) -> bool {
    if (src.is_none()) {
        return false;
    }
    auto *obj = src.ptr();
    if (!PyObject_TypeCheck(obj, PyClingo::g_symbol_type)) {
        return false;
    }
    auto *s = PyClingo::cast_symbol(obj);
    value = PyClingo::Symbol{s->sym, true};
    return true;
}

auto type_caster<PyClingo::Symbol>::cast(PyClingo::Symbol const &src, [[maybe_unused]] return_value_policy policy,
                                         [[maybe_unused]] handle parent) -> handle {
    PyObject *obj = PyClingo::py_symbol_from_clingo(src.handle(), true);
    if (obj == nullptr) {
        throw error_already_set();
    }
    return obj;
}

} // namespace pybind11::detail
