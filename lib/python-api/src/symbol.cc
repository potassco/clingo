#include "symbol.hh"
#include "util.hh"

#include <pybind11/native_enum.h>

#include <sstream>

namespace PyClingo {

static constexpr int decimal_base = 10;

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
    /*
    try {
        auto val = num.cast<int32_t>();
        return Symbol{clingo_symbol_create_number(val)};
    } catch (py::cast_error const &) {
        auto sym = clingo_symbol_t{0};
        auto str = static_cast<std::string>(py::str(num));
        handle_error(clingo_symbol_create_number_str(lib, str.c_str(), &sym));
        return Symbol{sym};
    }
    */
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

    make_comparable(py::class_<Symbol>(symbol, "Symbol", R"(
Represents a clingo symbol.

This includes `#inf` and `#sup`, numbers, strings, tuples, functions (including
constants with `len(arguments) == 0`.

Symbol objects implement Python's rich comparison operators and are ordered
like in clingo. They can also be used as keys in dictionaries. Their string
representation corresponds to their clingo representation.

Note that this class does not have a constructor. Instead there are the
preconstructed symbols `Infimum` and `Supremum` and the functions `Number`,
`String`, `Tuple_`, and `Function` to construct symbol objects.
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
Argument `is_positive` has to be set to `False` to represent such atoms.

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
}

} // namespace PyClingo
