#pragma once

#include <sstream>

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core.hh"

namespace Clingo::Symbol {

static constexpr int decimal_base = 10;

class Symbol {
  public:
    [[nodiscard]] auto type() const -> clingo_symbol_type_e {
        return static_cast<clingo_symbol_type_e>(clingo_symbol_type(sym_));
    }
    [[nodiscard]] auto number() const -> py::int_ {
        if (type() != clingo_symbol_type_number) {
            throw std::invalid_argument("symbol is not a number");
        }
        int32_t num = 0;
        if (clingo_symbol_number(sym_, &num)) {
            return num;
        }
        return py::reinterpret_steal<py::int_>(PyLong_FromString(str().c_str(), nullptr, decimal_base));
    }
    [[nodiscard]] auto string() const -> py::str {
        auto t = type();
        if (t != clingo_symbol_type_string) {
            throw std::invalid_argument("symbol is not a string");
        }
        char const *name = nullptr;
        if (!clingo_symbol_string(sym_, &name)) {
            throw std::runtime_error("could not get string value");
        }
        return name;
    }
    [[nodiscard]] auto name() const -> std::string_view {
        auto t = type();
        if (t != clingo_symbol_type_function) {
            throw std::invalid_argument("symbol is not a function");
        }
        char const *name = nullptr;
        if (!clingo_symbol_name(sym_, &name)) {
            throw std::runtime_error("could not get name");
        }
        return name;
    }
    [[nodiscard]] auto arity() const -> size_t {
        auto t = type();
        if (t != clingo_symbol_type_tuple && t != clingo_symbol_type_function) {
            throw std::invalid_argument("symbol is not a tuple or function");
        }
        clingo_symbol_t const *args = nullptr;
        size_t size = 0;
        if (!clingo_symbol_arguments(sym_, &args, &size)) {
            throw std::runtime_error("could not get arguments");
        }
        return size;
    }
    [[nodiscard]] auto args() const -> py::list {
        auto t = type();
        if (t != clingo_symbol_type_tuple && t != clingo_symbol_type_function) {
            throw std::invalid_argument("symbol is not a tuple or function");
        }
        clingo_symbol_t const *args = nullptr;
        size_t size = 0;
        if (!clingo_symbol_arguments(sym_, &args, &size)) {
            throw std::runtime_error("could not get arguments");
        }
        py::list ret;
        for (size_t i = 0; i < size; ++i) {
            ret.append(Symbol{args[i]});
        }
        return ret;
    }
    [[nodiscard]] auto str() const -> std::string {
        size_t len = 0;
        if (!clingo_symbol_to_string_size(sym_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_symbol_to_string(sym_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }
    [[nodiscard]] auto repr() const -> std::string {
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
                // NOTE: gringo representation is compatible with python
                oss << str();
                break;
            }
            case clingo_symbol_type_tuple: {
                oss << "Tuple(" << args() << ")";
                break;
            }
            case clingo_symbol_type_function: {
                oss << "Function(" << name() << ", " << args() << ", " << (sign() ? "True" : "False") << ")";
                break;
            }
        }
        return oss.str();
    }
    [[nodiscard]] auto sign() const -> bool {
        auto t = type();
        if (t != clingo_symbol_type_number && t != clingo_symbol_type_function) {
            throw std::invalid_argument("symbol is not a number or function");
        }
        bool sign = false;
        if (!clingo_symbol_has_sign(sym_, &sign)) {
            throw std::runtime_error("could not get name");
        }
        return sign;
    }
    [[nodiscard]] auto match_function(std::string_view name, size_t arity, bool sign) const -> bool {
        return type() == clingo_symbol_type_function && name == this->name() && arity == this->arity() &&
               sign == this->sign();
    }
    [[nodiscard]] auto match_tuple(size_t arity) const -> bool {
        return type() == clingo_symbol_type_tuple && arity == this->arity();
    }
    [[nodiscard]] auto hash() const -> size_t { return clingo_symbol_hash(sym_); }
    friend auto Infimum() -> Symbol;
    friend auto Supremum() -> Symbol;
    friend auto Number(Library &lib, py::int_ num) -> Symbol;
    friend auto String(Library &lib, std::string const &str) -> Symbol;
    friend auto Tuple(Library &lib, std::vector<Symbol> const &args) -> Symbol;
    friend auto Function(Library &lib, std::string const &name, std::vector<Symbol> const &args, bool positive)
        -> Symbol;
    friend auto parse_term(Library &lib, std::string str) -> Symbol;
    friend auto operator==(Symbol const &a, Symbol const &b) -> bool {
        return clingo_symbol_is_equal_to(a.sym_, b.sym_);
    }
    friend auto operator<(Symbol const &a, Symbol const &b) -> bool {
        return clingo_symbol_is_less_than(a.sym_, b.sym_);
    }
    CLINGO_CPP_TOTAL_ORDER(Symbol)

    static auto acquire(clingo_symbol_t sym) -> Symbol { return Symbol{sym}; }
    [[nodiscard]] auto handle() const -> clingo_symbol_t { return sym_; }

  private:
    Symbol(clingo_symbol_t sym) : sym_{sym} {}
    clingo_symbol_t sym_;
};

auto Infimum() -> Symbol { return clingo_symbol_create_infimum(); }

auto Supremum() -> Symbol { return clingo_symbol_create_supremum(); }

auto Number(Library &lib, py::int_ num) -> Symbol {
    int overflow = 0;
    auto val = PyLong_AsLongAndOverflow(num.ptr(), &overflow);
    if (PyErr_Occurred() != nullptr) {
        throw py::error_already_set();
    }
    if (overflow == 0 && std::numeric_limits<int32_t>::min() <= val && val <= std::numeric_limits<int32_t>::max()) {
        return Symbol{clingo_symbol_create_number(static_cast<int32_t>(val))};
    }
    auto sym = clingo_symbol_t{0};
    auto str = static_cast<std::string>(py::str(num));
    handle_error(lib, clingo_symbol_create_number_str(lib, str.c_str(), &sym));
    return Symbol{sym};
    /*
    try {
        auto val = num.cast<int32_t>();
        return Symbol{clingo_symbol_create_number(val)};
    } catch (py::cast_error const &) {
        auto sym = clingo_symbol_t{0};
        auto str = static_cast<std::string>(py::str(num));
        handle_error(lib, clingo_symbol_create_number_str(lib, str.c_str(), &sym));
        return Symbol{sym};
    }
    */
}

auto String(Library &lib, std::string const &str) -> Symbol {
    clingo_symbol_t sym = 0;
    handle_error(lib, clingo_symbol_create_string(lib, str.data(), &sym));
    return sym;
}

auto Tuple(Library &lib, std::vector<Symbol> const &args) -> Symbol {
    clingo_symbol_t sym = 0;
    handle_error(lib, clingo_symbol_create_tuple(lib, reinterpret_cast<clingo_symbol_t const *>(args.data()),
                                                 args.size(), &sym));
    return sym;
}

auto Function(Library &lib, std::string const &name, std::vector<Symbol> const &args, bool positive) -> Symbol {
    clingo_symbol_t sym = 0;
    handle_error(lib,
                 clingo_symbol_create_function(lib, name.data(), reinterpret_cast<clingo_symbol_t const *>(args.data()),
                                               args.size(), positive, &sym));
    return sym;
}

auto parse_term(Library &lib, std::string str) -> Symbol {
    clingo_symbol_t sym = 0;
    handle_error(lib, clingo_parse_term(lib, str.data(), &sym));
    return Symbol{sym};
}

void register_module(pybind11::module &m) {
    auto symbol = m.def_submodule("symbol", doc(R"(
Functions and classes for symbol manipulation.

Examples
--------

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
)"));

    py::enum_<clingo_symbol_type_e>(symbol, "SymbolType", R"(Enumeration of symbols types.)")
        .value("Number", clingo_symbol_type_number, R"(A numeric symbol, e.g., `1`.)")
        .value("Infimum", clingo_symbol_type_infimum, R"(The `#inf` symbol.)")
        .value("Supremum", clingo_symbol_type_supremum, R"(The `#sup` symbol.)")
        .value("String", clingo_symbol_type_string, R"(A string symbol, e.g., `"a"`.)")
        .value("Tuple", clingo_symbol_type_tuple, R"("A tuple symbol `(1,a)`.")")
        .value("Function", clingo_symbol_type_function, R"(A function symbol, e.g., `c`, `-c`, or `f(1,"a")`.)");

    py::class_<Symbol>(symbol, "Symbol", doc(R"(
Represents a gringo symbol.

This includes `#inf` and `#sup`, numbers, strings, tuples, functions (including
constants with `len(arguments) == 0`.

Symbol objects implement Python's rich comparison operators and are ordered
like in gringo. They can also be used as keys in dictionaries. Their string
representation corresponds to their gringo representation.

Notes
-----
Note that this class does not have a constructor. Instead there are the
preconstructed symbols `Infimum` and `Supremum` and the functions `Number`,
`String`, `Tuple_`, and `Function` to construct symbol objects.
)"))
        //
        CLINGO_PY_TOTAL_ORDER
            //
            .def("__str__", &Symbol::str)
            .def("__repr__", &Symbol::repr)
            .def("__hash__", &Symbol::hash)
            .def("match", &Symbol::match_function, py::arg("name"), py::arg("arity") = 0, py::arg("sign") = false,
                 doc(R"(
Check if this is a function symbol with the given signature.

Parameters
----------
name
    The name of the function.

arity
    The arity of the function.

sign
    Whether to match positive or negative signatures.

Returns
-------
Whether the function matches.
)"))
            .def("match", &Symbol::match_tuple, py::arg("arity") = 0, doc(R"(
Check if this is a tuple symbol with the given arity.

Parameters
----------
arity
    The arity of the function.

Returns
-------
Whether the tuple matches.
)"))
            .def_property_readonly("type", &Symbol::type, doc(R"(
type: clingo.symbol.SymbolType

The type of the symbol.
)"))
            .def_property_readonly("number", &Symbol::number, doc(R"(
number: int

The numeric value.
)"))
            .def_property_readonly("string", &Symbol::string, doc(R"(
string: str

The string value.
)"))
            .def_property_readonly("name", &Symbol::name, doc(R"(
name: str

The name.
)"))
            .def_property_readonly("arguments", &Symbol::args, doc(R"(
arguments: list[clingo.symbol.Symbol]

The list of arguments.
)"))
            .def_property_readonly("arity", &Symbol::arity, doc(R"(
arity: int

The arity of a function or tuple.
)"))
            .def_property_readonly("sign", &Symbol::sign, doc(R"(
sign: bool

Whether the symbol has a sign.
)"));

    symbol.add_object("Infimum", py::cast(Infimum()));
    symbol.add_object("Supremum", py::cast(Supremum()));
    symbol.def("Number", &Number, py::arg("lib"), py::arg("number"), doc(R"(
Construct a numeric symbol given a number.

Parameters
----------
lib
    A library object to store the function in.
number
    The given number.
)"));
    symbol.def("String", &String, py::arg("lib"), py::arg("string"), doc(R"(
Construct a string symbol given a string.

Parameters
----------
lib
    A library object to store the function in.
string
    The given string.
)"));
    symbol.def("Tuple_", &Tuple, py::arg("lib"), py::arg("arguments"), doc(R"(
Construct a tuple symbol.

Parameters
----------
lib
    A library object to store the function in.
arguments
    The arguments in form of a list of symbols.
)"));
    symbol.def("Function", &Function, py::arg("lib"), py::arg("name"), py::arg("arguments") = std::vector<Symbol>{},
               py::arg("sign") = false, doc(R"(
Construct a function symbol.

This includes constants and tuples. Constants have an empty argument list and
tuples have an empty name. Functions can represent classically negated atoms.
Argument positive has to be set to false to represent such atoms.

Parameters
----------
lib
    A library object to store the function in.
name
    The name of the function.
arguments
    The arguments in form of a list of symbols.
sign
    The sign of the function.
)"));
    symbol.def("parse_term", &parse_term, py::arg("lib"), py::arg("string"), doc(R"(
Parse the given string using gringo's term parser for ground terms.

The function also evaluates arithmetic functions.

Parameters
----------
lib
    A library object to store parsed symbols in.
string
    The string to be parsed.
)"));
}

} // namespace Clingo::Symbol
