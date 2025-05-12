#pragma once

#include <clingo/core.hh>

#include <clingo/symbol.h>

#include <optional>
#include <span>

namespace Clingo {

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
using SymbolList = std::initializer_list<Symbol const>;
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

    Symbol(Symbol &&other) noexcept : rep_{std::exchange(other.rep_, 0)} {}
    auto operator=(Symbol &&other) noexcept -> Symbol & {
        if (rep_ != other.rep_) {
            clingo_symbol_release(rep_);
            rep_ = std::exchange(other.rep_, 0);
        }
        return *this;
    }

    explicit Symbol(clingo_symbol_t rep, bool acquire) : rep_{rep} {
        if (acquire) {
            clingo_symbol_acquire(rep_);
        }
    }
    [[nodiscard]] friend auto c_cast(Symbol const &sym) -> clingo_symbol_t const & { return sym.rep_; }
    [[nodiscard]] friend auto c_cast(Symbol const *sym) -> clingo_symbol_t const * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<clingo_symbol_t const *>(sym);
    }
    [[nodiscard]] friend auto cpp_cast(clingo_symbol_t const *sym) -> Symbol const * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<Symbol const *>(sym);
    }

    [[nodiscard]] auto number() const -> int { return Detail::call<clingo_symbol_number>(rep_); }
    [[nodiscard]] auto name() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_symbol_name>(rep_);
        return {data, size};
    }
    [[nodiscard]] auto string() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_symbol_string>(rep_);
        return {data, size};
    }
    [[nodiscard]] auto is_positive() const -> bool { return Detail::call<clingo_symbol_is_positive>(rep_); }
    [[nodiscard]] auto is_negative() const -> bool { return !is_positive(); }
    [[nodiscard]] auto arguments() const -> SymbolSpan {
        clingo_symbol_t const *res = nullptr;
        size_t n = 0;
        Detail::handle_error(clingo_symbol_arguments(rep_, &res, &n));
        return {cpp_cast(res), n};
    }
    [[nodiscard]] auto type() const -> SymbolType { return static_cast<SymbolType>(clingo_symbol_type(rep_)); }
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        clingo_symbol_to_string(rep_, c_cast(bld));
        return std::string{bld.str()};
    }
    [[nodiscard]] auto signature() const -> std::optional<std::tuple<std::string_view, size_t, bool>> {
        if (type() == SymbolType::function) {
            return std::make_tuple(name(), arguments().size(), is_positive());
        }
        return std::nullopt;
    }
    [[nodiscard]] auto match(std::string_view name, size_t arity, bool positive = true) const -> bool {
        return type() == SymbolType::function && this->name() == name && arguments().size() == arity &&
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
    clingo_symbol_t rep_ = 0;
};

inline auto Number(int num) -> Symbol {
    return Symbol{clingo_symbol_create_number(num), false};
}

inline auto Number(Library const &lib, std::string_view str) -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_number_str>(c_cast(lib), str.data(), str.size()), false};
}

inline auto Supremum() -> Symbol {
    return Symbol{clingo_symbol_create_supremum(), false};
}

inline auto Infimum() -> Symbol {
    return Symbol{clingo_symbol_create_infimum(), false};
}

inline auto String(Library const &lib, std::string_view str) -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_string>(c_cast(lib), str.data(), str.size()), false};
}

inline auto Function(Library const &lib, std::string_view str, SymbolSpan arguments = {}, bool is_postitve = true)
    -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_function>(c_cast(lib), str.data(), str.size(),
                                                              c_cast(arguments.data()), arguments.size(), is_postitve),
                  false};
}

inline auto Function(Library const &lib, std::string_view str, SymbolList arguments, bool is_postitve = true)
    -> Symbol {
    return Function(lib, str, std::span{arguments.begin(), arguments.end()}, is_postitve);
}

inline auto Tuple(Library const &lib, SymbolSpan arguments = {}) -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_tuple>(c_cast(lib), c_cast(arguments.data()), arguments.size()),
                  false};
}

} // namespace Clingo

namespace std {

template <> struct hash<Clingo::Symbol> {
    auto operator()(Clingo::Symbol const &sym) const -> size_t { return sym.hash(); }
};

} // namespace std
