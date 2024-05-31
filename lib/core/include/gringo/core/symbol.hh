#pragma once

#include <gringo/core/number.hh>

#include <gringo/util/hash.hh>
#include <gringo/util/unordered_set.hh>

#include <ostream>
#include <span>

namespace Gringo {

//! @addtogroup core_symbol
//! @{

//! Reference to a string stored in a symbol store.
class String {
  public:
    //! Construct an empty string.
    constexpr String() : String{0} {};

    //! Get the underlying C string.
    [[nodiscard]] auto c_str() const -> const char *;
    //! Get a string view.
    [[nodiscard]] auto view() const -> std::string_view;
    //! Test if the string is empty.
    [[nodiscard]] auto empty() const -> bool;
    //! Get the length of the string.
    [[nodiscard]] auto size() const -> size_t;
    //! Check if the string starts with the given string.
    [[nodiscard]] auto starts_with(std::string_view prefix) const -> bool;

    //! Equality compare two strings.
    friend auto operator==(String a, String b) -> bool { return a.rep_ == b.rep_; }
    //! Equality compare a string and a string view.
    friend auto operator==(String a, std::string_view b) -> bool { return a.view() == b; }
    //! Equality compare a string view and a string.
    friend auto operator==(std::string_view a, String b) -> bool { return a == b.view(); }

    //! Less than compare two strings.
    friend auto operator<=>(String a, String b) -> std::strong_ordering { return a.view() <=> b.view(); }
    //! Less than compare a string view and a string.
    friend auto operator<=>(std::string_view a, String b) -> std::strong_ordering { return a <=> b.view(); }
    //! Less than compare a string and a string view.
    friend auto operator<=>(String a, std::string_view b) -> std::strong_ordering { return a.view() <=> b; }

    //! Convert a string to its representation.
    static auto to_rep(String str) noexcept -> uint64_t { return static_cast<uint64_t>(str.rep_); }
    //! Construct a string from its representation.
    static auto from_rep(uint64_t rep) noexcept -> String { return String{static_cast<uintptr_t>(rep)}; }

  private:
    constexpr String(uintptr_t rep) noexcept : rep_{rep} {}
    uintptr_t rep_;
};

} // namespace Gringo

//! Hasher for strings.
template <> struct std::hash<Gringo::String> : private std::hash<uint64_t> {
    //! Compute hash of string.
    auto operator()(Gringo::String str) const -> size_t {
        return std::hash<uint64_t>::operator()(Gringo::String::to_rep(str));
    }
};

namespace Gringo {

//! A set of strings.
using StringSet = Util::unordered_set<String>;
//! A vector of strings.
using StringVec = std::vector<String>;

//! Output the given string (as is).
auto operator<<(std::ostream &out, String const &str) -> std::ostream &;

//! Enumeration of available symbols types.
//!
//! See the documentation of the corresponding functions in the SymbolStore.
enum class SymbolType : uint8_t { number, sup, inf, string, tuple, function };

class Symbol;
//! A span of symbols.
using SymbolSpan = std::span<Symbol const>;

//! Variant-like class to store symbols.
class Symbol {
  public:
    //! Get the type of the symbol.
    [[nodiscard]] auto type() const noexcept -> SymbolType;
    //! Get the numeric value of the symbol.
    [[nodiscard]] auto num() const noexcept -> NumberRef;
    //! Get the (raw) string value of the symbol.
    [[nodiscard]] auto str() const noexcept -> String;
    //! Get the name of the symbol.
    [[nodiscard]] auto name() const noexcept -> String;
    //! Get the arguments of the symbol.
    [[nodiscard]] auto args() const noexcept -> SymbolSpan;
    //! Flip the classical sign of the symbol.
    [[nodiscard]] auto flip_classical_sign() const -> std::optional<Symbol>;
    //! Check whether the symbol has a sign.
    //!
    //! Returns true for negative integers and negated functions.
    [[nodiscard]] auto has_sign() const -> bool;
    //! Check whether the symbol has a sign.
    //!
    //! Returns true for negated functions.
    [[nodiscard]] auto has_classical_sign() const -> bool;

    //! Compare two symbols.
    friend auto compare(Symbol a, Symbol b) -> int;

    //! Equality compare two symbols.
    friend auto operator==(Symbol a, Symbol b) -> bool { return a.rep_ == b.rep_; }

    //! Less than compare two symbols.
    friend auto operator<=>(Symbol a, Symbol b) -> std::strong_ordering { return compare(a, b) <=> 0; }

    //! Get the representation of the symbol.
    static auto to_rep(Symbol sym) noexcept -> uint64_t { return sym.rep_; }
    //! Create a symbol from its representation.
    static auto from_rep(uint64_t rep) noexcept -> Symbol { return Symbol{rep}; }

    //! Output the given symbol.
    friend auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream &;

  private:
    Symbol(uint64_t repr) noexcept : rep_{repr} {}
    uint64_t rep_;
};

//! A vector of symbols.
using SymbolVec = std::vector<Symbol>;

} // namespace Gringo

//! Hasher for symbols.
template <> struct std::hash<Gringo::Symbol> : private std::hash<uint64_t> {
    //! Compute hash of symbol.
    auto operator()(Gringo::Symbol sym) const -> size_t {
        return std::hash<uint64_t>::operator()(Gringo::Symbol::to_rep(sym));
    }
};

namespace Gringo {

class SymbolRef {
  public:
    //! Take ownership of the symbol.
    SymbolRef(Symbol sym) noexcept;
    //! Release ownership of the held symbol.
    ~SymbolRef() noexcept;
    //! Copy constructor.
    SymbolRef(SymbolRef const &sym) noexcept : SymbolRef{sym.get()} {}
    //! Move constructor.
    SymbolRef(SymbolRef &&sym) noexcept : sym_{Symbol::from_rep(0)} { std::swap(sym.sym_, sym_); }
    //! Copy assignment.
    auto operator=(SymbolRef const &sym) noexcept -> SymbolRef & { return *this = SymbolRef(sym.get()); }
    //! Move assignment.
    auto operator=(SymbolRef &&sym) noexcept -> SymbolRef & {
        std::swap(sym.sym_, sym_);
        return *this;
    }
    [[nodiscard]] auto get() const -> Symbol const & { return sym_; }
    operator Symbol const &() const { return sym_; }
    [[nodiscard]] auto operator->() const -> Symbol const * { return &sym_; }
    [[nodiscard]] auto operator*() const -> Symbol const & { return get(); }

  private:
    Symbol sym_;
};

class SymbolCollector {
  public:
    void mark(Symbol const &sym);
};

class SymbolOwner {
  public:
    virtual ~SymbolOwner() = default;
    virtual void mark(SymbolCollector &gc) = 0;
};

//! A store for symbols.
//!
//! Symbols are stored in a hash table ensuring that there is just one
//! representation for a symbol. Constants and numbers are stored directly in
//! the symbol.
class SymbolStore {
  public:
    virtual ~SymbolStore() noexcept = default;
    //! Construct the infimum constant (<tt>\#inf</tt>).
    [[nodiscard]] static auto sup() noexcept -> Symbol;
    //! Construct the supremum constant (<tt>\#sup</tt>).
    [[nodiscard]] static auto inf() noexcept -> Symbol;
    //! Construct a quoted string.
    //!
    //! A raw string is stored and quoted when the symbol is output.
    //! For example: <tt>"foo\nbar"</tt>.
    [[nodiscard]] static auto str(String str) noexcept -> Symbol;
    //! Construct a number (e.g., <tt>42</tt>).
    [[nodiscard]] auto num(Number const &num) noexcept -> Symbol;
    //! Construct a number (avoids copying the number).
    [[nodiscard]] auto num(Number &&num) noexcept -> Symbol;
    //! Construct a tuple.
    //!
    //! For example: <tt>(x,y)</tt>.
    [[nodiscard]] auto tup(SymbolSpan args) -> Symbol;
    //! Construct a function symbol.
    //!
    //! For example: <tt>f(x,y)</tt>.
    [[nodiscard]] auto fun(String name, SymbolSpan args, bool sign) -> Symbol;
    //! Construct a string.
    //!
    //! The string is stored as is.
    [[nodiscard]] auto string(std::string_view str) -> String;

    void add_owner(SymbolOwner &owner);
    void remove_owner(SymbolOwner &owner) noexcept;
    void gc();

  private:
    [[nodiscard]] virtual auto do_tup(SymbolSpan args) -> Symbol = 0;
    [[nodiscard]] virtual auto do_fun(String str, SymbolSpan args, bool sign) -> Symbol = 0;
    [[nodiscard]] virtual auto do_string(std::string_view str) -> String = 0;
    [[nodiscard]] virtual auto do_num(Number const &num) noexcept -> Symbol = 0;
    [[nodiscard]] virtual auto do_num(Number &&num) noexcept -> Symbol = 0;

    std::vector<SymbolOwner *> owners_;
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
//! Optionally, a slotted allocator can be used to (hopefully) speed up
//! allocation.
//!
//! Either a default store for single-threaded use or a locked one for
//! multi-threaded use can be created.
auto make_symbol_store(bool slotted, bool shared) -> USymbolStore;

class SingleSymbolOwner : public SymbolOwner {
  public:
    SingleSymbolOwner(SymbolStore &store) : store_{&store} { store_->add_owner(*this); }
    ~SingleSymbolOwner() override { store_->remove_owner(*this); }

  private:
    SymbolStore *store_;
};

//! Generator for auxiliary names.
class NameGen {
  public:
    //! Constructor taking a set of variables names.
    //!
    //! The generator ensures that there are no collisions with these names.
    NameGen(SymbolStore &store, StringSet names, char const *prefix)
        : store_{store}, names_{std::move(names)}, prefix_{prefix} {}
    //! Delete move/copy constructor.
    NameGen(NameGen &&) noexcept = delete;
    //! Initialize/reset the name generator.
    void init(StringSet names, char const *prefix) {
        num_ = 0;
        names_ = std::move(names);
        prefix_ = prefix;
    }
    //! Add a name returning true if it is not yet used.
    [[nodiscard]] auto add_name(String name) -> bool { return names_.emplace(name).second; }
    //! Generate a unique variable name.
    [[nodiscard]] auto new_name() -> String;
    //! Return the associated symbol store.
    [[nodiscard]] auto store() const -> SymbolStore & { return store_; }

  private:
    //! Symbol store to store strings.
    SymbolStore &store_;
    //! Taken variable names.
    StringSet names_;
    //! The prefix of the generated names.
    char const *prefix_;
    //! Running number used to generate names.
    size_t num_ = 0;
};

//! @}

} // namespace Gringo
