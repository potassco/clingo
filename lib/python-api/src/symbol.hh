#pragma once

#include <pybind11/pybind11.h>

#include "core.hh"

namespace Clingo::Symbol {

using Clingo::Core::Library;

namespace py = pybind11;

class Symbol {
  public:
    [[nodiscard]] auto type() const -> clingo_symbol_type_e {
        return static_cast<clingo_symbol_type_e>(clingo_symbol_type(sym_));
    }
    friend auto Number(Clingo::Core::Library &lib, py::int_ num) -> Symbol;

  private:
    Symbol(clingo_symbol_t sym) : sym_{sym} {}
    clingo_symbol_t sym_;
};

auto Number(Library &lib, py::int_ num) -> Symbol {
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

void register_module(pybind11::module &m) {
    auto symbol = m.def_submodule("symbol");

    py::enum_<clingo_symbol_type_e>(symbol, "SymbolType")
        .value("Number", clingo_symbol_type_number)
        .value("Infimum", clingo_symbol_type_infimum)
        .value("Supremum", clingo_symbol_type_supremum)
        .value("String", clingo_symbol_type_string)
        .value("Tuple", clingo_symbol_type_tuple)
        .value("Function", clingo_symbol_type_function);

    py::class_<Symbol>(symbol, "Symbol").def("type", &Symbol::type);

    symbol.def("Number", &Number);
}

} // namespace Clingo::Symbol
