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
class StringRef {
  public:
    //! Construct an empty string.
    //!
    //! Empty strings exist independently of symbol stores.
    constexpr StringRef() = default;

    //! Convert a string to its integer representation.
    static auto to_rep(StringRef str) noexcept -> uint64_t { return static_cast<uint64_t>(str.rep_); }
    //! Construct a string from its integer representation.
    static auto from_rep(uint64_t rep) noexcept -> StringRef { return StringRef{static_cast<uintptr_t>(rep)}; }

    //! Get the underlying C string.
    //!
    //! The lifetime is tied to that of the reference.
    [[nodiscard]] auto c_str() const -> const char *;
    //! Get a string view.
    //!
    //! The lifetime is tied to that of the reference.
    [[nodiscard]] auto view() const -> std::string_view;
    //! Test if the string is empty.
    [[nodiscard]] auto empty() const -> bool;
    //! Get the length of the string.
    [[nodiscard]] auto size() const -> size_t;
    //! Check if the string starts with the given string.
    [[nodiscard]] auto starts_with(std::string_view prefix) const -> bool;

    //! Compute the hash of the string.
    [[nodiscard]] auto hash() const -> size_t { return Gringo::Util::value_hash(rep_); }

    //! Equality compare two strings.
    friend auto operator==(StringRef a, StringRef b) -> bool { return a.rep_ == b.rep_; }
    //! Equality compare a string and a string view.
    friend auto operator==(StringRef a, std::string_view b) -> bool { return a.view() == b; }
    //! Equality compare a string view and a string.
    friend auto operator==(std::string_view a, StringRef b) -> bool { return a == b.view(); }

    //! Less than compare two strings.
    friend auto operator<=>(StringRef a, StringRef b) -> std::strong_ordering { return a.view() <=> b.view(); }
    //! Less than compare two strings.
    friend auto operator<=>(std::string_view a, StringRef b) -> std::strong_ordering { return a <=> b.view(); }
    //! Less than compare two strings.
    friend auto operator<=>(StringRef a, std::string_view b) -> std::strong_ordering { return a.view() <=> b; }

    //! Output the given string.
    friend auto operator<<(std::ostream &out, StringRef const &str) -> std::ostream &;

  private:
    constexpr StringRef(uintptr_t rep) noexcept : rep_{rep} {}
    uintptr_t rep_ = 0;
};

//! A set of strings.
using StringRefSet = Util::unordered_set<StringRef>;
//! A vector of strings.
using StringRefVec = std::vector<StringRef>;

//! Class managing the lifetime of a StringRef.
//!
//! References held by this class are not collected by the gc method of the
//! symbol store.
class String {
  public:
    //! Construct an empty string.
    constexpr String() = default;
    //! Take ownership of the string reference.
    String(StringRef ref) noexcept : ref_{ref} { acquire_(); }
    //! Release ownership of the held string reference.
    ~String() noexcept { release_(); }
    //! Copy constructor.
    String(String const &other) noexcept : ref_{other.ref_} { acquire_(); }
    //! Move constructor.
    String(String &&other) noexcept { std::swap(other.ref_, ref_); }
    //! Copy assignment.
    auto operator=(String const &other) noexcept -> String & {
        if (&other != this) {
            release_();
            ref_ = other.ref_;
            acquire_();
        }
        return *this;
    }
    //! Move assignment.
    auto operator=(String &&other) noexcept -> String & {
        if (&other != this) {
            release_();
            ref_ = other.ref_;
            other.ref_ = StringRef::from_rep(0);
        }
        return *this;
    }
    //! Get the contained string reference.
    //!
    //! The lifetime is tied to the string.
    [[nodiscard]] auto get() const -> StringRef const & { return ref_; }
    //! Get an integer representation of the string.
    //!
    //! To correctly free the string, it has to passed to from_rep again.
    [[nodiscard]] static auto to_rep(String sym) -> uint64_t {
        auto ret = sym.ref_;
        sym.ref_ = StringRef{};
        return StringRef::to_rep(ret);
    }
    //! Construct a string from its representation.
    [[nodiscard]] static auto from_rep(uint64_t repr) -> String {
        auto ret = String();
        ret.ref_ = StringRef::from_rep(repr);
        return ret;
    }

    //! Get the underlying C string.
    [[nodiscard]] auto c_str() const -> const char * { return ref_.c_str(); }
    //! Get a string view.
    [[nodiscard]] auto view() const -> std::string_view { return ref_.c_str(); }
    //! Test if the string is empty.
    [[nodiscard]] auto empty() const -> bool { return ref_.empty(); }
    //! Get the length of the string.
    [[nodiscard]] auto size() const -> size_t { return ref_.size(); }
    //! Check if the string starts with the given string.
    [[nodiscard]] auto starts_with(std::string_view prefix) const -> bool { return ref_.starts_with(prefix); }

    //! Compute the hash of the string.
    [[nodiscard]] auto hash() const -> size_t { return Gringo::Util::value_hash(ref_); }

    //! Equality compare two strings.
    friend auto operator==(String const &a, String const &b) -> bool { return a.ref_ == b.get(); }
    //! Equality compare two strings.
    friend auto operator==(StringRef const &a, String const &b) -> bool { return a == b.get(); }
    //! Equality compare two strings.
    friend auto operator==(String const &a, StringRef const &b) -> bool { return a.get() == b; }
    //! Equality compare a string and a string view.
    friend auto operator==(String const &a, std::string_view const &b) -> bool { return a.get() == b; }
    //! Equality compare a string view and a string.
    friend auto operator==(std::string_view const &a, String const &b) -> bool { return a == b.get(); }

    //! Less than compare two strings.
    friend auto operator<=>(String const &a, String const &b) -> std::strong_ordering { return a.get() <=> b.get(); }
    //! Less than compare two strings.
    friend auto operator<=>(StringRef const &a, String const &b) -> std::strong_ordering { return a <=> b.get(); }
    //! Less than compare two strings.
    friend auto operator<=>(String const &a, StringRef const &b) -> std::strong_ordering { return a.get() <=> b; }
    //! Less than compare a string view and a string.
    friend auto operator<=>(std::string_view const &a, String const &b) -> std::strong_ordering {
        return a <=> b.get();
    }
    //! Less than compare a string and a string view.
    friend auto operator<=>(String const &a, std::string_view const &b) -> std::strong_ordering {
        return a.get() <=> b;
    }

    //! Output the given string.
    friend auto operator<<(std::ostream &out, String const &str) -> std::ostream & {
        out << str.get();
        return out;
    }

  private:
    void acquire_() const noexcept;
    void release_() const noexcept;

    StringRef ref_;
};

//! Enumeration of available symbols types.
//!
//! See the documentation of the corresponding functions in the SymbolStore.
enum class SymbolType : uint8_t { number, sup, inf, string, tuple, function };

class SymbolRef;
//! A span of symbols.
using SymbolRefSpan = std::span<SymbolRef const>;
//! A vector of symbols.
using SymbolRefVec = std::vector<SymbolRef>;

//! Variant-like class to store symbols stored in a symbol store.
class SymbolRef {
  public:
    //! Create a reference to number zero.
    //!
    //! Number zero can exist independently of a symbol store.
    constexpr SymbolRef() = default;
    //! Get the type of the symbol.
    [[nodiscard]] auto type() const noexcept -> SymbolType;
    //! Get the numeric value of the symbol.
    [[nodiscard]] auto num() const noexcept -> NumberRef;
    //! Get the (raw) string value of the symbol.
    [[nodiscard]] auto str() const noexcept -> StringRef;
    //! Get the name of the symbol.
    [[nodiscard]] auto name() const noexcept -> StringRef;
    //! Get the arguments of the symbol.
    [[nodiscard]] auto args() const noexcept -> SymbolRefSpan;
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

    //! Compute the hash of the symbol.
    [[nodiscard]] auto hash() const -> size_t { return Gringo::Util::value_hash(rep_); }

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
    uint64_t rep_ = 0;
};

//! Class managing the lifetime of a SymbolRef.
//!
//! References held by this class are not collected by the gc method of the
//! symbol store.
class Symbol {
  public:
    //! Create a symbol for number zero.
    //!
    //! Number zero can exist independently of a symbol store.
    constexpr Symbol() noexcept = default;
    //! Take ownership of the symbol.
    Symbol(SymbolRef sym) noexcept : ref_{sym} { acquire_(); }
    //! Release ownership of the held symbol.
    ~Symbol() noexcept { release_(); }
    //! Copy constructor.
    Symbol(Symbol const &sym) noexcept : ref_{sym.ref_} { acquire_(); }
    //! Move constructor.
    Symbol(Symbol &&sym) noexcept { std::swap(sym.ref_, ref_); }
    //! Copy assignment.
    auto operator=(Symbol const &sym) noexcept -> Symbol & {
        if (&sym != this) {
            release_();
            ref_ = sym.ref_;
            acquire_();
        }
        return *this;
    }
    //! Move assignment.
    auto operator=(Symbol &&sym) noexcept -> Symbol & {
        if (&sym != this) {
            release_();
            ref_ = sym.ref_;
            sym.ref_ = SymbolRef::from_rep(0);
        }
        return *this;
    }
    //! Get a reference to the contained symbol.
    //!
    //! The lifetime is tied to the symbol.
    [[nodiscard]] auto get() const -> SymbolRef const & { return ref_; }
    //! Get an integer representation of the symbol.
    //!
    //! The representation increments the reference count of the symbol.
    //! To correctly free the symbol, it has to passed to from_rep again.
    [[nodiscard]] static auto to_rep(Symbol const &sym) -> uint64_t {
        sym.acquire_();
        return SymbolRef::to_rep(sym.ref_);
    }
    [[nodiscard]] static auto from_rep(uint64_t repr) -> Symbol {
        auto ret = Symbol();
        ret.ref_ = SymbolRef::from_rep(repr);
        return ret;
    }

    //! Get the type of the symbol.
    [[nodiscard]] auto type() const noexcept -> SymbolType { return ref_.type(); }
    //! Get the numeric value of the symbol.
    [[nodiscard]] auto num() const noexcept -> NumberRef { return ref_.num(); }
    //! Get the (raw) string value of the symbol.
    [[nodiscard]] auto str() const noexcept -> StringRef { return ref_.str(); }
    //! Get the name of the symbol.
    [[nodiscard]] auto name() const noexcept -> StringRef { return ref_.name(); }
    //! Get the arguments of the symbol.
    //!
    //! The lifetime of the arguments is tied to the symbol.
    [[nodiscard]] auto args() const noexcept -> SymbolRefSpan { return ref_.args(); }
    //! Flip the classical sign of the symbol.
    //!
    //! The lifetime of the result is tied to the symbol.
    [[nodiscard]] auto flip_classical_sign() const -> std::optional<SymbolRef> { return ref_.flip_classical_sign(); }
    //! Check whether the symbol has a sign.
    //!
    //! Returns true for negative integers and negated functions.
    [[nodiscard]] auto has_sign() const -> bool { return ref_.has_sign(); }
    //! Check whether the symbol has a sign.
    //!
    //! Returns true for negated functions.
    [[nodiscard]] auto has_classical_sign() const -> bool;

    //! Compute the hash of the symbol.
    [[nodiscard]] auto hash() const -> size_t { return Gringo::Util::value_hash(ref_); }

    //! Compare two symbols.
    friend auto compare(Symbol const &a, Symbol const &b) -> int { return compare(a.get(), b.get()); }
    friend auto compare(SymbolRef const &a, Symbol const &b) -> int { return compare(a, b.get()); }
    friend auto compare(Symbol const &a, SymbolRef const &b) -> int { return compare(a.get(), b); }

    //! Equality compare two symbols.
    friend auto operator==(Symbol const &a, Symbol const &b) -> bool { return a.get() == b.get(); }
    friend auto operator==(SymbolRef const &a, Symbol const &b) -> bool { return a == b.get(); }
    friend auto operator==(Symbol const &a, SymbolRef const &b) -> bool { return a.get() == b; }

    //! Less than compare two symbols.
    friend auto operator<=>(Symbol const &a, Symbol const &b) -> std::strong_ordering { return compare(a, b) <=> 0; }
    friend auto operator<=>(SymbolRef const &a, Symbol const &b) -> std::strong_ordering { return compare(a, b) <=> 0; }
    friend auto operator<=>(Symbol const &a, SymbolRef const &b) -> std::strong_ordering { return compare(a, b) <=> 0; }

    //! Output the given symbol.
    friend auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream & { return out << sym.get(); }

  private:
    void acquire_() const noexcept;
    void release_() const noexcept;
    SymbolRef ref_ = SymbolRef::from_rep(0);
};

//! A span of symbols.
using SymbolSpan = std::span<Symbol const>;
//! A vector of symbols.
using SymbolVec = std::vector<Symbol>;

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

// TODO: there need to be two ways to create a symbol:
// - floating ones, and
// - reference counted ones.
// The do functions can be extended with a parameter indicated whether
// referenced or floading symbols shall be returned.

//! A store for symbols.
//!
//! Symbols are stored in a hash table ensuring that there is just one
//! representation for a symbol. Constants and numbers are stored directly in
//! the symbol.
class SymbolStore {
  public:
    //! Destroy the store and all symbols in it.
    virtual ~SymbolStore() noexcept = default;

    //! Construct a string.
    //!
    //! The string is stored as is.
    [[nodiscard]] auto string(std::string_view str) -> String;

    //! Construct the infimum constant (<tt>\#inf</tt>).
    [[nodiscard]] static auto sup() noexcept -> SymbolRef;
    //! Construct the supremum constant (<tt>\#sup</tt>).
    [[nodiscard]] static auto inf() noexcept -> SymbolRef;
    //! Construct a quoted string.
    //
    //! A raw string is stored and quoted when the symbol is output.
    //! For example: <tt>"foo\nbar"</tt>.
    [[nodiscard]] static auto str(String str) noexcept -> Symbol;
    //! Construct a number.
    [[nodiscard]] auto num(Number num) noexcept -> Symbol;
    //! Construct a tuple.
    //!
    //! For example: <tt>(x,y)</tt>.
    [[nodiscard]] auto tup(SymbolSpan args) -> Symbol;
    //! @copydoc tup(SymbolSpan)
    [[nodiscard]] auto tup(SymbolRefSpan args) -> Symbol;
    //! Construct a function symbol.
    //!
    //! For example: <tt>f(x,y)</tt>.
    [[nodiscard]] auto fun(String const &name, SymbolSpan args, bool sign) -> Symbol;
    //! @copydoc fun(String const &, SymbolSpan, bool)
    [[nodiscard]] auto fun(StringRef name, SymbolRefSpan args, bool sign) -> Symbol;

    // interface to create floating references

    //! Construct a string.
    //!
    //! The string is stored as is.
    [[nodiscard]] auto string_ref(std::string_view str) -> StringRef;

    //! Construct a quoted string.
    //!
    //! A raw string is stored and quoted when the symbol is output.
    //! For example: <tt>"foo\nbar"</tt>.
    [[nodiscard]] static auto str_ref(StringRef str) noexcept -> SymbolRef;
    //! Construct a number (e.g., <tt>42</tt>).
    [[nodiscard]] auto num_ref(Number num) noexcept -> SymbolRef;
    //! Construct a tuple.
    //!
    //! For example: <tt>(x,y)</tt>.
    [[nodiscard]] auto tup_ref(SymbolRefSpan args) -> SymbolRef;
    //! Construct a function symbol.
    //!
    //! For example: <tt>f(x,y)</tt>.
    [[nodiscard]] auto fun_ref(StringRef name, SymbolRefSpan args, bool sign) -> SymbolRef;

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
    [[nodiscard]] virtual auto do_tup(SymbolRefSpan args) -> SymbolRef = 0;
    [[nodiscard]] virtual auto do_fun(StringRef str, SymbolRefSpan args, bool sign) -> SymbolRef = 0;
    [[nodiscard]] virtual auto do_string(std::string_view str) -> StringRef = 0;
    [[nodiscard]] virtual auto do_num(Number num) noexcept -> SymbolRef = 0;

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
