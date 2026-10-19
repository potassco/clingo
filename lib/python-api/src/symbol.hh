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

class Symbol2 {
  public:
    explicit Symbol2() noexcept = default;
    explicit Symbol2(clingo_symbol_t sym, bool acquire) noexcept : sym_{sym} {
        if (acquire) {
            clingo_symbol_acquire(sym_);
        }
    }
    Symbol2(Symbol2 const &other) noexcept : sym_{other.sym_} { clingo_symbol_acquire(sym_); }
    Symbol2(Symbol2 &&other) noexcept : sym_{std::exchange(other.sym_, 0)} {}
    auto operator=(Symbol2 const &other) noexcept -> Symbol2 & {
        if (sym_ != other.sym_) {
            clingo_symbol_acquire(other.sym_);
            clingo_symbol_release(sym_);
            sym_ = other.sym_;
        }
        return *this;
    }
    auto operator=(Symbol2 &&other) noexcept -> Symbol2 & {
        if (sym_ != other.sym_) {
            clingo_symbol_release(sym_);
            sym_ = std::exchange(other.sym_, 0);
        }
        return *this;
    }
    ~Symbol2() noexcept {
        if (sym_ != 0) {
            clingo_symbol_release(sym_);
        }
    }

    [[nodiscard]] auto handle() const -> clingo_symbol_t { return sym_; }

  private:
    clingo_symbol_t sym_ = 0;
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

namespace pybind11::detail {

template <> struct type_caster<PyClingo::Symbol2> {
  public:
    PYBIND11_TYPE_CASTER(PyClingo::Symbol2, _("Symbol2"));

    auto load(handle src, bool noclue) -> bool;
    static auto cast(PyClingo::Symbol2 const &src, return_value_policy policy, handle parent) -> handle;
};

} // namespace pybind11::detail
