#pragma once

#include <gringo/core/number.hh>

#include <gringo/util/hash.hh>
#include <gringo/util/unordered_set.hh>

#include <ostream>
#include <span>

namespace Gringo {

//! @addtogroup core_symbol
//! @{

// TODO: there need to be two ways to create a symbol:
// - floating ones, and
// - reference counted ones.
// The SymbolRef and StringRef classes should have an accompanying Symbol and String classes.
// New symbols can only be created when the gc is not running.
// The Ref classes should duplicate the interface.

//! Reference to a string stored in a symbol store.
class StringRef {
  public:
    //! Construct an empty string.
    constexpr StringRef() : StringRef{0} {};

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
    friend auto operator==(StringRef a, StringRef b) -> bool { return a.rep_ == b.rep_; }
    //! Equality compare a string and a string view.
    friend auto operator==(StringRef a, std::string_view b) -> bool { return a.view() == b; }
    //! Equality compare a string view and a string.
    friend auto operator==(std::string_view a, StringRef b) -> bool { return a == b.view(); }

    //! Less than compare two strings.
    friend auto operator<=>(StringRef a, StringRef b) -> std::strong_ordering { return a.view() <=> b.view(); }
    //! Less than compare a string view and a string.
    friend auto operator<=>(std::string_view a, StringRef b) -> std::strong_ordering { return a <=> b.view(); }
    //! Less than compare a string and a string view.
    friend auto operator<=>(StringRef a, std::string_view b) -> std::strong_ordering { return a.view() <=> b; }

    //! Convert a string to its representation.
    static auto to_rep(StringRef str) noexcept -> uint64_t { return static_cast<uint64_t>(str.rep_); }
    //! Construct a string from its representation.
    static auto from_rep(uint64_t rep) noexcept -> StringRef { return StringRef{static_cast<uintptr_t>(rep)}; }

  private:
    constexpr StringRef(uintptr_t rep) noexcept : rep_{rep} {}
    uintptr_t rep_;
};

} // namespace Gringo

//! Hasher for strings.
template <> struct std::hash<Gringo::StringRef> : private std::hash<uint64_t> {
    //! Compute hash of string.
    auto operator()(Gringo::StringRef str) const -> size_t {
        return Gringo::Util::value_hash(Gringo::StringRef::to_rep(str));
    }
};

namespace Gringo {

//! A set of strings.
using StringRefSet = Util::unordered_set<StringRef>;
//! A vector of strings.
using StringRefVec = std::vector<StringRef>;

//! Output the given string (as is).
auto operator<<(std::ostream &out, StringRef const &str) -> std::ostream &;

//! Enumeration of available symbols types.
//!
//! See the documentation of the corresponding functions in the SymbolStore.
enum class SymbolType : uint8_t { number, sup, inf, string, tuple, function };

class SymbolRef;
//! A span of symbols.
using SymbolSpan = std::span<SymbolRef const>;

//! Variant-like class to store symbols.
class SymbolRef {
  public:
    //! Get the type of the symbol.
    [[nodiscard]] auto type() const noexcept -> SymbolType;
    //! Get the numeric value of the symbol.
    [[nodiscard]] auto num() const noexcept -> NumberRef;
    //! Get the (raw) string value of the symbol.
    [[nodiscard]] auto str() const noexcept -> StringRef;
    //! Get the name of the symbol.
    [[nodiscard]] auto name() const noexcept -> StringRef;
    //! Get the arguments of the symbol.
    [[nodiscard]] auto args() const noexcept -> SymbolSpan;
    //! Flip the classical sign of the symbol.
    [[nodiscard]] auto flip_classical_sign() const -> std::optional<SymbolRef>;
    //! Check whether the symbol has a sign.
    //!
    //! Returns true for negative integers and negated functions.
    [[nodiscard]] auto has_sign() const -> bool;
    //! Check whether the symbol has a sign.
    //!
    //! Returns true for negated functions.
    [[nodiscard]] auto has_classical_sign() const -> bool;

    //! Compare two symbols.
    friend auto compare(SymbolRef a, SymbolRef b) -> int;

    //! Equality compare two symbols.
    friend auto operator==(SymbolRef a, SymbolRef b) -> bool { return a.rep_ == b.rep_; }

    //! Less than compare two symbols.
    friend auto operator<=>(SymbolRef a, SymbolRef b) -> std::strong_ordering { return compare(a, b) <=> 0; }

    //! Get the representation of the symbol.
    static auto to_rep(SymbolRef sym) noexcept -> uint64_t { return sym.rep_; }
    //! Create a symbol from its representation.
    static auto from_rep(uint64_t rep) noexcept -> SymbolRef { return SymbolRef{rep}; }

    //! Output the given symbol.
    friend auto operator<<(std::ostream &out, SymbolRef const &sym) -> std::ostream &;

  private:
    SymbolRef(uint64_t repr) noexcept : rep_{repr} {}
    uint64_t rep_;
};

//! A vector of symbols.
using SymbolRefVec = std::vector<SymbolRef>;

} // namespace Gringo

//! Hasher for symbols.
template <> struct std::hash<Gringo::SymbolRef> : private std::hash<uint64_t> {
    //! Compute hash of symbol.
    auto operator()(Gringo::SymbolRef sym) const -> size_t {
        return Gringo::Util::value_hash(Gringo::SymbolRef::to_rep(sym));
    }
};

namespace Gringo {

class Symbol {
  public:
    //! Take ownership of the symbol.
    Symbol(SymbolRef sym) noexcept;
    //! Release ownership of the held symbol.
    ~Symbol() noexcept;
    //! Copy constructor.
    Symbol(Symbol const &sym) noexcept : Symbol{sym.get()} {}
    //! Move constructor.
    Symbol(Symbol &&sym) noexcept : sym_{SymbolRef::from_rep(0)} { std::swap(sym.sym_, sym_); }
    //! Copy assignment.
    auto operator=(Symbol const &sym) noexcept -> Symbol & { return *this = Symbol(sym.get()); }
    //! Move assignment.
    auto operator=(Symbol &&sym) noexcept -> Symbol & {
        std::swap(sym.sym_, sym_);
        return *this;
    }
    [[nodiscard]] auto get() const -> SymbolRef const & { return sym_; }
    operator SymbolRef const &() const { return sym_; }
    [[nodiscard]] auto operator->() const -> SymbolRef const * { return &sym_; }
    [[nodiscard]] auto operator*() const -> SymbolRef const & { return get(); }

  private:
    SymbolRef sym_;
};

class SymbolCollector {
  public:
    void mark(SymbolRef const &sym);
    void mark(StringRef const &str);

  private:
    std::vector<SymbolRef> stack_;
};

class SymbolOwner {
  public:
    virtual ~SymbolOwner() = default;
    virtual void mark(SymbolCollector &gc) const = 0;
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
    [[nodiscard]] static auto sup() noexcept -> SymbolRef;
    //! Construct the supremum constant (<tt>\#sup</tt>).
    [[nodiscard]] static auto inf() noexcept -> SymbolRef;
    //! Construct a quoted string.
    //!
    //! A raw string is stored and quoted when the symbol is output.
    //! For example: <tt>"foo\nbar"</tt>.
    [[nodiscard]] static auto str(StringRef str) noexcept -> SymbolRef;
    //! Construct a number (e.g., <tt>42</tt>).
    [[nodiscard]] auto num(Number const &num) noexcept -> SymbolRef;
    //! Construct a number (avoids copying the number).
    [[nodiscard]] auto num(Number &&num) noexcept -> SymbolRef;
    //! Construct a tuple.
    //!
    //! For example: <tt>(x,y)</tt>.
    [[nodiscard]] auto tup(SymbolSpan args) -> SymbolRef;
    //! Construct a function symbol.
    //!
    //! For example: <tt>f(x,y)</tt>.
    [[nodiscard]] auto fun(StringRef name, SymbolSpan args, bool sign) -> SymbolRef;
    //! Construct a string.
    //!
    //! The string is stored as is.
    [[nodiscard]] auto string(std::string_view str) -> StringRef;

    //! Block garbage collection.
    //!
    //! Block/unblock calls must be balanced. No symbols should be accessed
    //! while a store is unblocked (if a call to gc is intended).
    void gc_block() noexcept { do_gc_block(true); }
    //! Unblock garbage collection.
    //!
    //! Block/unblock calls must be balanced. In the multi-threaded case,
    //! floating symbols must only be used while a store is blocked.
    void gc_unblock() noexcept { do_gc_block(false); }
    //! Add a symbol owner.
    void gc_add_owner(SymbolOwner const &owner) { do_gc_add_owner(owner); }
    //! Delete a symbol owner.
    void gc_del_owner(SymbolOwner const &owner) noexcept { do_gc_del_owner(owner); }
    //! Cleanup symbols.
    void gc() { do_gc(); }

  private:
    [[nodiscard]] virtual auto do_tup(SymbolSpan args) -> SymbolRef = 0;
    [[nodiscard]] virtual auto do_fun(StringRef str, SymbolSpan args, bool sign) -> SymbolRef = 0;
    [[nodiscard]] virtual auto do_string(std::string_view str) -> StringRef = 0;
    [[nodiscard]] virtual auto do_num(Number const &num) noexcept -> SymbolRef = 0;
    [[nodiscard]] virtual auto do_num(Number &&num) noexcept -> SymbolRef = 0;

    virtual void do_gc_block(bool block) noexcept = 0;
    virtual void do_gc_add_owner(SymbolOwner const &owner) = 0;
    virtual void do_gc_del_owner(SymbolOwner const &owner) noexcept = 0;
    virtual void do_gc() = 0;
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

//! Generator for auxiliary names.
class NameGen {
  public:
    //! Constructor taking a set of variables names.
    //!
    //! The generator ensures that there are no collisions with these names.
    NameGen(SymbolStore &store, StringRefSet names, char const *prefix)
        : store_{store}, names_{std::move(names)}, prefix_{prefix} {}
    //! Delete move/copy constructor.
    NameGen(NameGen &&) noexcept = delete;
    //! Initialize/reset the name generator.
    void init(StringRefSet names, char const *prefix) {
        num_ = 0;
        names_ = std::move(names);
        prefix_ = prefix;
    }
    //! Add a name returning true if it is not yet used.
    [[nodiscard]] auto add_name(StringRef name) -> bool { return names_.emplace(name).second; }
    //! Generate a unique variable name.
    [[nodiscard]] auto new_name() -> StringRef;
    //! Return the associated symbol store.
    [[nodiscard]] auto store() const -> SymbolStore & { return store_; }

  private:
    //! Symbol store to store strings.
    SymbolStore &store_;
    //! Taken variable names.
    StringRefSet names_;
    //! The prefix of the generated names.
    char const *prefix_;
    //! Running number used to generate names.
    size_t num_ = 0;
};

//! @}

} // namespace Gringo
