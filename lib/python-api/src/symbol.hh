#pragma once

#include "core.hh"

#include <clingo/symbol.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

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
    [[nodiscard]] auto name() const -> std::string_view;
    [[nodiscard]] auto arity() const -> size_t;
    [[nodiscard]] auto args() const -> TypeHint<"Sequence[Symbol]">;
    [[nodiscard]] auto str() const -> std::string_view;
    [[nodiscard]] auto repr() const -> std::string;
    [[nodiscard]] auto is_positive() const -> bool;
    [[nodiscard]] auto is_negative() const -> bool;
    [[nodiscard]] auto match_function(std::string_view name, size_t arity, bool is_positive) const -> bool;
    [[nodiscard]] auto match_tuple(size_t arity) const -> bool;
    [[nodiscard]] auto hash() const -> size_t;
    [[nodiscard]] auto signature() const -> std::optional<std::tuple<std::string_view, size_t, bool>>;

    friend auto Infimum() -> Symbol;
    friend auto Supremum() -> Symbol;
    friend auto Number(Library &lib, py::int_ num) -> Symbol;
    friend auto String(Library &lib, std::string const &str) -> Symbol;
    friend auto Tuple(Library &lib, std::span<Symbol> const &args) -> Symbol;
    friend auto Function(Library &lib, std::string const &name, std::span<Symbol> const &args, bool positive) -> Symbol;
    friend auto parse_term(Library &lib, std::string str) -> Symbol;
    friend auto operator==(Symbol const &a, Symbol const &b) -> bool;
    friend auto operator<=>(Symbol const &a, Symbol const &b) -> std::strong_ordering;

    [[nodiscard]] auto handle() const -> clingo_symbol_t;

  private:
    clingo_symbol_t sym_;
};

using SymbolVec = std::vector<Symbol>;
using SymbolSpan = std::span<Symbol const>;

void register_symbol(pybind11::module &m);

inline auto cpp_cast(clingo_symbol_t const *sym) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Symbol const *>(sym);
}
inline auto cpp_cast(clingo_symbol_t *sym) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Symbol *>(sym);
}

inline auto c_cast(Symbol const *sym) {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_symbol_t const *>(sym);
}
inline auto c_cast(Symbol *sym) {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_symbol_t *>(sym);
}

} // namespace PyClingo
