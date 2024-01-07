#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "core.hh"

namespace Clingo::Symbol {

using Clingo::Core::Library;

namespace py = pybind11;

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
    [[nodiscard]] auto name() const -> py::str {
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
        return str;
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

    friend auto Number(Clingo::Core::Library &lib, py::int_ num) -> Symbol;
    friend auto Infimum() -> Symbol;
    friend auto Supremum() -> Symbol;
    friend auto Tuple(Library &lib, std::vector<Symbol> const &args) -> Symbol;
    friend auto Function(Library &lib, std::string const &name, std::vector<Symbol> const &args, bool positive)
        -> Symbol;
    friend auto String(Library &lib, std::string const &str) -> Symbol;

  private:
    Symbol(clingo_symbol_t sym) : sym_{sym} {}
    clingo_symbol_t sym_;
};

auto Infimum() -> Symbol { return clingo_symbol_create_infimum(); }

auto Supremum() -> Symbol { return clingo_symbol_create_supremum(); }

auto Number(Library &lib, py::int_ num) -> Symbol {
    // TODO: check if try/catch can be avoided
    try {
        auto val = num.cast<int32_t>();
        return Symbol{clingo_symbol_create_number(val)};
    } catch (py::cast_error const &) {
        auto sym = clingo_symbol_t{0};
        auto str = static_cast<std::string>(py::str(num));
        handle_error(lib, clingo_symbol_create_number_str(lib, str.c_str(), &sym));
        return Symbol{sym};
    }
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

void register_module(pybind11::module &m) {
    auto symbol = m.def_submodule("symbol");

    py::enum_<clingo_symbol_type_e>(symbol, "SymbolType")
        .value("Number", clingo_symbol_type_number)
        .value("Infimum", clingo_symbol_type_infimum)
        .value("Supremum", clingo_symbol_type_supremum)
        .value("String", clingo_symbol_type_string)
        .value("Tuple", clingo_symbol_type_tuple)
        .value("Function", clingo_symbol_type_function);

    py::class_<Symbol>(symbol, "Symbol")
        .def("__str__", &Symbol::str)
        .def_property_readonly("type", &Symbol::type, "the type of the symbol")
        .def_property_readonly("number", &Symbol::number, "the numeric value")
        .def_property_readonly("string", &Symbol::string, "the string value")
        .def_property_readonly("name", &Symbol::name, "the name")
        .def_property_readonly("arguments", &Symbol::args, "the list of arguments")
        .def_property_readonly("sign", &Symbol::sign, "whether the symbol has a sign");

    symbol.add_object("Infimum", py::cast(Infimum()));
    symbol.add_object("Supremum", py::cast(Supremum()));
    symbol.def("Number", &Number, py::arg("lib"), py::arg("num"));
    symbol.def("String", &String, py::arg("lib"), py::arg("str"));
    symbol.def("Tuple_", &Tuple, py::arg("lib"), py::arg("args"));
    symbol.def("Function", &Function, py::arg("lib"), py::arg("name"), py::arg("args") = std::vector<Symbol>{},
               py::arg("sign") = false);
}

} // namespace Clingo::Symbol
