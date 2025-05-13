#pragma once

#include <clingo/core/number.hh>

#include <clingo/util/hash.hh>
#include <clingo/util/print.hh>
#include <clingo/util/unordered_set.hh>

#include <ostream>
#include <span>

namespace CppClingo {

//! @addtogroup core_symbol
//! @{

//! Reference to a string stored in a symbol store.
class String {
  public:
    //! Construct an empty string.
    //!
    //! Empty strings exist independently of symbol stores.
    constexpr String() = default;

    //! Manually increment the reference count of the string.
    void acquire() const noexcept;
    //! Manually decrement the reference count of the string.
    void release() const noexcept;

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
    //! Get the underlying character array.
    //!
    //! The lifetime is tied to that of the reference.
    [[nodiscard]] auto data() const -> const char *;
    //! Test if the string is empty.
    [[nodiscard]] auto empty() const -> bool;
    //! Get the length of the string.
    [[nodiscard]] auto size() const -> size_t;
    //! Check if the string starts with the given string.
    [[nodiscard]] auto starts_with(std::string_view prefix) const -> bool;

    //! Compute the hash of the string.
    [[nodiscard]] auto hash() const -> size_t;

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

    //! Append the given string to the buffer.
    friend auto operator<<(Util::OutputBuffer &out, String const &str) -> Util::OutputBuffer &;

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
            ref.acquire();
        }
    }
    //! Release ownership of the held string reference.
    ~SharedString() { ref_.release(); }
    //! Copy constructor.
    SharedString(SharedString const &other) noexcept : ref_{other.ref_} { ref_.acquire(); }
    //! Move constructor.
    SharedString(SharedString &&other) noexcept { std::swap(other.ref_, ref_); }
    //! Copy assignment.
    auto operator=(SharedString const &other) noexcept -> SharedString & {
        if (&other != this) {
            ref_.release();
            ref_ = other.ref_;
            ref_.acquire();
        }
        return *this;
    }
    //! Move assignment.
    auto operator=(SharedString &&other) noexcept -> SharedString & {
        if (&other != this) {
            ref_.release();
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
        auto ret = std::exchange(sym.ref_, String{});
        return String::to_rep(ret);
    }
    //! Construct a string from its representation.
    [[nodiscard]] static auto from_rep(uint64_t repr) -> SharedString {
        return SharedString{String::from_rep(repr), false};
    }

    //! Compute the hash of the string.
    [[nodiscard]] auto hash() const -> size_t { return CppClingo::Util::value_hash(ref_); }

    //! Equality compare two strings.
    friend auto operator==(SharedString const &a, SharedString const &b) -> bool { return a.ref_ == b.get(); }
    //! Equality compare two strings.
    friend auto operator==(SharedString const &a, String const &b) -> bool { return a.ref_ == b; }
    //! Equality compare two strings.
    friend auto operator==(String const &a, SharedString const &b) -> bool { return a == b.get(); }
    //! Equality compare two strings.
    friend auto operator==(std::string_view a, SharedString const &b) -> bool { return a == b->view(); }
    //! Equality compare two strings.
    friend auto operator==(SharedString const &a, std::string_view b) -> bool { return a->view() == b; }

    //! Compare two strings.
    friend auto operator<=>(SharedString const &a, SharedString const &b) -> std::strong_ordering {
        return a.get() <=> b.get();
    }
    //! Compare two strings.
    friend auto operator<=>(SharedString const &a, String const &b) -> std::strong_ordering { return a.get() <=> b; }
    //! Compare two strings.
    friend auto operator<=>(String const &a, SharedString const &b) -> std::strong_ordering { return a <=> b.get(); }
    //! Equality compare two strings.
    friend auto operator<=>(std::string_view a, SharedString const &b) -> std::strong_ordering {
        return a <=> b->view();
    }
    //! Equality compare two strings.
    friend auto operator<=>(SharedString const &a, std::string_view b) -> std::strong_ordering {
        return a->view() <=> b;
    }

  private:
    String ref_;
};

//! A set of strings.
using SharedStringSet = Util::unordered_set<SharedString>;
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
//! Assignment mapping variables to symbols.
using Assignment = std::vector<std::optional<Symbol>>;

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
    [[nodiscard]] auto hash() const -> size_t { return CppClingo::Util::value_hash(rep_); }

    //! Get the signature of the symbol.
    //!
    //! Returns `std::nullopt` if the symbol is not a function.
    [[nodiscard]] auto signature() const -> std::optional<std::tuple<String, size_t, bool>>;

    //! Compare two symbols.
    friend auto compare(Symbol const &a, Symbol const &b) -> int;
    //! Compare two symbols.
    friend auto compare(Number const &a, Symbol const &b) -> int;
    //! Compare two symbols.
    friend auto compare(Symbol const &a, Number const &b) -> int;

    //! Equality compare two symbols.
    friend auto operator==(Symbol const &a, Symbol const &b) -> bool { return a.rep_ == b.rep_; }
    //! Equality compare numbers and symbols.
    friend auto operator==(Number const &a, Symbol const &b) -> bool { return Number::to_repr(a) == b.rep_; }
    //! Equality compare numbers and symbols.
    friend auto operator==(Symbol const &a, Number const &b) -> bool { return a.rep_ == Number::to_repr(b); }

    //! Compare two symbols.
    friend auto operator<=>(Symbol const &a, Symbol const &b) -> std::strong_ordering { return compare(a, b) <=> 0; }
    //! Compare symbols and numbers.
    friend auto operator<=>(Number const &a, Symbol const &b) -> std::strong_ordering { return compare(a, b) <=> 0; }
    //! Compare symbols and numbers.
    friend auto operator<=>(Symbol const &a, Number const &b) -> std::strong_ordering { return compare(a, b) <=> 0; }

    //! Get the representation of the symbol.
    static auto to_rep(Symbol sym) noexcept -> uint64_t { return sym.rep_; }
    //! Create a symbol from its representation.
    static auto from_rep(uint64_t rep) noexcept -> Symbol { return Symbol{rep}; }

    //! Manually increment the reference count of the symbol.
    void acquire() const noexcept;
    //! Manually decrement the reference count of the symbol.
    void release() const noexcept;

    //! Output the given symbol.
    friend auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream &;

    //! Output the symbol to the given buffer.
    friend auto operator<<(Util::OutputBuffer &out, Symbol const &sym) -> Util::OutputBuffer &;

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
            ref_.acquire();
        }
    }
    //! Release ownership of the held symbol.
    ~SharedSymbol() { ref_.release(); }
    //! Copy constructor.
    SharedSymbol(SharedSymbol const &sym) noexcept : ref_{sym.ref_} { ref_.acquire(); }
    //! Move constructor.
    SharedSymbol(SharedSymbol &&sym) noexcept { std::swap(sym.ref_, ref_); }
    //! Copy assignment.
    auto operator=(SharedSymbol const &sym) noexcept -> SharedSymbol & {
        if (&sym != this) {
            ref_.release();
            ref_ = sym.ref_;
            ref_.acquire();
        }
        return *this;
    }
    //! Move assignment.
    auto operator=(SharedSymbol &&sym) noexcept -> SharedSymbol & {
        if (&sym != this) {
            ref_.release();
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
        sym.ref_.acquire();
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
    [[nodiscard]] auto hash() const -> size_t { return CppClingo::Util::value_hash(ref_); }

    //! Compare two symbols.
    friend auto compare(SharedSymbol const &a, SharedSymbol const &b) -> int { return compare(a.get(), b.get()); }

    //! Equality compare two symbols.
    friend auto operator==(SharedSymbol const &a, SharedSymbol const &b) -> bool { return a.get() == b.get(); }

    //! Equality compare two symbols.
    friend auto operator==(SharedSymbol const &a, Symbol const &b) -> bool { return a.get() == b; }

    //! Equality compare two symbols.
    friend auto operator==(Symbol const &a, SharedSymbol const &b) -> bool { return a == b.get(); }

    //! Less than compare two symbols.
    friend auto operator<=>(SharedSymbol const &a, SharedSymbol const &b) -> std::strong_ordering {
        return compare(a, b) <=> 0;
    }

    //! Less than compare two symbols.
    friend auto operator<=>(SharedSymbol const &a, Symbol const &b) -> std::strong_ordering {
        return compare(*a, b) <=> 0;
    }

    //! Less than compare two symbols.
    friend auto operator<=>(Symbol const &a, SharedSymbol const &b) -> std::strong_ordering {
        return compare(a, *b) <=> 0;
    }

  private:
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
    //! Construct a number.
    [[nodiscard]] static auto num(int32_t num) noexcept -> SharedSymbol;
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
    //! Construct a number.
    [[nodiscard]] static auto num_ref(int32_t num) noexcept -> Symbol;
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
    //!
    //! Returns the number of symbols owners, the number of symbols with
    //! reference count greater 0, and the number of symbols collected.
    auto gc() -> std::tuple<size_t, size_t, size_t> { return do_gc(); }

  private:
    [[nodiscard]] virtual auto do_tup(SymbolSpan args, bool referenced) -> Symbol = 0;
    [[nodiscard]] virtual auto do_fun(String str, SymbolSpan args, bool sign, bool referenced) -> Symbol = 0;
    [[nodiscard]] virtual auto do_string(std::string_view str, bool referenced) -> String = 0;
    [[nodiscard]] virtual auto do_num(Number num) noexcept -> Symbol = 0;

    virtual void do_gc_block(bool block) noexcept = 0;
    virtual void do_gc_add_owner(SymbolOwner const &owner) = 0;
    virtual void do_gc_del_owner(SymbolOwner const &owner) noexcept = 0;
    virtual auto do_gc() -> std::tuple<size_t, size_t, size_t> = 0;
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
//! set up and returned.
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
        : store_{store}, names_{names.begin(), names.end()}, prefix_{prefix} {}
    //! Delete move/copy constructor.
    NameGen(NameGen &&) noexcept = delete;
    //! Initialize/reset the name generator.
    void init(StringSet names, char const *prefix) {
        num_ = 0;
        names_.insert(names.begin(), names.end());
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
    SharedStringSet names_;
    //! The prefix of the generated names.
    char const *prefix_;
    //! Running number used to generate names.
    size_t num_ = 0;
};

//! @}

} // namespace CppClingo
