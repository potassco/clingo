#pragma once

#include <ostream>
#include <span>
#include <string>

#include <util/hash.hh>

namespace Gringo {

//! @defgroup core Core
//!
//! Core functionality shared across modules.

//! @defgroup core_symbol Symbols
//! @ingroup core
//!
//! Data structures and functions to represent symbols.
//!
//! The symbol implementation available here is currently a placeholder.
//! Something similar to what is used in gringo should be used.
//!
//! @{

//! The supremum and infimum constants.
//!
//! @todo Belongs into statement.
enum class Constant : int {
    supremum, //!< The supremum (<tt>\#sup</tt>).
    infimum,  //!< The infimum (<tt>\#inf</tt>).
};

//! Output the string representation of the constant.
auto operator<<(std::ostream &out, Constant op) -> std::ostream &;

//! Reference to a string stored in a symbol store.
class String {
  public:
    constexpr String() : String{0} {};

    [[nodiscard]] auto c_str() const -> const char *;
    [[nodiscard]] auto view() const -> std::string_view;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto starts_with(std::string_view prefix) const -> bool;

    friend auto operator==(String a, String b) -> bool { return a.rep_ == b.rep_; }
    friend auto operator==(String a, std::string_view b) -> bool { return a.view() == b; }
    friend auto operator==(std::string_view a, String b) -> bool { return a == b.view(); }

    static auto to_rep(String str) noexcept -> uint64_t { return static_cast<uint64_t>(str.rep_); }
    static auto from_rep(uint64_t rep) noexcept -> String { return String{static_cast<uintptr_t>(rep)}; }

  private:
    constexpr String(uintptr_t rep) noexcept : rep_{rep} {}
    uintptr_t rep_;
};

//! Output the given string (as is).
auto operator<<(std::ostream &out, String const &str) -> std::ostream &;

//! Enumeration of available symbols types.
//!
//! See the documentation of the corresponding functions in the SymbolStore.
enum class SymbolType { number, sup, inf, string, tuple, function };

class Symbol;
using SymbolSpan = std::span<Symbol const>;

class Symbol {
  public:
    [[nodiscard]] auto type() const noexcept -> SymbolType;
    [[nodiscard]] auto num() const noexcept -> int32_t;
    [[nodiscard]] auto str() const noexcept -> String;
    [[nodiscard]] auto name() const noexcept -> String;
    [[nodiscard]] auto args() const noexcept -> SymbolSpan;
    [[nodiscard]] auto flip_sign() const -> std::optional<Symbol>;

    //! Check whether the symbol has a sign.
    //!
    //! Returns true for negative integers and negated functions.
    [[nodiscard]] auto has_sign() const -> bool;

    friend auto operator==(Symbol a, Symbol b) -> bool { return a.rep_ == b.rep_; }
    friend auto operator!=(Symbol a, Symbol b) -> bool { return a.rep_ != b.rep_; }

    static auto to_rep(Symbol sym) noexcept -> uint64_t { return sym.rep_; }
    static auto from_rep(uint64_t rep) noexcept -> Symbol { return Symbol{rep}; }

  private:
    Symbol(uint64_t repr) noexcept : rep_{repr} {}
    uint64_t rep_;
};

//! Output the given symbol.
auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream &;

//! A store for symbols.
//!
//! Symbols are stored in a hash table ensuring that there is just one
//! representation for a symbol. Constants and numbers are stored directly in
//! the symbol.
class SymbolStore {
  public:
    //!< Construct the infimum constant (<tt>\#inf</tt>).
    [[nodiscard]] static auto sup() noexcept -> Symbol;
    //!< Construct the supremum constant (<tt>\#sup</tt>).
    [[nodiscard]] static auto inf() noexcept -> Symbol;
    //!< Construct a number (e.g., <tt>42</tt>).
    [[nodiscard]] static auto num(int32_t num) noexcept -> Symbol;
    //! Construct a quoted string.
    //!
    //! A raw string is stored that is quoted when the symbol is output..
    //! For example: <tt>"foo\nbar"</tt>.
    [[nodiscard]] static auto str(String str) noexcept -> Symbol;
    //! Construct a tuple.
    //!
    //! For example: <tt>(x,y)</tt>.
    [[nodiscard]] virtual auto tup(SymbolSpan args) -> Symbol = 0;
    //! Construct a function symbol.
    //!
    //! For example: <tt>f(x,y)</tt>.
    [[nodiscard]] virtual auto fun(String str, SymbolSpan args, bool sign) -> Symbol = 0;
    //! Construct a string.
    //!
    //! The string is stored as is.
    [[nodiscard]] virtual auto string(std::string_view str) -> String = 0;
    virtual ~SymbolStore() noexcept = default;
};

//! A pointer to a symbol store.
using USymbolStore = std::unique_ptr<SymbolStore>;

//! Initialize the default symbol store.
//!
//! Fails if there is already a default one.
void init_default_symbol_store(USymbolStore store);

//! Get the default symbol store.
//!
//! If no symbol store has been set, a default one that is *not* thread-safe is
//! setup and returned.
auto default_symbol_store() -> SymbolStore &;

//! Construct a new symbol store.
//!
//! Either a default store for single-threaded use or a locked one for
//! multi-threaded use can be created.
//!
//! Furthermore, it can be selected whether the store can use global state.
//! Using global state speeds up allocation but can only be used in shared mode
//! if there is more than one thread accessing symbols.
auto make_symbol_store(bool local, bool shared) -> USymbolStore;

} // namespace Gringo

//! @}

namespace std {

template <> struct hash<Gringo::String> {
    auto operator()(Gringo::String str) const -> size_t {
        return Gringo::Util::value_hash(Gringo::String::to_rep(str));
    }
};

template <> struct hash<Gringo::Symbol> {
    auto operator()(Gringo::Symbol sym) const -> size_t {
        return Gringo::Util::value_hash(Gringo::Symbol::to_rep(sym));
    }
};

} // namespace std
