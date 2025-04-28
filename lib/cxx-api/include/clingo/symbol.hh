#pragma once

#include <clingo/core.hh>

#include <clingo/symbol.h>

#include <span>

namespace Clingo {

// NOLINTNEXTLINE(performance-enum-size)
enum class SymbolType : clingo_symbol_type_t {
    infimum = clingo_symbol_type_infimum,
    number = clingo_symbol_type_number,
    string = clingo_symbol_type_string,
    tuple = clingo_symbol_type_tuple,
    function = clingo_symbol_type_function,
    supremum = clingo_symbol_type_supremum
};

class Symbol;
using SymbolSpan = std::span<Symbol const>;
using SymbolVector = std::vector<Symbol>;

[[nodiscard]] auto cpp_cast(clingo_symbol_t const *sym) -> Symbol const *;

class Symbol {
  public:
    Symbol() = default;
    ~Symbol() { clingo_symbol_release(rep_); }

    Symbol(Symbol const &other) noexcept : rep_{other.rep_} { clingo_symbol_acquire(rep_); }
    auto operator=(Symbol const &other) noexcept -> Symbol & {
        clingo_symbol_acquire(other.rep_);
        clingo_symbol_release(rep_);
        rep_ = other.rep_;
        return *this;
    }

    Symbol(Symbol &&other) noexcept : rep_{std::exchange(other.rep_, clingo_symbol_create_number(0))} {}
    auto operator=(Symbol &&other) noexcept -> Symbol & {
        if (rep_ != other.rep_) {
            clingo_symbol_release(rep_);
            rep_ = std::exchange(other.rep_, clingo_symbol_create_number(0));
        }
        return *this;
    }

    explicit Symbol(clingo_symbol_t rep, bool acquire) : rep_{rep} {
        if (acquire) {
            clingo_symbol_acquire(rep_);
        }
    }
    [[nodiscard]] friend auto c_cast(Symbol const &sym) -> clingo_symbol_t { return sym.rep_; }
    [[nodiscard]] friend auto c_cast(Symbol const *sym) -> clingo_symbol_t const * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<clingo_symbol_t const *>(sym);
    }
    [[nodiscard]] friend auto cpp_cast(clingo_symbol_t const *sym) -> Symbol const * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<Symbol const *>(sym);
    }

    [[nodiscard]] auto number() const -> int {
        int res = 0;
        Detail::handle_error(clingo_symbol_number(rep_, &res));
        return res;
    }
    [[nodiscard]] auto name() const -> char const * {
        char const *res = nullptr;
        Detail::handle_error(clingo_symbol_name(rep_, &res));
        return res;
    }
    [[nodiscard]] auto string() const -> char const * {
        char const *res = nullptr;
        Detail::handle_error(clingo_symbol_string(rep_, &res));
        return res;
    }
    [[nodiscard]] auto is_positive() const -> bool {
        bool res = false;
        Detail::handle_error(clingo_symbol_is_positive(rep_, &res));
        return res;
    }
    [[nodiscard]] auto is_negative() const -> bool { return !is_positive(); }
    [[nodiscard]] auto arguments() const -> SymbolSpan {
        size_t n = 0;
        clingo_symbol_t const *res = nullptr;
        Detail::handle_error(clingo_symbol_arguments(rep_, &res, &n));
        return {cpp_cast(res), n};
    }
    [[nodiscard]] auto type() const -> SymbolType { return static_cast<SymbolType>(clingo_symbol_type(rep_)); }
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        clingo_symbol_to_string(rep_, c_cast(bld));
        return bld.str();
    }
    [[nodiscard]] auto match(char const *name, size_t arity, bool positive = true) const -> bool {
        return type() == SymbolType::function && std::strcmp(this->name(), name) == 0 && arguments().size() == arity &&
               positive == is_positive();
    }
    [[nodiscard]] auto hash() const noexcept -> size_t { return clingo_symbol_hash(rep_); }

    friend auto operator==(Symbol const &a, Symbol const &b) noexcept -> bool {
        return clingo_symbol_equal(a.rep_, b.rep_);
    }
    friend auto operator<=>(Symbol const &a, Symbol const &b) noexcept -> std::strong_ordering {
        return clingo_symbol_compare(a.rep_, b.rep_) <=> 0;
    }
    friend auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream & {
        out << sym.to_string();
        return out;
    }

  private:
    clingo_symbol_t rep_ = clingo_symbol_create_number(0);
};

inline auto Number(int num) -> Symbol {
    return Symbol{clingo_symbol_create_number(num), false};
}

inline auto Number(Library const &lib, char const *str) -> Symbol {
    clingo_symbol_t sym = 0;
    Detail::handle_error(clingo_symbol_create_number_str(c_cast(lib), str, &sym));
    return Symbol{sym, false};
}

inline auto Supremum() -> Symbol {
    return Symbol{clingo_symbol_create_supremum(), false};
}

inline auto Infimum() -> Symbol {
    return Symbol{clingo_symbol_create_infimum(), false};
}

inline auto String(Library const &lib, char const *str) -> Symbol {
    clingo_symbol_t sym = 0;
    Detail::handle_error(clingo_symbol_create_string(c_cast(lib), str, &sym));
    return Symbol{sym, false};
}

inline auto Function(Library const &lib, char const *str, SymbolSpan arguments = {}, bool is_postitve = true)
    -> Symbol {
    clingo_symbol_t sym = 0;
    Detail::handle_error(
        clingo_symbol_create_function(c_cast(lib), str, c_cast(arguments.data()), arguments.size(), is_postitve, &sym));
    return Symbol{sym, false};
}

inline auto Tuple(Library const &lib, SymbolSpan arguments = {}) -> Symbol {
    clingo_symbol_t sym = 0;
    Detail::handle_error(clingo_symbol_create_tuple(c_cast(lib), c_cast(arguments.data()), arguments.size(), &sym));
    return Symbol{sym, false};
}

} // namespace Clingo

namespace std {

template <> struct hash<Clingo::Symbol> {
    auto operator()(Clingo::Symbol const &sym) const -> size_t { return sym.hash(); }
};

} // namespace std
