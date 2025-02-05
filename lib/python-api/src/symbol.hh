#pragma once

#include "core.hh"

#include <clingo/symbol.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

namespace py = pybind11;

class Symbol {
  public:
    explicit Symbol() noexcept : sym_{0} {}
    explicit Symbol(clingo_symbol_t sym, bool acquire) noexcept;
    Symbol(Symbol const &other) noexcept;
    Symbol(Symbol &&other) noexcept;
    auto operator=(Symbol const &other) noexcept -> Symbol &;
    auto operator=(Symbol &&other) noexcept -> Symbol &;
    ~Symbol() noexcept;

    [[nodiscard]] auto type() const -> clingo_symbol_type_e;
    [[nodiscard]] auto number() const -> py::int_;
    [[nodiscard]] auto string() const -> py::str;
    [[nodiscard]] auto name() const -> char const *;
    [[nodiscard]] auto arity() const -> size_t;
    [[nodiscard]] auto args() const -> py::list;
    [[nodiscard]] auto str() const -> char const *;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto sign() const -> bool;
    [[nodiscard]] auto match_function(char const *name, size_t arity, bool sign) const -> bool;
    [[nodiscard]] auto match_tuple(size_t arity) const -> bool;
    [[nodiscard]] auto hash() const -> size_t;
    [[nodiscard]] auto signature() const -> std::optional<std::tuple<char const *, size_t, bool>>;

    friend auto Infimum() -> Symbol;
    friend auto Supremum() -> Symbol;
    friend auto Number(Library &lib, py::int_ num) -> Symbol;
    friend auto String(Library &lib, std::string const &str) -> Symbol;
    friend auto Tuple(Library &lib, std::vector<Symbol> const &args) -> Symbol;
    friend auto Function(Library &lib, std::string const &name, std::vector<Symbol> const &args, bool positive)
        -> Symbol;
    friend auto parse_term(Library &lib, std::string str) -> Symbol;
    friend auto operator==(Symbol const &a, Symbol const &b) -> bool;
    friend auto operator<=>(Symbol const &a, Symbol const &b) -> std::strong_ordering;

    [[nodiscard]] auto handle() const -> clingo_symbol_t;

  private:
    clingo_symbol_t sym_;
};

using SymbolVec = std::vector<Symbol>;

void register_symbol(pybind11::module &m);

// NOLINTNEXTLINE
inline auto cpp_cast(clingo_symbol_t const *sym) { return reinterpret_cast<Symbol const *>(sym); }
// NOLINTNEXTLINE
inline auto cpp_cast(clingo_symbol_t *sym) { return reinterpret_cast<Symbol *>(sym); }

// NOLINTNEXTLINE
inline auto c_cast(Symbol const *sym) { return reinterpret_cast<clingo_symbol_t const *>(sym); }
// NOLINTNEXTLINE
inline auto c_cast(Symbol *sym) { return reinterpret_cast<clingo_symbol_t *>(sym); }

} // namespace Clingo::Python
