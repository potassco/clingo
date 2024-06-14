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
    //!
    //! Empty strings exist independently of symbol stores.
    constexpr String() = default;

    //! Convert a string to its integer representation.
    static auto to_rep(String str) noexcept -> uint64_t { return static_cast<uint64_t>(str.rep_); }
    //! Construct a string from its integer representation.
    static auto from_rep(uint64_t rep) noexcept -> String { return String{static_cast<uintptr_t>(rep)}; }

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
    friend auto operator==(String a, String b) -> bool { return a.rep_ == b.rep_; }
    //! Equality compare a string and a string view.
    friend auto operator==(String a, std::string_view b) -> bool { return a.view() == b; }
    //! Equality compare a string view and a string.
    friend auto operator==(std::string_view a, String b) -> bool { return a == b.view(); }

    //! Less than compare two strings.
    friend auto operator<=>(String a, String b) -> std::strong_ordering { return a.view() <=> b.view(); }
    //! Less than compare two strings.
    friend auto operator<=>(std::string_view a, String b) -> std::strong_ordering { return a <=> b.view(); }
    //! Less than compare two strings.
    friend auto operator<=>(String a, std::string_view b) -> std::strong_ordering { return a.view() <=> b; }

    //! Output the given string.
    friend auto operator<<(std::ostream &out, String const &str) -> std::ostream &;

  private:
    constexpr String(uintptr_t rep) noexcept : rep_{rep} {}
    uintptr_t rep_ = 0;
};

//! A set of strings.
using StringSet = Util::unordered_set<String>;
//! A vector of strings.
using StringVec = std::vector<String>;
//! A span of strings.
using StringSpan = std::span<String const>;

//! Class managing the lifetime of a String.
//!
//! References held by this class are not collected by the gc method of the
//! symbol store.
class SharedString {
  public:
    //! Construct an empty string.
    constexpr SharedString() = default;
    //! Take ownership of the string reference.
    explicit SharedString(String ref, bool acquire = true) noexcept : ref_{ref} {
        if (acquire) {
            acquire_();
        }
    }
    //! Release ownership of the held string reference.
    ~SharedString() { release_(); }
    //! Copy constructor.
    SharedString(SharedString const &other) noexcept : ref_{other.ref_} { acquire_(); }
    //! Move constructor.
    SharedString(SharedString &&other) noexcept { std::swap(other.ref_, ref_); }
    //! Copy assignment.
    auto operator=(SharedString const &other) noexcept -> SharedString & {
        if (&other != this) {
            release_();
            ref_ = other.ref_;
            acquire_();
        }
        return *this;
    }
    //! Move assignment.
    auto operator=(SharedString &&other) noexcept -> SharedString & {
        if (&other != this) {
            release_();
            ref_ = other.ref_;
            other.ref_ = String::from_rep(0);
        }
        return *this;
    }
    //! Get the contained string reference.
    [[nodiscard]] auto get() const -> String const & { return ref_; }
    //! Get the contained string reference.
    [[nodiscard]] auto operator*() const -> String const & { return ref_; }
    //! Get the contained string reference.
    [[nodiscard]] auto operator->() const -> String const * { return &ref_; }
    //! Get an integer representation of the string.
    //!
    //! To correctly free the string, it has to passed to from_rep again.
    [[nodiscard]] static auto to_rep(SharedString sym) -> uint64_t {
        auto ret = sym.ref_;
        sym.ref_ = String{};
        return String::to_rep(ret);
    }
    //! Construct a string from its representation.
    [[nodiscard]] static auto from_rep(uint64_t repr) -> SharedString {
        auto ret = SharedString();
        ret.ref_ = String::from_rep(repr);
        return ret;
    }

    //! Compute the hash of the string.
    [[nodiscard]] auto hash() const -> size_t { return Gringo::Util::value_hash(ref_); }

    //! Equality compare two strings.
    friend auto operator==(SharedString const &a, SharedString const &b) -> bool { return a.ref_ == b.get(); }

    //! Less than compare two strings.
    friend auto operator<=>(SharedString const &a, SharedString const &b) -> std::strong_ordering {
        return a.get() <=> b.get();
    }

  private:
    void acquire_() const noexcept;
    void release_() const noexcept;

    String ref_;
};

//! A vector of strings.
using SharedStringVec = std::vector<SharedString>;
//! An array of strings.
using SharedStringArray = Util::immutable_array<SharedString>;
//! A vector of strings.
using SharedStringSpan = std::span<SharedString const>;

//! Convert a shared string pointer into a string pointer.
inline auto as_string_ptr(SharedString const *ptr) -> String const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<String const *>(ptr);
}

//! Convert a collection of shared strings into a string span.
template <class T> inline auto as_string_span(T const &vec) -> StringSpan {
    return {as_string_ptr(vec.data()), vec.size()};
}

//! Convert a string pointer into a shared string pointer.
inline auto as_shared_string_ptr(String const *ptr) -> SharedString const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<SharedString const *>(ptr);
}

//! Convert a collection of strings into a shared string span.
template <class T> inline auto as_shared_string_span(T const &vec) -> SharedStringSpan {
    return {as_shared_string_ptr(vec.data()), vec.size()};
}

//! Enumeration of available symbols types.
//!
//! See the documentation of the corresponding functions in the SymbolStore.
enum class SymbolType : uint8_t { number, sup, inf, string, tuple, function };

class Symbol;
//! A span of symbols.
using SymbolSpan = std::span<Symbol const>;
//! A vector of symbols.
using SymbolVec = std::vector<Symbol>;

//! Variant-like class to store symbols stored in a symbol store.
class Symbol {
  public:
    //! Create a reference to number zero.
    //!
    //! Number zero can exist independently of a symbol store.
    constexpr Symbol() = default;
    //! Get the type of the symbol.
    [[nodiscard]] auto type() const noexcept -> SymbolType;
    //! Get the numeric value of the symbol.
    [[nodiscard]] auto num() const noexcept -> Number const &;
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

    //! Compute the hash of the symbol.
    [[nodiscard]] auto hash() const -> size_t { return Gringo::Util::value_hash(rep_); }

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
    uint64_t rep_ = 0;
};

//! Class managing the lifetime of a Symbol.
//!
//! References held by this class are not collected by the gc method of the
//! symbol store.
class SharedSymbol {
  public:
    //! Create a symbol for number zero.
    //!
    //! Number zero can exist independently of a symbol store.
    constexpr SharedSymbol() noexcept = default;
    //! Take ownership of the symbol.
    explicit SharedSymbol(Symbol sym, bool acquire = true) noexcept : ref_{sym} {
        if (acquire) {
            acquire_();
        }
    }
    //! Release ownership of the held symbol.
    ~SharedSymbol() { release_(); }
    //! Copy constructor.
    SharedSymbol(SharedSymbol const &sym) noexcept : ref_{sym.ref_} { acquire_(); }
    //! Move constructor.
    SharedSymbol(SharedSymbol &&sym) noexcept { std::swap(sym.ref_, ref_); }
    //! Copy assignment.
    auto operator=(SharedSymbol const &sym) noexcept -> SharedSymbol & {
        if (&sym != this) {
            release_();
            ref_ = sym.ref_;
            acquire_();
        }
        return *this;
    }
    //! Move assignment.
    auto operator=(SharedSymbol &&sym) noexcept -> SharedSymbol & {
        if (&sym != this) {
            release_();
            ref_ = sym.ref_;
            sym.ref_ = Symbol::from_rep(0);
        }
        return *this;
    }
    //! Get a reference to the contained symbol.
    //!
    //! The lifetime is tied to the symbol.
    [[nodiscard]] auto get() const -> Symbol const & { return ref_; }
    //! Get the contained string reference.
    [[nodiscard]] auto operator*() const -> Symbol const & { return ref_; }
    //! Get the contained string reference.
    [[nodiscard]] auto operator->() const -> Symbol const * { return &ref_; }
    //! Get an integer representation of the symbol.
    //!
    //! The representation increments the reference count of the symbol.
    //! To correctly free the symbol, it has to passed to from_rep again.
    [[nodiscard]] static auto to_rep(SharedSymbol const &sym) -> uint64_t {
        sym.acquire_();
        return Symbol::to_rep(sym.ref_);
    }
    //! Create a shared symbol from its representation.
    //!
    //! No reference counts are touched here.
    [[nodiscard]] static auto from_rep(uint64_t repr) -> SharedSymbol {
        auto ret = SharedSymbol();
        ret.ref_ = Symbol::from_rep(repr);
        return ret;
    }

    //! Compute the hash of the symbol.
    [[nodiscard]] auto hash() const -> size_t { return Gringo::Util::value_hash(ref_); }

    //! Compare two symbols.
    friend auto compare(SharedSymbol const &a, SharedSymbol const &b) -> int { return compare(a.get(), b.get()); }

    //! Equality compare two symbols.
    friend auto operator==(SharedSymbol const &a, SharedSymbol const &b) -> bool { return a.get() == b.get(); }

    //! Less than compare two symbols.
    friend auto operator<=>(SharedSymbol const &a, SharedSymbol const &b) -> std::strong_ordering {
        return compare(a, b) <=> 0;
    }

  private:
    void acquire_() const noexcept;
    void release_() const noexcept;
    Symbol ref_ = Symbol::from_rep(0);
};

//! A span of symbols.
using SharedSymbolSpan = std::span<SharedSymbol const>;
//! A vector of symbols.
using SharedSymbolVec = std::vector<SharedSymbol>;

//! Convert a shared symbol pointer into a symbol pointer.
inline auto as_symbol_ptr(SharedSymbol const *ptr) -> Symbol const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<Symbol const *>(ptr);
}

//! Convert a shared symbol collection into a symbol span.
template <class T> inline auto as_symbol_span(T const &vec) -> SymbolSpan {
    return {as_symbol_ptr(vec.data()), vec.size()};
}

//! Convert a symbol pointer into a shared symbol pointer.
inline auto as_shared_symbol_ptr(Symbol const *ptr) -> SharedSymbol const * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<SharedSymbol const *>(ptr);
}

//! Convert a symbol collection into a shared symbol span.
template <class T> inline auto as_shared_symbol_span(T const &vec) -> SharedSymbolSpan {
    return {as_shared_symbol_ptr(vec.data()), vec.size()};
}

//! Helper class to mark owned symbols.
class SymbolCollector {
  public:
    //! Mark a symbol and its descendants.
    void mark(Symbol const &sym);
    //! Mark a string.
    void mark(String const &str);

  private:
    std::vector<Symbol> stack_;
};

//! Interface for classes owning references to symbols.
class SymbolOwner {
  public:
    //! Destroy the symbol owner.
    virtual ~SymbolOwner() = default;
    //! Function called to mark all owned symbols.
    virtual void mark(SymbolCollector &gc) const = 0;
};

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
    [[nodiscard]] auto string(std::string_view str) -> SharedString;

    //! Construct the infimum constant (<tt>\#inf</tt>).
    [[nodiscard]] static auto sup() noexcept -> Symbol;
    //! Construct the supremum constant (<tt>\#sup</tt>).
    [[nodiscard]] static auto inf() noexcept -> Symbol;
    //! Construct a quoted string.
    //
    //! A raw string is stored and quoted when the symbol is output.
    //! For example: <tt>"foo\nbar"</tt>.
    [[nodiscard]] static auto str(SharedString str) noexcept -> SharedSymbol;
    //! @copydoc str(SharedString)
    [[nodiscard]] static auto str(String str) noexcept -> SharedSymbol;
    //! Construct a number.
    [[nodiscard]] auto num(Number num) noexcept -> SharedSymbol;
    //! Construct a tuple.
    //!
    //! For example: <tt>(x,y)</tt>.
    [[nodiscard]] auto tup(SharedSymbolSpan args) -> SharedSymbol;
    //! @copydoc tup(SharedSymbolSpan)
    [[nodiscard]] auto tup(SymbolSpan args) -> SharedSymbol;
    //! Construct a function symbol.
    //!
    //! For example: <tt>f(x,y)</tt>.
    [[nodiscard]] auto fun(SharedString const &name, SharedSymbolSpan args, bool sign) -> SharedSymbol;
    //! @copydoc fun(SharedString const &, SharedSymbolSpan, bool)
    [[nodiscard]] auto fun(String name, SymbolSpan args, bool sign) -> SharedSymbol;

    // interface to create floating references

    //! Construct a string.
    //!
    //! The string is stored as is.
    [[nodiscard]] auto string_ref(std::string_view str) -> String;

    //! Construct a quoted string.
    //!
    //! A raw string is stored and quoted when the symbol is output.
    //! For example: <tt>"foo\nbar"</tt>.
    [[nodiscard]] static auto str_ref(String str) noexcept -> Symbol;
    //! Construct a number (e.g., <tt>42</tt>).
    [[nodiscard]] auto num_ref(Number num) noexcept -> Symbol;
    //! Construct a tuple.
    //!
    //! For example: <tt>(x,y)</tt>.
    [[nodiscard]] auto tup_ref(SymbolSpan args) -> Symbol;
    //! Construct a function symbol.
    //!
    //! For example: <tt>f(x,y)</tt>.
    [[nodiscard]] auto fun_ref(String name, SymbolSpan args, bool sign) -> Symbol;

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
    auto gc() -> std::pair<size_t, size_t> { return do_gc(); }

  private:
    [[nodiscard]] virtual auto do_tup(SymbolSpan args, bool referenced) -> Symbol = 0;
    [[nodiscard]] virtual auto do_fun(String str, SymbolSpan args, bool sign, bool referenced) -> Symbol = 0;
    [[nodiscard]] virtual auto do_string(std::string_view str, bool referenced) -> String = 0;
    [[nodiscard]] virtual auto do_num(Number num) noexcept -> Symbol = 0;

    virtual void do_gc_block(bool block) noexcept = 0;
    virtual void do_gc_add_owner(SymbolOwner const &owner) = 0;
    virtual void do_gc_del_owner(SymbolOwner const &owner) noexcept = 0;
    virtual auto do_gc() -> std::pair<size_t, size_t> = 0;
};

//! A pointer to a symbol store.
using USymbolStore = std::unique_ptr<SymbolStore>;

//! Helper to block garbage collection.
class GCLock {
  public:
    //! Block garbage collection.
    GCLock(SymbolStore &store) : store_{&store} { store_->gc_block(); }
    //! Unblock garbage collection.
    ~GCLock() { store_->gc_unblock(); }

  private:
    SymbolStore *store_;
};

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
