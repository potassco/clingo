#pragma once

#include <clingo/core.hh>

#include <clingo/symbol.h>

#include <optional>
#include <ostream>
#include <span>

namespace Clingo {

//! @addtogroup cpp_symbol
//! Working with (evaluated) ground terms and related functions.
//!
//! @note Functions to create symbols are only thread-safe if library flags
//! have been requested accordingly.
//! @{

//! Enumeration of symbol types.
enum class SymbolType : clingo_symbol_type_t {
    //! The symbol is a sentinel for the minimum value.
    infimum = clingo_symbol_type_infimum,
    //! The symbol is a number.
    number = clingo_symbol_type_number,
    //! The symbol is a string.
    string = clingo_symbol_type_string,
    //! The symbol is a tuple.
    tuple = clingo_symbol_type_tuple,
    //! The symbol is a function.
    function = clingo_symbol_type_function,
    //! The symbol is a sentinel for the maximum value.
    supremum = clingo_symbol_type_supremum
};

class Symbol;
//! A span of symbols, which is a view on a contiguous sequence of symbols.
using SymbolSpan = std::span<Symbol const>;
//! A list of symbols.
using SymbolList = std::initializer_list<Symbol const>;
//! A vector of symbols.
using SymbolVector = std::vector<Symbol>;

//! Cast a C symbol to its C++ representation.
//!
//! Intended to cast continous ranges of symbols, e.g., from a vector or span.
//!
//! For internal use.
//!
//! @param sym the symbol to cast
//! @return the C++ representation of the symbol
[[nodiscard]] inline auto cpp_cast(clingo_symbol_t const *sym) -> Symbol const * {
    return reinterpret_cast<Symbol const *>(sym); // NOLINT
}

//! Class modeling a symbol in Clingo.
//!
//! Symbols are immutable and can be created from numbers, strings, functions,
//! tuples, or parsed terms.
//!
//! They are totally ordered and support hashing also provding a specialization
//! for `std::hash`.
//!
//! Symbols are reference counted modeling value semantics. They are stored in
//! library objects and should all be released before the library is. Failing
//! to do so won't lead to crashed but prevents release of the underlying
//! symbol store.
class Symbol {
  public:
    //! Default constructor, creates a symbol holding number zero.
    Symbol() = default;
    //! The destructor releases the symbol.
    ~Symbol() { clingo_symbol_release(rep_); }

    //! The copy constructor acquires the symbol.
    //!
    //! @param other the symbol to copy
    Symbol(Symbol const &other) noexcept : rep_{other.rep_} { clingo_symbol_acquire(rep_); }
    //! The copy assignment acquires the symbol and releases the old one.
    //!
    //! @param other the symbol to copy
    //! @return a reference to this symbol
    auto operator=(Symbol const &other) noexcept -> Symbol & {
        clingo_symbol_acquire(other.rep_);
        clingo_symbol_release(rep_);
        rep_ = other.rep_;
        return *this;
    }

    //! The move constructor transfers ownership of the symbol.
    //!
    //! @param other the symbol to move
    Symbol(Symbol &&other) noexcept : rep_{std::exchange(other.rep_, 0)} {}
    //! The move assignment transfers ownership and releases the old one.
    //!
    //! @param other the symbol to move
    //! @return a reference to this symbol
    auto operator=(Symbol &&other) noexcept -> Symbol & {
        if (rep_ != other.rep_) {
            clingo_symbol_release(rep_);
            rep_ = std::exchange(other.rep_, 0);
        }
        return *this;
    }

    //! Construct a symbol from its C API representation.
    //!
    //! For internal use.
    //!
    //! @param rep the C API representation of the symbol
    //! @param acquire whether to acquire the symbol
    explicit Symbol(clingo_symbol_t rep, bool acquire) : rep_{rep} {
        if (acquire) {
            clingo_symbol_acquire(rep_);
        }
    }

    //! Cast the symbol to its C API representation.
    //!
    //! @param sym the symbol to cast
    //! @return the C API representation of the symbol
    [[nodiscard]] friend auto c_cast(Symbol const &sym) -> clingo_symbol_t const & { return sym.rep_; }
    //! Cast the symbol to its C API representation.
    //!
    //! @param sym the symbol to cast
    //! @return the C API representation of the symbol
    [[nodiscard]] friend auto c_cast(Symbol const *sym) -> clingo_symbol_t const * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<clingo_symbol_t const *>(sym);
    }

    //! Get the numeric value of the symbol if it is a number.
    //!
    //! @return the numeric value of the symbol
    [[nodiscard]] auto number() const -> int { return Detail::call<clingo_symbol_number>(rep_); }

    //! Get the name of the symbol if it is a function.
    //!
    //! @return the name of the symbol
    [[nodiscard]] auto name() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_symbol_name>(rep_);
        return {data, size};
    }

    //! Get the string value of the symbol if it is a string.
    //!
    //! @return the string value of the symbol
    [[nodiscard]] auto string() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_symbol_string>(rep_);
        return {data, size};
    }

    //! Check whether a function or number symbol is positive.
    //!
    //! @return whether the symbol is positive
    [[nodiscard]] auto is_positive() const -> bool { return Detail::call<clingo_symbol_is_positive>(rep_); }

    //! Check whether a function or number symbol is negative.
    //!
    //! @return whether the symbol is negative
    [[nodiscard]] auto is_negative() const -> bool { return !is_positive(); }

    //! Get the arguments of a function or tuple symbol.
    //!
    //! @return a span of symbols representing the arguments
    [[nodiscard]] auto arguments() const -> SymbolSpan {
        clingo_symbol_t const *res = nullptr;
        size_t n = 0;
        Detail::handle_error(clingo_symbol_arguments(rep_, &res, &n));
        return {cpp_cast(res), n};
    }

    //! Get the type of the symbol.
    //!
    //! @return the type of the symbol
    [[nodiscard]] auto type() const -> SymbolType { return static_cast<SymbolType>(clingo_symbol_type(rep_)); }

    //! Get a string representation of the symbol.
    //!
    //! The string representation adheres to clingo's input language and is
    //! parsable by clingo.
    //!
    //! @return a string representation of the symbol
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        clingo_symbol_to_string(rep_, c_cast(bld));
        return std::string{bld.str()};
    }

    //! Get the signature of the symbol if it is a function.
    //!
    //! The result is empty if the symbol is not a function.
    //!
    //! @return an optional name arity positivity tuple
    [[nodiscard]] auto signature() const -> std::optional<std::tuple<std::string_view, size_t, bool>> {
        if (type() == SymbolType::function) {
            return std::make_tuple(name(), arguments().size(), is_positive());
        }
        return std::nullopt;
    }

    //! Match the symbol against a function signature.
    //!
    //! @param name the name of the function
    //! @param arity the number of arguments of the function
    //! @param positive whether the function is positive
    //! @return whether the symbol matches the function signature
    [[nodiscard]] auto match(std::string_view name, size_t arity, bool positive = true) const -> bool {
        return type() == SymbolType::function && this->name() == name && arguments().size() == arity &&
               positive == is_positive();
    }

    //! Match the symbol against a tuple signature.
    //!
    //! @param arity the number of arguments of the tuple
    //! @return whether the symbol matches the tuple signature
    [[nodiscard]] auto match(size_t arity) const -> bool {
        return type() == SymbolType::tuple && arguments().size() == arity;
    }

    //! Compute a hash value for the symbol.
    //!
    //! The hash value is entended for usage in hash tables.
    //!
    //! @return a hash value for the symbol
    [[nodiscard]] auto hash() const noexcept -> size_t { return clingo_symbol_hash(rep_); }

    //! Compare two symbols for equality.
    //!
    //! @param a the first symbol to compare
    //! @param b the second symbol to compare
    //! @return whether the symbols are equal
    friend auto operator==(Symbol const &a, Symbol const &b) noexcept -> bool {
        return clingo_symbol_equal(a.rep_, b.rep_);
    }

    //! Compare two symbols.
    //!
    //! @param a the first symbol to compare
    //! @param b the second symbol to compare
    //! @return the result of the comparison
    friend auto operator<=>(Symbol const &a, Symbol const &b) noexcept -> std::strong_ordering {
        return clingo_symbol_compare(a.rep_, b.rep_) <=> 0;
    }

    //! Output the symbol to a stream.
    //!
    //! @param out the output stream
    //! @param sym the symbol to output
    //! @return the output stream
    friend auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream & {
        out << sym.to_string();
        return out;
    }

  private:
    clingo_symbol_t rep_ = 0;
};

//! Construct a numeric symbol from an integer.
//!
//! @param num the integer to convert
//! @return a symbol representing the number
inline auto Number(int num) -> Symbol {
    return Symbol{clingo_symbol_create_number(num), false};
}

//! Construct a numeric symbol from a string.
//!
//! @param lib the library for storing symbols
//! @param str the string to convert
//! @return a symbol representing the number
inline auto Number(Library const &lib, std::string_view str) -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_number_str>(c_cast(lib), str.data(), str.size()), false};
}

//! Construct a supremum symbol.
//!
//! The supremum symbol is a sentinel for the maximum value.
//!
//! @return a supremum symbol
inline auto Supremum() -> Symbol {
    return Symbol{clingo_symbol_create_supremum(), false};
}

//! Construct an infimum symbol.
//!
//! The infimum symbol is a sentinel for the minimum value.
//!
//! @return an infimum symbol
inline auto Infimum() -> Symbol {
    return Symbol{clingo_symbol_create_infimum(), false};
}

//! Construct a string symbol from a string.
//!
//! @param lib the library for storing symbols
//! @param str the string to convert
//! @return a symbol representing the string
inline auto String(Library const &lib, std::string_view str) -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_string>(c_cast(lib), str.data(), str.size()), false};
}

//! Construct a function symbol from a name and arguments.
//!
//! The positivity of the function is used to model classical negation of
//! symbolic atoms. However, it can also be used in term contexts.
//!
//! @param lib the library for storing symbols
//! @param str the name of the function
//! @param arguments the arguments of the function
//! @param is_positive whether the function is positive
inline auto Function(Library const &lib, std::string_view str, SymbolSpan arguments = {}, bool is_positive = true)
    -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_function>(c_cast(lib), str.data(), str.size(),
                                                              c_cast(arguments.data()), arguments.size(), is_positive),
                  false};
}

//! Construct a function symbol from a name and arguments.
//!
//! The positivity of the function is used to model classical negation of
//! symbolic atoms. However, it can also be used in term contexts.
//!
//! @param lib the library for storing symbols
//! @param str the name of the function
//! @param arguments the arguments of the function
//! @param is_positive whether the function is positive
inline auto Function(Library const &lib, std::string_view str, SymbolList arguments, bool is_positive = true)
    -> Symbol {
    return Function(lib, str, std::span{arguments.begin(), arguments.end()}, is_positive);
}

//! Construct a tuple symbol from a list of arguments.
//!
//! @param lib the library for storing symbols
//! @param arguments the arguments of the tuple
//! @return a symbol representing the tuple
inline auto Tuple(Library const &lib, SymbolSpan arguments = {}) -> Symbol {
    return Symbol{Detail::call<clingo_symbol_create_tuple>(c_cast(lib), c_cast(arguments.data()), arguments.size()),
                  false};
}

//! Construct a tuple symbol from a list of arguments.
//!
//! @param lib the library for storing symbols
//! @param arguments the arguments of the tuple
//! @return a symbol representing the tuple
inline auto Tuple(Library const &lib, SymbolList arguments) -> Symbol {
    return Tuple(lib, std::span{arguments.begin(), arguments.end()});
}

//! Parse the given string and evaluate it to a symbol.
//!
//! @param lib the library for storing symbols
//! @param str the string to parse
inline auto parse_term(Library const &lib, std::string_view str) -> Symbol {
    return Symbol{Detail::call<clingo_parse_term>(c_cast(lib), str.data(), str.size()), false};
}

//! @}

} // namespace Clingo

namespace std {

template <> struct hash<Clingo::Symbol> {
    auto operator()(Clingo::Symbol const &sym) const noexcept -> size_t { return sym.hash(); }
};

} // namespace std
