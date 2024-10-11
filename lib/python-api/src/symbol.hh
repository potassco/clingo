#pragma once

#include "core.hh"

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Clingo::Symbol {

namespace py = pybind11;

class Symbol {
  public:
    explicit Symbol(clingo_symbol_t sym, bool acquire) noexcept;
    Symbol(Symbol const &other) noexcept;
    Symbol(Symbol &&other) noexcept;
    auto operator=(Symbol const &other) noexcept -> Symbol &;
    auto operator=(Symbol &&other) noexcept -> Symbol &;
    ~Symbol() noexcept;

    [[nodiscard]] auto type() const -> clingo_symbol_type_e;
    [[nodiscard]] auto number() const -> py::int_;
    [[nodiscard]] auto string() const -> py::str;
    [[nodiscard]] auto name() const -> std::string_view;
    [[nodiscard]] auto arity() const -> size_t;
    [[nodiscard]] auto args() const -> py::list;
    [[nodiscard]] auto str() const -> std::string;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto sign() const -> bool;
    [[nodiscard]] auto match_function(std::string_view name, size_t arity, bool sign) const -> bool;
    [[nodiscard]] auto match_tuple(size_t arity) const -> bool;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto Infimum() -> Symbol;
    friend auto Supremum() -> Symbol;
    friend auto Number(Core::Library &lib, py::int_ num) -> Symbol;
    friend auto String(Core::Library &lib, std::string const &str) -> Symbol;
    friend auto Tuple(Core::Library &lib, std::vector<Symbol> const &args) -> Symbol;
    friend auto Function(Core::Library &lib, std::string const &name, std::vector<Symbol> const &args,
                         bool positive) -> Symbol;
    friend auto parse_term(Core::Library &lib, std::string str) -> Symbol;
    friend auto operator==(Symbol const &a, Symbol const &b) -> bool;
    friend auto operator<=>(Symbol const &a, Symbol const &b) -> std::strong_ordering;

    [[nodiscard]] auto handle() const -> clingo_symbol_t;

  private:
    clingo_symbol_t sym_;
};

void register_module(pybind11::module &m);

} // namespace Clingo::Symbol
