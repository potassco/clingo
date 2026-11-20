#define PY_SSIZE_T_CLEAN

#include "ast.hh"

#include "control.hh"
#include "core.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/ast.h>

#include <array>
#include <cassert>
#include <cctype>
#include <cstring>
#include <deque>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PyClingo {

namespace {

// The ast module wraps clingo ASTs.
//
// The AST types are created during registration from the description of the
// AST structure (see clingo_ast_info()): records become subtypes of
// `clingo.ast.AST`, unions become `types.UnionType` objects, and enumerations
// become `enum.IntEnum` types. They are added to the submodules of `clingo.ast`
// given by their groups. AST objects own their clingo AST and do not reference
// other Python objects.
//
// Programs and rewrite contexts keep their library alive because their clingo
// objects use it.

//! The number of records, i.e., AST types.
constexpr size_t record_count = clingo_ast_type_statement_comment + 1;

//! The maximum number of arguments of a record.
constexpr size_t max_arguments = 8;

struct ASTModuleState {
    PyObject *lib_type{nullptr};
    PyObject *symbol_type{nullptr};
    PyObject *location_type{nullptr};
    //! The base type of all AST types.
    PyObject *ast_type{nullptr};
    //! The AST types indexed by clingo_ast_type_e.
    PyObject *records{nullptr};
    //! The enumerations indexed as in the AST description.
    PyObject *enums{nullptr};
    PyObject *projection_mode_enum{nullptr};
    PyObject *program_type{nullptr};
    PyObject *rewrite_context_type{nullptr};
};

constexpr auto state_members = std::array{
    &ASTModuleState::lib_type,
    &ASTModuleState::symbol_type,
    &ASTModuleState::location_type,
    &ASTModuleState::ast_type,
    &ASTModuleState::records,
    &ASTModuleState::enums,
    &ASTModuleState::program_type,
    &ASTModuleState::projection_mode_enum,
    &ASTModuleState::rewrite_context_type,
};

struct PyASTObject {
    PyObject_HEAD
    clingo_ast_t *ast;
};

struct PyProgramObject {
    PyObject_HEAD
    clingo_program_t *prg;
    //! The library object, which has to outlive the clingo program.
    PyObject *library;
};

struct PyRewriteContextObject {
    PyObject_HEAD
    clingo_ast_rewrite_context_t *ctx;
    //! The library object, which has to outlive the clingo rewrite context.
    PyObject *library;
};

using ASTGuard = Guard<clingo_ast_t *, clingo_ast_free>;

// casts and module state

auto ast_cast(PyObject *obj) -> PyASTObject * {
    return reinterpret_cast<PyASTObject *>(obj);
}

auto program_cast(PyObject *obj) -> PyProgramObject * {
    return reinterpret_cast<PyProgramObject *>(obj);
}

auto context_cast(PyObject *obj) -> PyRewriteContextObject * {
    return reinterpret_cast<PyRewriteContextObject *>(obj);
}

auto get_state(PyObject *module) -> ASTModuleState * {
    auto *state = static_cast<ASTModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "ast module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_type(PyTypeObject *type) -> ASTModuleState * {
    auto *state = static_cast<ASTModuleState *>(PyType_GetModuleState(type));
    if (state == nullptr) {
        if (PyErr_Occurred() == nullptr) {
            PyErr_SetString(PyExc_RuntimeError, "ast module state is unavailable");
        }
        return nullptr;
    }
    return state;
}

auto state_of(PyObject *self) -> ASTModuleState * {
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

auto info() -> clingo_ast_info_t const & {
    return *clingo_ast_info();
}

//! Convert a snake case name into camel case.
auto camel_case(std::string_view name) -> std::string {
    auto res = std::string{};
    auto upper = true;
    for (auto c : name) {
        if (c == '_') {
            upper = true;
        } else {
            res.push_back(upper ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c);
            upper = false;
        }
    }
    return res;
}

//! Get the first line of a documentation string.
auto summary(char const *doc) -> std::string {
    auto str = std::string_view{doc};
    return std::string{str.substr(0, str.find('\n'))};
}

// static type data
//
// NOTE: The type data has to outlive the types, which keep pointers to names
// and attribute definitions. It is created once and never freed because the
// Python interpreter is initialized only once.

struct RecordData {
    //! The name of the type including its module.
    std::string qualified_name;
    //! The name of the type.
    std::string name;
    std::string doc;
    std::deque<std::string> arg_docs;
    std::vector<PyGetSetDef> getset;
    std::vector<PyType_Slot> slots;
    PyType_Spec spec{};
};

struct GroupData {
    std::string qualified_name;
    std::string doc;
    PyModuleDef def{};
};

struct TypeData {
    std::deque<RecordData> records;
    std::deque<GroupData> groups;
};

auto type_data() -> TypeData & {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    static auto *data = new TypeData{};
    return *data;
}

auto record_name(size_t index) -> char const * {
    return type_data().records[index].name.c_str();
}

// conversion of argument values

//! Get the value of an AST argument as a Python object (new reference).
auto ast_new(ASTModuleState *state, clingo_ast_t *ast) -> PyObject *;

auto get_value(ASTModuleState *state, clingo_ast_t *ast, clingo_ast_argument_info_t const &arg) -> PyObject * {
    switch (arg.type) {
        case clingo_ast_value_type_number:
        case clingo_ast_value_type_bool:
        case clingo_ast_value_type_enum: {
            int value = 0;
            if (!clingo_ast_attribute_get_number(ast, arg.attribute, &value)) {
                return py_handle_error();
            }
            if (arg.type == clingo_ast_value_type_bool) {
                return PyBool_FromLong(value);
            }
            if (arg.type == clingo_ast_value_type_enum) {
                return PyObject_CallFunction(PyTuple_GetItem(state->enums, static_cast<Py_ssize_t>(arg.ref.index)), "i",
                                             value);
            }
            return PyLong_FromLong(value);
        }
        case clingo_ast_value_type_symbol: {
            clingo_symbol_t sym = 0;
            if (!clingo_ast_attribute_get_symbol(ast, arg.attribute, &sym)) {
                return py_handle_error();
            }
            return py_symbol_new(state->symbol_type, sym, true);
        }
        case clingo_ast_value_type_location: {
            clingo_location_t const *loc = nullptr;
            if (!clingo_ast_attribute_get_location(ast, arg.attribute, &loc)) {
                return py_handle_error();
            }
            return py_location_new(state->location_type, loc);
        }
        case clingo_ast_value_type_string: {
            clingo_string_t str;
            if (!clingo_ast_attribute_get_string(ast, arg.attribute, &str)) {
                return py_handle_error();
            }
            return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
        }
        case clingo_ast_value_type_string_array: {
            clingo_string_t const *data = nullptr;
            size_t size = 0;
            if (!clingo_ast_attribute_get_string_array(ast, arg.attribute, &data, &size)) {
                return py_handle_error();
            }
            auto res = steal(PyList_New(static_cast<Py_ssize_t>(size)));
            if (res.get() == nullptr) {
                return nullptr;
            }
            for (size_t i = 0; i < size; ++i) {
                auto const &str = data[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                auto *item = PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
                if (item == nullptr) {
                    return nullptr;
                }
                PyList_SetItem(res.get(), static_cast<Py_ssize_t>(i), item);
            }
            return res.release();
        }
        case clingo_ast_value_type_symbol_array: {
            clingo_symbol_t const *data = nullptr;
            size_t size = 0;
            if (!clingo_ast_attribute_get_symbol_array(ast, arg.attribute, &data, &size)) {
                return py_handle_error();
            }
            auto res = steal(PyList_New(static_cast<Py_ssize_t>(size)));
            if (res.get() == nullptr) {
                return nullptr;
            }
            for (size_t i = 0; i < size; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                auto *item = py_symbol_new(state->symbol_type, data[i], true);
                if (item == nullptr) {
                    return nullptr;
                }
                PyList_SetItem(res.get(), static_cast<Py_ssize_t>(i), item);
            }
            return res.release();
        }
        case clingo_ast_value_type_ast:
        case clingo_ast_value_type_optional_ast: {
            clingo_ast_t *value = nullptr;
            if (!clingo_ast_attribute_get_ast(ast, arg.attribute, &value)) {
                return py_handle_error();
            }
            if (value == nullptr) {
                Py_RETURN_NONE;
            }
            return ast_new(state, value);
        }
        case clingo_ast_value_type_ast_array: {
            struct Array {
                Array() = default;
                Array(Array const &) = delete;
                Array(Array &&) = delete;
                auto operator=(Array const &) -> Array & = delete;
                auto operator=(Array &&) -> Array & = delete;
                ~Array() { clingo_ast_array_free(data, size); }
                clingo_ast_t **data = nullptr;
                size_t size = 0;
            };
            auto arr = Array{};
            if (!clingo_ast_attribute_get_ast_array(ast, arg.attribute, &arr.data, &arr.size)) {
                return py_handle_error();
            }
            auto res = steal(PyList_New(static_cast<Py_ssize_t>(arr.size)));
            if (res.get() == nullptr) {
                return nullptr;
            }
            for (size_t i = 0; i < arr.size; ++i) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                auto *item = ast_new(state, std::exchange(arr.data[i], nullptr));
                if (item == nullptr) {
                    return nullptr;
                }
                PyList_SetItem(res.get(), static_cast<Py_ssize_t>(i), item);
            }
            return res.release();
        }
        default: {
            PyErr_SetString(PyExc_RuntimeError, "unexpected AST value type");
            return nullptr;
        }
    }
}

//! Get the default value of an argument (new reference).
auto default_value(clingo_ast_argument_info_t const &arg) -> PyObject * {
    switch (arg.type) {
        case clingo_ast_value_type_bool: {
            return PyBool_FromLong(arg.default_value);
        }
        case clingo_ast_value_type_optional_ast: {
            Py_RETURN_NONE;
        }
        case clingo_ast_value_type_string_array:
        case clingo_ast_value_type_symbol_array:
        case clingo_ast_value_type_ast_array: {
            return PyTuple_New(0);
        }
        default: {
            return PyLong_FromLong(arg.default_value);
        }
    }
}

//! Buffers to convert Python objects into values to construct an AST.
//!
//! The buffer borrows the converted objects, which have to outlive it.
class ValueBuffer {
  public:
    //! Convert the given object into the i-th value.
    auto set(ASTModuleState *state, char const *fname, size_t i, clingo_ast_argument_info_t const &arg, PyObject *obj)
        -> bool {
        auto &val = values_[i];
        val.type = arg.type;
        switch (arg.type) {
            case clingo_ast_value_type_number:
            case clingo_ast_value_type_enum: {
                return parse_num(obj, &val.number, arg.name);
            }
            case clingo_ast_value_type_bool: {
                auto res = PyObject_IsTrue(obj);
                val.number = res > 0 ? 1 : 0;
                return res >= 0;
            }
            case clingo_ast_value_type_symbol: {
                return py_symbol_get(state->symbol_type, obj, &val.symbol);
            }
            case clingo_ast_value_type_location: {
                val.location = py_location_get(state->location_type, obj);
                return val.location != nullptr;
            }
            case clingo_ast_value_type_string: {
                if (PyUnicode_Check(obj) == 0) {
                    return type_error(fname, arg, "str");
                }
                Py_ssize_t size = 0;
                val.string.data = PyUnicode_AsUTF8AndSize(obj, &size);
                val.string.size = static_cast<size_t>(size);
                return val.string.data != nullptr;
            }
            case clingo_ast_value_type_optional_ast:
            case clingo_ast_value_type_ast: {
                if (arg.type == clingo_ast_value_type_optional_ast && obj == Py_None) {
                    val.ast = nullptr;
                    return true;
                }
                if (PyObject_TypeCheck(obj, reinterpret_cast<PyTypeObject *>(state->ast_type)) == 0) {
                    return type_error(fname, arg, "an AST");
                }
                val.ast = ast_cast(obj)->ast;
                return true;
            }
            case clingo_ast_value_type_string_array:
            case clingo_ast_value_type_symbol_array:
            case clingo_ast_value_type_ast_array: {
                return set_array(state, fname, i, arg, obj);
            }
            default: {
                PyErr_SetString(PyExc_RuntimeError, "unexpected AST value type");
                return false;
            }
        }
    }

    //! Get the values after all of them have been set.
    auto values(size_t size) -> clingo_ast_value_t const * {
        // NOTE: the pointers are set here because the buffers might move while appending
        for (size_t i = 0; i < size; ++i) {
            auto &val = values_[i];
            auto offset = offsets_[i];
            switch (val.type) {
                case clingo_ast_value_type_string_array: {
                    val.string_array.data = strings_.data() + offset;
                    break;
                }
                case clingo_ast_value_type_symbol_array: {
                    val.symbol_array.data = symbols_.data() + offset;
                    break;
                }
                case clingo_ast_value_type_ast_array: {
                    val.ast_array.data = asts_.data() + offset;
                    break;
                }
                default: {
                    break;
                }
            }
        }
        return values_.data();
    }

  private:
    static auto type_error(char const *fname, clingo_ast_argument_info_t const &arg, char const *expected) -> bool {
        PyErr_Format(PyExc_TypeError, "%s() argument '%s' must be %s", fname, arg.name, expected);
        return false;
    }

    template <class T> static auto append(TrivialVector<T> &vec, T const &value) -> bool {
        if (!vec.push_back(value)) {
            PyErr_NoMemory();
            return false;
        }
        return true;
    }

    auto set_array(ASTModuleState *state, char const *fname, size_t i, clingo_ast_argument_info_t const &arg,
                   PyObject *obj) -> bool {
        if (PyUnicode_Check(obj) != 0 || PyBytes_Check(obj) != 0) {
            return type_error(fname, arg, "a sequence");
        }
        keep_[i] = steal(PySequence_Tuple(obj));
        auto *seq = keep_[i].get();
        if (seq == nullptr) {
            if (PyErr_ExceptionMatches(PyExc_TypeError) != 0) {
                PyErr_Clear();
                return type_error(fname, arg, "a sequence");
            }
            return false;
        }
        auto n = PyTuple_Size(seq);
        if (n < 0) {
            return false;
        }
        auto &val = values_[i];
        for (Py_ssize_t j = 0; j < n; ++j) {
            auto *item = PyTuple_GetItem(seq, j);
            switch (arg.type) {
                case clingo_ast_value_type_string_array: {
                    if (PyUnicode_Check(item) == 0) {
                        return type_error(fname, arg, "a sequence of str");
                    }
                    Py_ssize_t size = 0;
                    auto const *data = PyUnicode_AsUTF8AndSize(item, &size);
                    if (data == nullptr || !append(strings_, clingo_string_t{data, static_cast<size_t>(size)})) {
                        return false;
                    }
                    break;
                }
                case clingo_ast_value_type_symbol_array: {
                    clingo_symbol_t sym = 0;
                    if (!py_symbol_get(state->symbol_type, item, &sym) || !append(symbols_, sym)) {
                        return false;
                    }
                    break;
                }
                default: {
                    if (PyObject_TypeCheck(item, reinterpret_cast<PyTypeObject *>(state->ast_type)) == 0) {
                        return type_error(fname, arg, "a sequence of ASTs");
                    }
                    if (!append(asts_, static_cast<clingo_ast_t const *>(ast_cast(item)->ast))) {
                        return false;
                    }
                    break;
                }
            }
        }
        switch (arg.type) {
            case clingo_ast_value_type_string_array: {
                offsets_[i] = strings_.size() - static_cast<size_t>(n);
                val.string_array.size = static_cast<size_t>(n);
                break;
            }
            case clingo_ast_value_type_symbol_array: {
                offsets_[i] = symbols_.size() - static_cast<size_t>(n);
                val.symbol_array.size = static_cast<size_t>(n);
                break;
            }
            default: {
                offsets_[i] = asts_.size() - static_cast<size_t>(n);
                val.ast_array.size = static_cast<size_t>(n);
                break;
            }
        }
        return true;
    }

    std::array<clingo_ast_value_t, max_arguments> values_{};
    std::array<size_t, max_arguments> offsets_{};
    std::array<PyGuard, max_arguments> keep_;
    TrivialVector<clingo_string_t> strings_;
    TrivialVector<clingo_symbol_t> symbols_;
    TrivialVector<clingo_ast_t const *> asts_;
};

//! The Python objects passed as arguments to construct an AST.
struct Arguments {
    Arguments() = default;

    std::array<PyObject *, max_arguments> objs{};
    //! Holds references for objects created during argument processing.
    std::array<PyGuard, max_arguments> owned;

    void own(size_t i, PyObject *obj) {
        owned[i] = steal(obj);
        objs[i] = obj;
    }
};

// AST

auto ast_new(ASTModuleState *state, clingo_ast_t *ast) -> PyObject * {
    auto guard = ASTGuard{ast};
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast, &type)) {
        return py_handle_error();
    }
    if (type < 0 || std::cmp_greater_equal(type, record_count)) {
        PyErr_SetString(PyExc_RuntimeError, "unexpected AST type");
        return nullptr;
    }
    auto *obj = alloc_object(PyTuple_GetItem(state->records, type));
    if (obj == nullptr) {
        return nullptr;
    }
    ast_cast(obj)->ast = guard.release();
    return obj;
}

//! Get the record description of an AST object.
auto ast_record(PyObject *self, size_t *index) -> clingo_ast_record_info_t const * {
    clingo_ast_type_t type = 0;
    if (!clingo_ast_get_type(ast_cast(self)->ast, &type)) {
        return py_handle_error();
    }
    *index = static_cast<size_t>(type);
    return &info().records[type]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

//! Get the values of all arguments of an AST object.
auto ast_values(ASTModuleState *state, PyObject *self, clingo_ast_record_info_t const &record, Arguments &args)
    -> bool {
    for (size_t i = 0; i < record.size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto *value = get_value(state, ast_cast(self)->ast, record.arguments[i]);
        if (value == nullptr) {
            return false;
        }
        args.own(i, value);
    }
    return true;
}

//! Construct an AST object of the given type from Python objects.
auto ast_construct(ASTModuleState *state, size_t index, PyObject *lib_obj, Arguments const &args) -> PyObject * {
    auto const &record = info().records[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto const *fname = record_name(index);
    auto buffer = ValueBuffer{};
    for (size_t i = 0; i < record.size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (!buffer.set(state, fname, i, record.arguments[i], args.objs[i])) {
            return nullptr;
        }
    }
    auto *lib = py_library_get(state->lib_type, lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }
    clingo_ast_t *ast = nullptr;
    if (!clingo_ast_construct_values(lib, static_cast<clingo_ast_type_t>(index), buffer.values(record.size),
                                     record.size, &ast)) {
        return py_handle_error();
    }
    return ast_new(state, ast);
}

//! Find the index of the argument with the given name (record.size if there is none).
auto find_argument(clingo_ast_record_info_t const &record, PyObject *name) -> size_t {
    size_t j = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    while (j < record.size && PyUnicode_CompareWithASCIIString(name, record.arguments[j].name) != 0) {
        ++j;
    }
    return j;
}

//! Parse the arguments to construct an AST: the library followed by the arguments of the record.
auto parse_record_args(size_t index, PyObject *args, PyObject *kwargs, PyObject **lib, Arguments &out) -> bool {
    auto const &record = info().records[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto const *fname = record_name(index);
    auto nargs = PyTuple_Size(args);
    if (nargs < 0) {
        return false;
    }
    if (std::cmp_greater(nargs, record.size + 1)) {
        PyErr_Format(PyExc_TypeError, "%s() takes at most %zu arguments (%zd given)", fname, record.size + 1, nargs);
        return false;
    }
    *lib = nullptr;
    for (Py_ssize_t i = 0; i < nargs; ++i) {
        auto *arg = PyTuple_GetItem(args, i);
        if (i == 0) {
            *lib = arg;
        } else {
            out.objs[i - 1] = arg;
        }
    }
    if (kwargs != nullptr) {
        PyObject *key = nullptr;
        PyObject *value = nullptr;
        Py_ssize_t pos = 0;
        while (PyDict_Next(kwargs, &pos, &key, &value) != 0) {
            if (PyUnicode_CompareWithASCIIString(key, "lib") == 0) {
                if (*lib != nullptr) {
                    PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument 'lib'", fname);
                    return false;
                }
                *lib = value;
                continue;
            }
            auto j = find_argument(record, key);
            if (j == record.size) {
                PyErr_Format(PyExc_TypeError, "%s() got an unexpected keyword argument '%U'", fname, key);
                return false;
            }
            if (out.objs[j] != nullptr) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                PyErr_Format(PyExc_TypeError, "%s() got multiple values for argument '%s'", fname,
                             record.arguments[j].name);
                return false;
            }
            out.objs[j] = value;
        }
    }
    if (*lib == nullptr) {
        PyErr_Format(PyExc_TypeError, "%s() missing required argument 'lib'", fname);
        return false;
    }
    for (size_t j = 0; j < record.size; ++j) {
        if (out.objs[j] != nullptr) {
            continue;
        }
        auto const &arg = record.arguments[j]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (!arg.has_default) {
            PyErr_Format(PyExc_TypeError, "%s() missing required argument '%s'", fname, arg.name);
            return false;
        }
        auto *value = default_value(arg);
        if (value == nullptr) {
            return false;
        }
        out.own(j, value);
    }
    return true;
}

template <size_t Index> auto py_record_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    PyObject *lib = nullptr;
    auto values = Arguments{};
    if (!parse_record_args(Index, args, kwargs, &lib, values)) {
        return nullptr;
    }
    return ast_construct(state, Index, lib, values);
}

template <size_t... Is> constexpr auto make_tp_new(std::index_sequence<Is...> /*unused*/) {
    return std::array<newfunc, sizeof...(Is)>{py_record_tp_new<Is>...};
}

constexpr auto record_tp_new = make_tp_new(std::make_index_sequence<record_count>{});

auto py_ast_getter(PyObject *self, void *closure) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    return get_value(state, ast_cast(self)->ast, *static_cast<clingo_ast_argument_info_t const *>(closure));
}

void py_ast_dealloc(PyObject *self) {
    auto *type = Py_TYPE(self);
    clingo_ast_free(std::exchange(ast_cast(self)->ast, nullptr));
    reinterpret_cast<freefunc>(PyType_GetSlot(type, Py_tp_free))(self);
    Py_DECREF(type);
}

auto py_ast_str(PyObject *self) -> PyObject * {
    auto bld = Guard<clingo_string_builder_t *, clingo_string_builder_free>{nullptr};
    if (!clingo_string_builder_new(&bld.get()) || !clingo_ast_to_string(ast_cast(self)->ast, bld.get())) {
        return py_handle_error();
    }
    clingo_string_t str;
    if (!clingo_string_builder_string(bld.get(), &str)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(str.data, static_cast<Py_ssize_t>(str.size));
}

auto py_ast_repr(PyObject *self) -> PyObject * {
    auto *state = state_of(self);
    size_t index = 0;
    auto const *record = state != nullptr ? ast_record(self, &index) : nullptr;
    if (record == nullptr) {
        return nullptr;
    }
    auto values = Arguments{};
    if (!ast_values(state, self, *record, values)) {
        return nullptr;
    }
    auto parts = steal(PyList_New(0));
    if (parts.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < record->size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto part = steal(PyUnicode_FromFormat("%s=%R", record->arguments[i].name, values.objs[i]));
        if (part.get() == nullptr || PyList_Append(parts.get(), part.get()) < 0) {
            return nullptr;
        }
    }
    auto sep = steal(PyUnicode_FromString(", "));
    auto args = steal(sep.get() != nullptr ? PyUnicode_Join(sep.get(), parts.get()) : nullptr);
    if (args.get() == nullptr) {
        return nullptr;
    }
    return PyUnicode_FromFormat("%s(%U)", record_name(index), args.get());
}

auto py_ast_hash(PyObject *self) -> Py_hash_t {
    return py_hash(clingo_ast_hash(ast_cast(self)->ast));
}

auto py_ast_richcompare(PyObject *a, PyObject *b, int op) -> PyObject * {
    auto *state = state_of(a);
    if (state == nullptr) {
        return nullptr;
    }
    if (PyObject_TypeCheck(b, reinterpret_cast<PyTypeObject *>(state->ast_type)) == 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    auto *x = ast_cast(a)->ast;
    auto *y = ast_cast(b)->ast;
    if (op == Py_EQ || op == Py_NE) {
        return PyBool_FromLong(clingo_ast_equal(x, y) == (op == Py_EQ) ? 1 : 0);
    }
    Py_RETURN_RICHCOMPARE(clingo_ast_compare(x, y), 0, op);
}

//! Call the function with the given object followed by the extra arguments.
auto call_with(PyObject *fun, PyObject *obj, PyObject *extra, PyObject *kwargs) -> PyObject * {
    auto n = PyTuple_Size(extra);
    if (n < 0) {
        return nullptr;
    }
    auto args = steal(PyTuple_New(n + 1));
    if (args.get() == nullptr) {
        return nullptr;
    }
    Py_INCREF(obj);
    PyTuple_SetItem(args.get(), 0, obj);
    for (Py_ssize_t i = 0; i < n; ++i) {
        auto *item = PyTuple_GetItem(extra, i);
        Py_INCREF(item);
        PyTuple_SetItem(args.get(), i + 1, item);
    }
    return PyObject_Call(fun, args.get(), kwargs);
}

//! Whether the argument holds ASTs.
auto has_ast(clingo_ast_argument_info_t const &arg) -> bool {
    return arg.type == clingo_ast_value_type_ast || arg.type == clingo_ast_value_type_optional_ast ||
           arg.type == clingo_ast_value_type_ast_array;
}

//! Split the given arguments into the leading required ones and the extra ones.
auto split_args(char const *fname, PyObject *args, std::span<PyObject *> out, PyGuard &extra) -> bool {
    auto nargs = PyTuple_Size(args);
    if (nargs < 0) {
        return false;
    }
    if (std::cmp_less(nargs, out.size())) {
        PyErr_Format(PyExc_TypeError, "%s() takes at least %zu positional arguments (%zd given)", fname, out.size(),
                     nargs);
        return false;
    }
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = PyTuple_GetItem(args, static_cast<Py_ssize_t>(i));
    }
    extra = steal(PyTuple_GetSlice(args, static_cast<Py_ssize_t>(out.size()), nargs));
    return extra.get() != nullptr;
}

auto py_ast_visit(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto visitor = std::array<PyObject *, 1>{};
    auto extra = PyGuard{nullptr};
    if (!split_args("visit", args, visitor, extra)) {
        return nullptr;
    }
    size_t index = 0;
    auto const *record = ast_record(self, &index);
    if (record == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < record->size; ++i) {
        auto const &arg = record->arguments[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (!has_ast(arg)) {
            continue;
        }
        auto value = steal(get_value(state, ast_cast(self)->ast, arg));
        if (value.get() == nullptr) {
            return nullptr;
        }
        if (arg.type == clingo_ast_value_type_ast_array) {
            auto n = PyList_Size(value.get());
            for (Py_ssize_t j = 0; j < n; ++j) {
                auto res = steal(call_with(visitor[0], PyList_GetItem(value.get(), j), extra.get(), kwargs));
                if (res.get() == nullptr) {
                    return nullptr;
                }
            }
        } else if (value.get() != Py_None) {
            auto res = steal(call_with(visitor[0], value.get(), extra.get(), kwargs));
            if (res.get() == nullptr) {
                return nullptr;
            }
        }
    }
    Py_RETURN_NONE;
}

auto py_ast_transform(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto req = std::array<PyObject *, 2>{};
    auto extra = PyGuard{nullptr};
    if (!split_args("transform", args, req, extra)) {
        return nullptr;
    }
    auto [lib, transformer] = req;
    size_t index = 0;
    auto const *record = ast_record(self, &index);
    if (record == nullptr) {
        return nullptr;
    }
    auto values = Arguments{};
    if (!ast_values(state, self, *record, values)) {
        return nullptr;
    }
    auto changed = false;
    for (size_t i = 0; i < record->size; ++i) {
        auto const &arg = record->arguments[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto *value = values.objs[i];
        if (!has_ast(arg) || value == Py_None) {
            continue;
        }
        if (arg.type == clingo_ast_value_type_ast_array) {
            // NOTE: the list is freshly created and can be modified
            auto n = PyList_Size(value);
            for (Py_ssize_t j = 0; j < n; ++j) {
                auto *res = call_with(transformer, PyList_GetItem(value, j), extra.get(), kwargs);
                if (res == nullptr) {
                    return nullptr;
                }
                if (res == Py_None) {
                    Py_DECREF(res);
                    continue;
                }
                PyList_SetItem(value, j, res);
                changed = true;
            }
        } else {
            auto *res = call_with(transformer, value, extra.get(), kwargs);
            if (res == nullptr) {
                return nullptr;
            }
            if (res == Py_None) {
                Py_DECREF(res);
                continue;
            }
            values.own(i, res);
            changed = true;
        }
    }
    if (!changed) {
        Py_RETURN_NONE;
    }
    return ast_construct(state, index, lib, values);
}

auto py_ast_update(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *state = state_of(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto nargs = PyTuple_Size(args);
    if (nargs != 1) {
        PyErr_Format(PyExc_TypeError, "update() takes exactly one positional argument (%zd given)", nargs);
        return nullptr;
    }
    auto *lib = PyTuple_GetItem(args, 0);
    size_t index = 0;
    auto const *record = ast_record(self, &index);
    if (record == nullptr) {
        return nullptr;
    }
    auto values = Arguments{};
    if (kwargs != nullptr) {
        PyObject *key = nullptr;
        PyObject *value = nullptr;
        Py_ssize_t pos = 0;
        while (PyDict_Next(kwargs, &pos, &key, &value) != 0) {
            auto j = find_argument(*record, key);
            if (j == record->size) {
                PyErr_Format(PyExc_TypeError, "update() got an unexpected keyword argument '%U'", key);
                return nullptr;
            }
            values.objs[j] = value;
        }
    }
    for (size_t i = 0; i < record->size; ++i) {
        if (values.objs[i] != nullptr) {
            continue;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto *value = get_value(state, ast_cast(self)->ast, record->arguments[i]);
        if (value == nullptr) {
            return nullptr;
        }
        values.own(i, value);
    }
    return ast_construct(state, index, lib, values);
}

auto py_ast_methods = std::array{
    PyMethodDef{
        "visit",
        method_cast(py_ast_visit),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Call the visitor with each child node followed by the given arguments."),
    },
    PyMethodDef{
        "transform",
        method_cast(py_ast_transform),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Replace child nodes by the non-None results of the transformer."),
    },
    PyMethodDef{
        "update",
        method_cast(py_ast_update),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Create a copy of the node with the given arguments replaced."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_ast_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("The base class of all AST nodes.")}, // NOLINT
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_ast_dealloc)},
    PyType_Slot{Py_tp_str, reinterpret_cast<void *>(py_ast_str)},
    PyType_Slot{Py_tp_repr, reinterpret_cast<void *>(py_ast_repr)},
    PyType_Slot{Py_tp_hash, reinterpret_cast<void *>(py_ast_hash)},
    PyType_Slot{Py_tp_richcompare, reinterpret_cast<void *>(py_ast_richcompare)},
    PyType_Slot{Py_tp_methods, py_ast_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_ast_spec = PyType_Spec{
    "clingo.ast.AST",
    sizeof(PyASTObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_ast_slots.data(),
};

// Program

auto py_program_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"lib", nullptr};
    PyObject *lib_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:Program", cast_kw(kwlist), &lib_obj) == 0) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    auto *lib = py_library_get(state->lib_type, lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }
    clingo_program_t *prg = nullptr;
    if (!clingo_program_new(lib, &prg)) {
        return py_handle_error();
    }
    auto guard = Guard<clingo_program_t *, clingo_program_free>{prg};
    auto *obj = alloc_object(reinterpret_cast<PyObject *>(type));
    if (obj == nullptr) {
        return nullptr;
    }
    auto *p = program_cast(obj);
    p->prg = guard.release();
    Py_INCREF(lib_obj);
    p->library = lib_obj;
    return obj;
}

auto py_program_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(program_cast(self)->library);
    return 0;
}

auto py_program_clear(PyObject *self) -> int {
    auto *p = program_cast(self);
    // NOTE: the program has to be freed before the library
    if (auto *prg = std::exchange(p->prg, nullptr); prg != nullptr) {
        clingo_program_free(prg);
    }
    Py_CLEAR(p->library);
    return 0;
}

auto program_get(PyObject *self) -> clingo_program_t * {
    auto *prg = program_cast(self)->prg;
    if (prg == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "program is no longer valid");
    }
    return prg;
}

auto py_program_add(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *statement = unset_arg();
    if (!parse_args("add", {"statement"}, {}, args, nargs, kwnames, {&statement})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr || !check_type(state->ast_type, statement, "statement must be an AST")) {
        return nullptr;
    }
    auto *prg = program_get(self);
    if (prg == nullptr) {
        return nullptr;
    }
    if (!clingo_program_add(prg, ast_cast(statement)->ast)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_program_methods = std::array{
    PyMethodDef{
        "add",
        method_cast(py_program_add),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a statement to the program."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_program_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A non-ground program.")}, // NOLINT
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(py_program_tp_new)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_program_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_program_clear)},
    PyType_Slot{Py_tp_methods, py_program_methods.data()},
    PyType_Slot{0, nullptr},
};

auto py_program_spec = PyType_Spec{
    "clingo.ast.Program", sizeof(PyProgramObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, py_program_slots.data(),
};

// RewriteContext

auto py_context_tp_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) -> PyObject * {
    static constexpr auto kwlist = std::array<char const *, 2>{"lib", nullptr};
    PyObject *lib_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O:RewriteContext", cast_kw(kwlist), &lib_obj) == 0) {
        return nullptr;
    }
    auto *state = get_state_from_type(type);
    if (state == nullptr) {
        return nullptr;
    }
    auto *lib = py_library_get(state->lib_type, lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }
    clingo_ast_rewrite_context_t *ctx = nullptr;
    if (!clingo_ast_rewrite_context_create(lib, &ctx)) {
        return py_handle_error();
    }
    auto guard = Guard<clingo_ast_rewrite_context_t *, clingo_ast_rewrite_context_free>{ctx};
    auto *obj = alloc_object(reinterpret_cast<PyObject *>(type));
    if (obj == nullptr) {
        return nullptr;
    }
    auto *c = context_cast(obj);
    c->ctx = guard.release();
    Py_INCREF(lib_obj);
    c->library = lib_obj;
    return obj;
}

auto py_context_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(context_cast(self)->library);
    return 0;
}

auto py_context_clear(PyObject *self) -> int {
    auto *c = context_cast(self);
    // NOTE: the context has to be freed before the library
    if (auto *ctx = std::exchange(c->ctx, nullptr); ctx != nullptr) {
        clingo_ast_rewrite_context_free(ctx);
    }
    Py_CLEAR(c->library);
    return 0;
}

auto context_get(PyObject *self) -> clingo_ast_rewrite_context_t * {
    auto *ctx = context_cast(self)->ctx;
    if (ctx == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "rewrite context is no longer valid");
    }
    return ctx;
}

auto py_context_add_param(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *name = unset_arg();
    if (!parse_args("add_param", {"name"}, {}, args, nargs, kwnames, {&name})) {
        return nullptr;
    }
    if (PyUnicode_Check(name) == 0) {
        PyErr_SetString(PyExc_TypeError, "name must be a str");
        return nullptr;
    }
    Py_ssize_t size = 0;
    auto const *str = PyUnicode_AsUTF8AndSize(name, &size);
    auto *ctx = str != nullptr ? context_get(self) : nullptr;
    if (ctx == nullptr) {
        return nullptr;
    }
    if (!clingo_ast_rewrite_context_add_param(ctx, str, static_cast<size_t>(size))) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_context_clear_params(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *ctx = context_get(self);
    if (ctx == nullptr) {
        return nullptr;
    }
    clingo_ast_rewrite_context_clear_params(ctx);
    Py_RETURN_NONE;
}

auto py_context_add_theory(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *theory = unset_arg();
    if (!parse_args("add_theory", {"theory"}, {}, args, nargs, kwnames, {&theory})) {
        return nullptr;
    }
    auto *state = state_of(self);
    if (state == nullptr || !check_type(state->ast_type, theory, "theory must be an AST")) {
        return nullptr;
    }
    auto *ctx = context_get(self);
    if (ctx == nullptr) {
        return nullptr;
    }
    if (!clingo_ast_rewrite_context_add_theory(ctx, ast_cast(theory)->ast)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_context_get_project_mode(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *state = state_of(self);
    auto *ctx = state != nullptr ? context_get(self) : nullptr;
    if (ctx == nullptr) {
        return nullptr;
    }
    return PyObject_CallFunction(state->projection_mode_enum, "i", clingo_ast_rewrite_context_get_project_mode(ctx));
}

auto py_context_set_project_mode(PyObject *self, PyObject *value, [[maybe_unused]] void *closure) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete project_mode");
        return -1;
    }
    int mode = 0;
    if (!parse_num(value, &mode, "project_mode")) {
        return -1;
    }
    if (mode != clingo_projection_mode_disabled && mode != clingo_projection_mode_anonymous &&
        mode != clingo_projection_mode_pure) {
        PyErr_SetString(PyExc_ValueError, "invalid projection mode");
        return -1;
    }
    auto *ctx = context_get(self);
    if (ctx == nullptr) {
        return -1;
    }
    clingo_ast_rewrite_context_set_project_mode(ctx, mode);
    return 0;
}

auto py_context_get_project_anonymous(PyObject *self, [[maybe_unused]] void *closure) -> PyObject * {
    auto *ctx = context_get(self);
    if (ctx == nullptr) {
        return nullptr;
    }
    return PyBool_FromLong(clingo_ast_rewrite_context_get_project_anonymous(ctx) ? 1 : 0);
}

auto py_context_set_project_anonymous(PyObject *self, PyObject *value, [[maybe_unused]] void *closure) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete project_anonymous");
        return -1;
    }
    auto res = PyObject_IsTrue(value);
    auto *ctx = res >= 0 ? context_get(self) : nullptr;
    if (ctx == nullptr) {
        return -1;
    }
    clingo_ast_rewrite_context_set_project_anonymous(ctx, res != 0);
    return 0;
}

auto py_context_methods = std::array{
    PyMethodDef{
        "add_param",
        method_cast(py_context_add_param),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a parameter, which is protected from simplification."),
    },
    PyMethodDef{"clear_params", py_context_clear_params, METH_NOARGS, PyDoc_STR("Remove all parameters.")},
    PyMethodDef{
        "add_theory",
        method_cast(py_context_add_theory),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Add a theory definition used to rewrite theory atoms."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_context_getset = std::array{
    PyGetSetDef{
        "project_mode",
        py_context_get_project_mode,
        py_context_set_project_mode,
        PyDoc_STR("The active projection mode."),
        nullptr,
    },
    PyGetSetDef{
        "project_anonymous",
        py_context_get_project_anonymous,
        py_context_set_project_anonymous,
        PyDoc_STR("Whether to project anonymous variables in negative literals."),
        nullptr,
    },
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_context_slots = std::array{
    PyType_Slot{Py_tp_doc, const_cast<char *>("A context to rewrite statements.")}, // NOLINT
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(py_context_tp_new)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_context_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_context_clear)},
    PyType_Slot{Py_tp_methods, py_context_methods.data()},
    PyType_Slot{Py_tp_getset, py_context_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_context_spec = PyType_Spec{
    "clingo.ast.RewriteContext", sizeof(PyRewriteContextObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    py_context_slots.data(),
};

// module functions

auto parse_expression(PyObject *module, char const *fname, clingo_ast_parse_type_t type, PyObject *const *args,
                      Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *lib_obj = unset_arg();
    PyObject *str_obj = unset_arg();
    if (!parse_args(fname, {"lib", "string"}, {}, args, nargs, kwnames, {&lib_obj, &str_obj})) {
        return nullptr;
    }
    auto *state = get_state(module);
    if (state == nullptr) {
        return nullptr;
    }
    if (PyUnicode_Check(str_obj) == 0) {
        PyErr_SetString(PyExc_TypeError, "string must be a str");
        return nullptr;
    }
    Py_ssize_t size = 0;
    auto const *str = PyUnicode_AsUTF8AndSize(str_obj, &size);
    auto *lib = str != nullptr ? py_library_get(state->lib_type, lib_obj) : nullptr;
    if (lib == nullptr) {
        return nullptr;
    }
    clingo_ast_t *ast = nullptr;
    if (!clingo_ast_parse_expression(lib, type, str, static_cast<size_t>(size), &ast)) {
        return py_handle_error();
    }
    return ast_new(state, ast);
}

template <clingo_ast_parse_type_e Type> struct ParseName;
template <> struct ParseName<clingo_ast_parse_type_term> {
    static constexpr char const *value = "parse_term";
};
template <> struct ParseName<clingo_ast_parse_type_theory_term> {
    static constexpr char const *value = "parse_theory_term";
};
template <> struct ParseName<clingo_ast_parse_type_literal> {
    static constexpr char const *value = "parse_literal";
};
template <> struct ParseName<clingo_ast_parse_type_head_literal> {
    static constexpr char const *value = "parse_head_literal";
};
template <> struct ParseName<clingo_ast_parse_type_body_literal> {
    static constexpr char const *value = "parse_body_literal";
};
template <> struct ParseName<clingo_ast_parse_type_statement> {
    static constexpr char const *value = "parse_statement";
};

template <clingo_ast_parse_type_e Type>
auto py_parse_expression(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    return parse_expression(module, ParseName<Type>::value, Type, args, nargs, kwnames);
}

struct ParseData {
    ASTModuleState *state;
    PyObject *callback;
};

auto parse_cb(clingo_ast_t *ast, void *data) -> bool {
    auto *d = static_cast<ParseData *>(data);
    clingo_ast_t *copy = nullptr;
    if (!clingo_ast_copy(ast, &copy)) {
        return false;
    }
    auto stm = steal(ast_new(d->state, copy));
    if (stm.get() == nullptr) {
        return store_python_error("creating statement failed");
    }
    auto res = steal(PyObject_CallFunctionObjArgs(d->callback, stm.get(), nullptr));
    if (res.get() == nullptr) {
        return store_python_error("parse callback failed");
    }
    return true;
}

//! Get the optional control argument of the parse functions.
auto parse_control(PyObject *obj, clingo_control_t **ctl) -> bool {
    *ctl = nullptr;
    if (obj == nullptr || obj == Py_None) {
        return true;
    }
    *ctl = py_control_get_any(obj);
    return *ctl != nullptr;
}

auto py_parse_string(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *lib_obj = unset_arg();
    PyObject *program = unset_arg();
    PyObject *callback = unset_arg();
    PyObject *control = nullptr;
    if (!parse_args("parse_string", {"lib", "program", "callback"}, {"control"}, args, nargs, kwnames,
                    {&lib_obj, &program, &callback, &control})) {
        return nullptr;
    }
    auto *state = get_state(module);
    if (state == nullptr) {
        return nullptr;
    }
    if (PyUnicode_Check(program) == 0) {
        PyErr_SetString(PyExc_TypeError, "program must be a str");
        return nullptr;
    }
    if (PyCallable_Check(callback) == 0) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }
    Py_ssize_t size = 0;
    auto const *str = PyUnicode_AsUTF8AndSize(program, &size);
    clingo_control_t *ctl = nullptr;
    auto *lib = str != nullptr && parse_control(control, &ctl) ? py_library_get(state->lib_type, lib_obj) : nullptr;
    if (lib == nullptr) {
        return nullptr;
    }
    auto data = ParseData{state, callback};
    if (!clingo_ast_parse_string(lib, str, static_cast<size_t>(size), ctl, parse_cb, &data)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_parse_files(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *lib_obj = unset_arg();
    PyObject *files_obj = unset_arg();
    PyObject *callback = unset_arg();
    PyObject *control = nullptr;
    if (!parse_args("parse_files", {"lib", "files", "callback"}, {"control"}, args, nargs, kwnames,
                    {&lib_obj, &files_obj, &callback, &control})) {
        return nullptr;
    }
    auto *state = get_state(module);
    if (state == nullptr) {
        return nullptr;
    }
    if (PyCallable_Check(callback) == 0) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }
    auto seq = PyGuard{nullptr};
    auto files = PyBuffer<clingo_string_t>{};
    if (!parse_string_sequence(files_obj, seq, files, "files must be a sequence of strings")) {
        return nullptr;
    }
    clingo_control_t *ctl = nullptr;
    auto *lib = parse_control(control, &ctl) ? py_library_get(state->lib_type, lib_obj) : nullptr;
    if (lib == nullptr) {
        return nullptr;
    }
    auto data = ParseData{state, callback};
    if (!clingo_ast_parse_files(lib, files.data(), files.size(), ctl, parse_cb, &data)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_rewrite_statement(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames) -> PyObject * {
    PyObject *ctx_obj = unset_arg();
    PyObject *statement = unset_arg();
    if (!parse_args("rewrite_statement", {"ctx", "statement"}, {}, args, nargs, kwnames, {&ctx_obj, &statement})) {
        return nullptr;
    }
    auto *state = get_state(module);
    if (state == nullptr || !check_type(state->rewrite_context_type, ctx_obj, "ctx must be a RewriteContext") ||
        !check_type(state->ast_type, statement, "statement must be an AST")) {
        return nullptr;
    }
    auto *ctx = context_get(ctx_obj);
    if (ctx == nullptr) {
        return nullptr;
    }
    struct Array {
        Array() = default;
        Array(Array const &) = delete;
        Array(Array &&) = delete;
        auto operator=(Array const &) -> Array & = delete;
        auto operator=(Array &&) -> Array & = delete;
        ~Array() { clingo_ast_array_free(data, size); }
        clingo_ast_t **data = nullptr;
        size_t size = 0;
    };
    auto arr = Array{};
    if (!clingo_ast_rewrite(ctx, ast_cast(statement)->ast, &arr.data, &arr.size)) {
        return py_handle_error();
    }
    auto res = steal(PyList_New(static_cast<Py_ssize_t>(arr.size)));
    if (res.get() == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < arr.size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto *item = ast_new(state, std::exchange(arr.data[i], nullptr));
        if (item == nullptr) {
            return nullptr;
        }
        PyList_SetItem(res.get(), static_cast<Py_ssize_t>(i), item);
    }
    return res.release();
}

// type information

//! Set a dictionary item stealing the value.
auto set_item(PyObject *dict, char const *key, PyObject *value) -> bool {
    auto val = steal(value);
    return val.get() != nullptr && PyDict_SetItemString(dict, key, val.get()) == 0;
}

auto ref_info(clingo_ast_ref_t ref) -> PyObject * {
    static constexpr auto kinds = std::array{"none", "enum", "record", "union"};
    if (ref.type == clingo_ast_ref_type_none) {
        Py_RETURN_NONE;
    }
    auto const &inf = info();
    char const *name = nullptr;
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    switch (ref.type) {
        case clingo_ast_ref_type_enum: {
            name = inf.enums[ref.index].name;
            break;
        }
        case clingo_ast_ref_type_record: {
            name = inf.records[ref.index].name;
            break;
        }
        default: {
            name = inf.unions[ref.index].name;
            break;
        }
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return Py_BuildValue("(ss)", kinds.at(static_cast<size_t>(ref.type)), name);
}

auto value_type_name(clingo_ast_value_type_t type) -> char const * {
    static constexpr auto names = std::array{
        "number",       "bool",         "enum", "symbol",       "location",  "string",
        "string_array", "symbol_array", "ast",  "optional_ast", "ast_array",
    };
    return type >= 0 && std::cmp_less(type, names.size()) ? names.at(static_cast<size_t>(type)) : "unknown";
}

auto py_type_info([[maybe_unused]] PyObject *module, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto const &inf = info();
    auto res = steal(PyDict_New());
    auto records = steal(PyList_New(0));
    auto unions = steal(PyList_New(0));
    auto enums = steal(PyList_New(0));
    if (res.get() == nullptr || records.get() == nullptr || unions.get() == nullptr || enums.get() == nullptr) {
        return nullptr;
    }
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto const &rec : std::span{inf.records, inf.records_size}) {
        auto arguments = steal(PyList_New(0));
        if (arguments.get() == nullptr) {
            return nullptr;
        }
        for (auto const &arg : std::span{rec.arguments, rec.size}) {
            auto item = steal(PyDict_New());
            if (item.get() == nullptr || !set_item(item.get(), "name", PyUnicode_FromString(arg.name)) ||
                !set_item(item.get(), "type", PyUnicode_FromString(value_type_name(arg.type))) ||
                !set_item(item.get(), "ref", ref_info(arg.ref)) ||
                !set_item(item.get(), "has_default", PyBool_FromLong(arg.has_default ? 1 : 0)) ||
                !set_item(item.get(), "default", PyLong_FromLong(arg.default_value)) ||
                !set_item(item.get(), "doc", PyUnicode_FromString(arg.doc)) ||
                PyList_Append(arguments.get(), item.get()) < 0) {
                return nullptr;
            }
        }
        auto item = steal(PyDict_New());
        if (item.get() == nullptr || !set_item(item.get(), "name", PyUnicode_FromString(rec.name)) ||
            !set_item(item.get(), "group", PyUnicode_FromString(rec.group)) ||
            !set_item(item.get(), "doc", PyUnicode_FromString(rec.doc)) ||
            PyDict_SetItemString(item.get(), "arguments", arguments.get()) < 0 ||
            PyList_Append(records.get(), item.get()) < 0) {
            return nullptr;
        }
    }
    for (auto const &uni : std::span{inf.unions, inf.unions_size}) {
        auto types = steal(PyList_New(0));
        if (types.get() == nullptr) {
            return nullptr;
        }
        for (auto const &ref : std::span{uni.types, uni.size}) {
            auto r = steal(ref_info(ref));
            if (r.get() == nullptr || PyList_Append(types.get(), r.get()) < 0) {
                return nullptr;
            }
        }
        auto item = steal(PyDict_New());
        if (item.get() == nullptr || !set_item(item.get(), "name", PyUnicode_FromString(uni.name)) ||
            !set_item(item.get(), "group", PyUnicode_FromString(uni.group)) ||
            !set_item(item.get(), "doc", PyUnicode_FromString(uni.doc)) ||
            PyDict_SetItemString(item.get(), "types", types.get()) < 0 || PyList_Append(unions.get(), item.get()) < 0) {
            return nullptr;
        }
    }
    for (auto const &enm : std::span{inf.enums, inf.enums_size}) {
        auto values = steal(PyList_New(0));
        if (values.get() == nullptr) {
            return nullptr;
        }
        for (auto const &val : std::span{enm.values, enm.size}) {
            auto v = steal(Py_BuildValue("(sis)", val.name, val.value, val.doc));
            if (v.get() == nullptr || PyList_Append(values.get(), v.get()) < 0) {
                return nullptr;
            }
        }
        auto item = steal(PyDict_New());
        if (item.get() == nullptr || !set_item(item.get(), "name", PyUnicode_FromString(enm.name)) ||
            !set_item(item.get(), "group", PyUnicode_FromString(enm.group)) ||
            !set_item(item.get(), "doc", PyUnicode_FromString(enm.doc)) ||
            PyDict_SetItemString(item.get(), "values", values.get()) < 0 ||
            PyList_Append(enums.get(), item.get()) < 0) {
            return nullptr;
        }
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (PyDict_SetItemString(res.get(), "records", records.get()) < 0 ||
        PyDict_SetItemString(res.get(), "unions", unions.get()) < 0 ||
        PyDict_SetItemString(res.get(), "enums", enums.get()) < 0) {
        return nullptr;
    }
    return res.release();
}

auto ast_module_methods = std::array{
    PyMethodDef{
        "parse_term",
        method_cast(py_parse_expression<clingo_ast_parse_type_term>),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse a term."),
    },
    PyMethodDef{
        "parse_theory_term",
        method_cast(py_parse_expression<clingo_ast_parse_type_theory_term>),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse a theory term."),
    },
    PyMethodDef{
        "parse_literal",
        method_cast(py_parse_expression<clingo_ast_parse_type_literal>),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse a literal."),
    },
    PyMethodDef{
        "parse_head_literal",
        method_cast(py_parse_expression<clingo_ast_parse_type_head_literal>),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse a head literal."),
    },
    PyMethodDef{
        "parse_body_literal",
        method_cast(py_parse_expression<clingo_ast_parse_type_body_literal>),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse a body literal."),
    },
    PyMethodDef{
        "parse_statement",
        method_cast(py_parse_expression<clingo_ast_parse_type_statement>),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse a statement."),
    },
    PyMethodDef{
        "parse_string",
        method_cast(py_parse_string),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse the program in the given string passing statements to the callback."),
    },
    PyMethodDef{
        "parse_files",
        method_cast(py_parse_files),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Parse the programs in the given files passing statements to the callback."),
    },
    PyMethodDef{
        "rewrite_statement",
        method_cast(py_rewrite_statement),
        METH_FASTCALL | METH_KEYWORDS,
        PyDoc_STR("Simplify the given statement returning a list of statements."),
    },
    PyMethodDef{
        "_type_info",
        py_type_info,
        METH_NOARGS,
        PyDoc_STR("Get a description of the AST structure (used to generate stubs)."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

constexpr auto projection_mode_members = std::array<IntEnumMember, 3>{
    IntEnumMember{"Disabled", static_cast<int>(clingo_projection_mode_disabled)},
    IntEnumMember{"Anonymous", static_cast<int>(clingo_projection_mode_anonymous)},
    IntEnumMember{"Pure", static_cast<int>(clingo_projection_mode_pure)},
};

auto ast_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<ASTModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_VISIT(state->*member);
    }
    return 0;
}

auto ast_module_clear(PyObject *module) -> int {
    auto *state = static_cast<ASTModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    for (auto member : state_members) {
        Py_CLEAR(state->*member);
    }
    return 0;
}

void ast_module_free(void *module) {
    ast_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef ast_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.ast",
    PyDoc_STR("Functions and classes to work with abstract syntax trees of logic programs."),
    static_cast<Py_ssize_t>(sizeof(ASTModuleState)),
    ast_module_methods.data(),
    nullptr,
    ast_module_traverse,
    ast_module_clear,
    ast_module_free,
};

// registration of the AST types

constexpr auto group_docs = std::array{
    std::pair{"terms", "AST nodes for terms."},           std::pair{"literals", "AST nodes for literals."},
    std::pair{"aggregates", "AST nodes for aggregates."}, std::pair{"theory", "AST nodes for theory terms and atoms."},
    std::pair{"body", "AST nodes for body literals."},    std::pair{"head", "AST nodes for head literals."},
    std::pair{"statements", "AST nodes for statements."},
};

//! Get (and create if necessary) the submodule of the given group (borrowed reference).
auto group_module(PyObject *ast_mod, PyObject *groups, char const *group) -> PyObject * {
    if (auto *mod = PyDict_GetItemString(groups, group); mod != nullptr) {
        return mod;
    }
    auto &data = type_data().groups.emplace_back();
    data.qualified_name = std::string{"clingo.ast."} + group;
    data.doc = "AST nodes.";
    for (auto const &[name, doc] : group_docs) {
        if (std::strcmp(name, group) == 0) {
            data.doc = doc;
        }
    }
    data.def = PyModuleDef{
        PyModuleDef_HEAD_INIT,
        data.qualified_name.c_str(),
        data.doc.c_str(),
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
    };
    auto mod = PyGuard{nullptr};
    if (!add_submodule(ast_mod, &data.def, &mod.get()) || PyDict_SetItemString(groups, group, mod.get()) < 0) {
        return nullptr;
    }
    return mod.get();
}

auto add_record(ASTModuleState *state, PyObject *ast_mod, PyObject *groups, size_t index) -> bool {
    auto const &record = info().records[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto &data = type_data().records.emplace_back();
    data.name = camel_case(record.name);
    data.qualified_name = std::string{"clingo.ast."} + record.group + "." + data.name;
    data.doc = summary(record.doc);
    for (auto const &arg : std::span{record.arguments, record.size}) {
        auto const &doc = data.arg_docs.emplace_back(summary(arg.doc));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        auto *closure = const_cast<void *>(static_cast<void const *>(&arg));
        data.getset.push_back(PyGetSetDef{arg.name, py_ast_getter, nullptr, doc.c_str(), closure});
    }
    data.getset.push_back(PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr});
    data.slots = {
        PyType_Slot{Py_tp_doc, const_cast<char *>(data.doc.c_str())}, // NOLINT(cppcoreguidelines-pro-type-const-cast)
        PyType_Slot{Py_tp_new, reinterpret_cast<void *>(record_tp_new.at(index))},
        PyType_Slot{Py_tp_getset, data.getset.data()},
        PyType_Slot{0, nullptr},
    };
    data.spec = PyType_Spec{data.qualified_name.c_str(), 0, 0, Py_TPFLAGS_DEFAULT, data.slots.data()};
    // NOTE: the type is created with the ast module, whose state the methods use
    auto *mod = group_module(ast_mod, groups, record.group);
    auto bases = steal(mod != nullptr ? PyTuple_Pack(1, state->ast_type) : nullptr);
    auto type = steal(bases.get() != nullptr ? PyType_FromModuleAndSpec(ast_mod, &data.spec, bases.get()) : nullptr);
    if (type.get() == nullptr || PyModule_AddObjectRef(mod, data.name.c_str(), type.get()) < 0) {
        return false;
    }
    // the argument names enable pattern matching with positional patterns
    auto match_args = steal(PyTuple_New(static_cast<Py_ssize_t>(record.size)));
    if (match_args.get() == nullptr) {
        return false;
    }
    for (size_t i = 0; i < record.size; ++i) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto *name = PyUnicode_FromString(record.arguments[i].name);
        if (name == nullptr) {
            return false;
        }
        PyTuple_SetItem(match_args.get(), static_cast<Py_ssize_t>(i), name);
    }
    if (PyObject_SetAttrString(type.get(), "__match_args__", match_args.get()) < 0) {
        return false;
    }
    PyTuple_SetItem(state->records, static_cast<Py_ssize_t>(index), type.release());
    return true;
}

auto add_enum(ASTModuleState *state, PyObject *ast_mod, PyObject *groups, size_t index) -> bool {
    auto const &enm = info().enums[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto names = std::vector<std::string>{};
    auto members = std::vector<IntEnumMember>{};
    names.reserve(enm.size);
    for (auto const &val : std::span{enm.values, enm.size}) {
        members.emplace_back(names.emplace_back(camel_case(val.name)).c_str(), val.value);
    }
    auto *mod = group_module(ast_mod, groups, enm.group);
    auto type = PyGuard{nullptr};
    if (mod == nullptr || !add_int_enum(mod, camel_case(enm.name).c_str(), members, &type.get(), {})) {
        return false;
    }
    auto doc = steal(PyUnicode_FromString(summary(enm.doc).c_str()));
    if (doc.get() == nullptr || PyObject_SetAttrString(type.get(), "__doc__", doc.get()) < 0) {
        return false;
    }
    PyTuple_SetItem(state->enums, static_cast<Py_ssize_t>(index), type.release());
    return true;
}

//! Get the object for a union (new reference).
auto union_object(ASTModuleState *state, std::vector<PyGuard> &unions, size_t index) -> PyObject *;

auto ref_object(ASTModuleState *state, std::vector<PyGuard> &unions, clingo_ast_ref_t ref) -> PyObject * {
    if (ref.type == clingo_ast_ref_type_record) {
        return borrow(PyTuple_GetItem(state->records, static_cast<Py_ssize_t>(ref.index))).release();
    }
    return union_object(state, unions, ref.index);
}

auto union_object(ASTModuleState *state, std::vector<PyGuard> &unions, size_t index) -> PyObject * {
    if (unions[index].get() == nullptr) {
        auto const &uni = info().unions[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto res = PyGuard{nullptr};
        for (auto const &ref : std::span{uni.types, uni.size}) {
            auto obj = steal(ref_object(state, unions, ref));
            if (obj.get() == nullptr) {
                return nullptr;
            }
            res = res.get() == nullptr ? std::move(obj) : steal(PyNumber_Or(res.get(), obj.get()));
            if (res.get() == nullptr) {
                return nullptr;
            }
        }
        unions[index] = std::move(res);
    }
    return borrow(unions[index].get()).release();
}

auto add_types(ASTModuleState *state, PyObject *ast_mod) -> bool {
    auto const &inf = info();
    if (inf.records_size != record_count) {
        PyErr_SetString(PyExc_RuntimeError, "unexpected number of AST types");
        return false;
    }
    for (auto const &rec : std::span{inf.records, inf.records_size}) {
        if (rec.size > max_arguments) {
            PyErr_SetString(PyExc_RuntimeError, "too many AST arguments");
            return false;
        }
    }
    auto groups = steal(PyDict_New());
    state->records = PyTuple_New(static_cast<Py_ssize_t>(inf.records_size));
    state->enums = PyTuple_New(static_cast<Py_ssize_t>(inf.enums_size));
    if (groups.get() == nullptr || state->records == nullptr || state->enums == nullptr) {
        return false;
    }
    for (size_t i = 0; i < inf.enums_size; ++i) {
        if (!add_enum(state, ast_mod, groups.get(), i)) {
            return false;
        }
    }
    for (size_t i = 0; i < inf.records_size; ++i) {
        if (!add_record(state, ast_mod, groups.get(), i)) {
            return false;
        }
    }
    auto unions = std::vector<PyGuard>(inf.unions_size);
    for (size_t i = 0; i < inf.unions_size; ++i) {
        auto const &uni = inf.unions[i]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto obj = steal(union_object(state, unions, i));
        auto *mod = obj.get() != nullptr ? group_module(ast_mod, groups.get(), uni.group) : nullptr;
        if (mod == nullptr || PyModule_AddObjectRef(mod, camel_case(uni.name).c_str(), obj.get()) < 0) {
            return false;
        }
    }
    return true;
}

auto import_types(ASTModuleState *state) -> bool {
    static constexpr auto types = std::array{
        std::pair{&ASTModuleState::lib_type, &py_library_type},
        std::pair{&ASTModuleState::symbol_type, &py_symbol_type},
        std::pair{&ASTModuleState::location_type, &py_location_type},
    };
    // NOLINTNEXTLINE(readability-use-anyofallof)
    for (auto [member, get] : types) {
        state->*member = get();
        if (state->*member == nullptr) {
            return false;
        }
    }
    return true;
}

auto import_state() -> ASTModuleState * {
    auto mod = steal(PyImport_ImportModule("clingo.ast"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    // NOTE: the state stays alive because the module is kept in sys.modules
    return get_state(mod.get());
}

auto get_type(PyObject *ASTModuleState::*member) -> PyObject * {
    auto *state = import_state();
    if (state == nullptr) {
        return nullptr;
    }
    auto *type = state->*member;
    if (type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.ast module is not initialized");
        return nullptr;
    }
    Py_INCREF(type);
    return type;
}

} // namespace

auto py_ast_type() -> PyObject * {
    return get_type(&ASTModuleState::ast_type);
}

auto py_ast_new(PyObject *type, clingo_ast_t *ast) -> PyObject * {
    auto *state = get_state_from_type(reinterpret_cast<PyTypeObject *>(type));
    if (state == nullptr) {
        clingo_ast_free(ast);
        return nullptr;
    }
    return ast_new(state, ast);
}

auto py_ast_get(PyObject *type, PyObject *obj) -> clingo_ast_t * {
    if (!check_type(type, obj, "expected clingo.ast.AST")) {
        return nullptr;
    }
    return ast_cast(obj)->ast;
}

auto py_ast_program_type() -> PyObject * {
    return get_type(&ASTModuleState::program_type);
}

auto py_ast_program_get(PyObject *type, PyObject *obj) -> clingo_program_t * {
    if (!check_type(type, obj, "expected clingo.ast.Program")) {
        return nullptr;
    }
    return program_get(obj);
}

void register_ast(pybind11::module &m) {
    auto ast_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &ast_def, &ast_mod.get())) {
        throw py::error_already_set();
    }
    auto *state = get_state(ast_mod.get());
    if (state == nullptr || !import_types(state) || !add_type(ast_mod.get(), "AST", &py_ast_spec, &state->ast_type) ||
        !add_types(state, ast_mod.get()) ||
        !add_int_enum(ast_mod.get(), "ProjectionMode", projection_mode_members, &state->projection_mode_enum, {}) ||
        !add_type(ast_mod.get(), "Program", &py_program_spec, &state->program_type) ||
        !add_type(ast_mod.get(), "RewriteContext", &py_context_spec, &state->rewrite_context_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
