#pragma once

#include "core.hh"

#include <clingo/symbol.h>

#include <pybind11/pybind11.h>

namespace PyClingo {

namespace py = pybind11;

/*
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

*/

class Symbol {
  public:
    explicit Symbol() noexcept = default;
    explicit Symbol(clingo_symbol_t sym, bool acquire) noexcept : sym_{sym} {
        if (acquire) {
            clingo_symbol_acquire(sym_);
        }
    }
    Symbol(Symbol const &other) noexcept : sym_{other.sym_} { clingo_symbol_acquire(sym_); }
    Symbol(Symbol &&other) noexcept : sym_{std::exchange(other.sym_, 0)} {}

    auto operator=(Symbol const &other) noexcept -> Symbol & {
        if (sym_ != other.sym_) {
            clingo_symbol_acquire(other.sym_);
            clingo_symbol_release(sym_);
            sym_ = other.sym_;
        }
        return *this;
    }
    auto operator=(Symbol &&other) noexcept -> Symbol & {
        if (sym_ != other.sym_) {
            clingo_symbol_release(sym_);
            sym_ = std::exchange(other.sym_, 0);
        }
        return *this;
    }
    ~Symbol() noexcept {
        if (sym_ != 0) {
            clingo_symbol_release(sym_);
        }
    }

    [[nodiscard]] auto type() const -> clingo_symbol_type_e {
        return static_cast<clingo_symbol_type_e>(clingo_symbol_type(sym_));
    }

    [[nodiscard]] auto name() const -> std::string_view {
        auto t = type();
        if (t != clingo_symbol_type_function) {
            throw std::invalid_argument("symbol is not a function");
        }
        clingo_string_t name;
        handle_error(clingo_symbol_name(sym_, &name));
        return {name.data, name.size};
    }

    [[nodiscard]] auto arity() const -> size_t {
        auto t = type();
        if (t != clingo_symbol_type_tuple && t != clingo_symbol_type_function) {
            throw std::invalid_argument("symbol is not a tuple or function");
        }
        clingo_symbol_t const *args = nullptr;
        size_t size = 0;
        handle_error(clingo_symbol_arguments(sym_, &args, &size));
        return size;
    }

    [[nodiscard]] auto is_positive() const -> bool {
        auto t = type();
        if (t != clingo_symbol_type_number && t != clingo_symbol_type_function) {
            throw std::invalid_argument("symbol is not a number or function");
        }
        bool is_positive = false;
        handle_error(clingo_symbol_is_positive(sym_, &is_positive));
        return is_positive;
    }

    [[nodiscard]] auto signature() const -> std::optional<std::tuple<std::string_view, size_t, bool>> {
        return type() == clingo_symbol_type_function
                   ? std::make_optional<std::tuple<std::string_view, size_t, bool>>(name(), arity(), is_positive())
                   : std::nullopt;
    }

    [[nodiscard]] auto str() const -> std::string_view {
        auto *bld = string_builder();
        handle_error(clingo_symbol_to_string(sym_, bld));
        clingo_string_t str;
        handle_error(clingo_string_builder_string(bld, &str));
        return {str.data, str.size};
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

template <> struct type_caster<PyClingo::Symbol> {
  public:
    PYBIND11_TYPE_CASTER(PyClingo::Symbol, _("Symbol"));

    auto load(handle src, bool noclue) -> bool;
    static auto cast(PyClingo::Symbol const &src, return_value_policy policy, handle parent) -> handle;
};

} // namespace pybind11::detail
