#pragma once

#include <span>
#include <string>
#include <util/hash.hh>

namespace Gringo {

//! The supremum and infimum constants.
enum class Constant : int {
    supremum, //!< The supremum (<tt>\#sup</tt>).
    infimum,  //!< The infimum (<tt>\#inf</tt>).
};

class String {
  public:
    [[nodiscard]] auto c_str() const -> const char *;
    [[nodiscard]] auto view() const -> std::string_view;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto starts_with(char const *prefix) const -> bool;
    [[nodiscard]] auto hash() const -> size_t;

    friend auto operator==(String a, String b) -> bool { return a.rep_ == b.rep_; }
    friend auto operator==(String a, std::string_view b) -> bool { return a.view() == b; }
    friend auto operator==(std::string_view a, String b) -> bool { return a == b.view(); }

    static auto to_rep(String str) noexcept -> uint64_t { return str.rep_; }
    static auto from_rep(uint64_t rep) noexcept -> String { return String{rep}; }

  private:
    String(uintptr_t rep) noexcept : rep_{rep} {}
    uintptr_t rep_;
};

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

    friend auto operator==(Symbol a, Symbol b) -> bool { return a.rep_ == b.rep_; }
    friend auto operator!=(Symbol a, Symbol b) -> bool { return a.rep_ != b.rep_; }

    static auto to_rep(Symbol sym) noexcept -> uint64_t { return sym.rep_; }
    static auto from_rep(uint64_t rep) noexcept -> Symbol { return Symbol{rep}; }

  private:
    Symbol(uint64_t repr) noexcept : rep_{repr} {}
    uint64_t rep_;
};

class SymbolStore {
  public:
    [[nodiscard]] static auto sup() noexcept -> Symbol;
    [[nodiscard]] static auto inf() noexcept -> Symbol;
    [[nodiscard]] static auto number(int32_t num) noexcept -> Symbol;
    [[nodiscard]] static auto string(String str) noexcept -> Symbol;
    [[nodiscard]] virtual auto tuple(SymbolSpan args) -> Symbol = 0;
    [[nodiscard]] virtual auto function(String str, SymbolSpan args) -> Symbol = 0;
    [[nodiscard]] virtual auto string(std::string_view str) -> String = 0;
    virtual ~SymbolStore() noexcept = default;
};

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
auto make_symbol_store(bool shared) -> USymbolStore;

} // namespace Gringo

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

/*

inline bool operator==(String a, String b) { return std::strcmp(a.c_str(), b.c_str()) == 0; }
inline bool operator!=(String a, String b) { return std::strcmp(a.c_str(), b.c_str()) != 0; }
inline bool operator< (String a, String b) { return std::strcmp(a.c_str(), b.c_str()) <  0; }
inline bool operator> (String a, String b) { return std::strcmp(a.c_str(), b.c_str()) >  0; }
inline bool operator<=(String a, String b) { return std::strcmp(a.c_str(), b.c_str()) <= 0; }
inline bool operator>=(String a, String b) { return std::strcmp(a.c_str(), b.c_str()) >= 0; }

inline bool operator==(String a, char const *b) { return std::strcmp(a.c_str(), b) == 0; }
inline bool operator!=(String a, char const *b) { return std::strcmp(a.c_str(), b) != 0; }
inline bool operator< (String a, char const *b) { return std::strcmp(a.c_str(), b) <  0; }
inline bool operator> (String a, char const *b) { return std::strcmp(a.c_str(), b) >  0; }
inline bool operator<=(String a, char const *b) { return std::strcmp(a.c_str(), b) <= 0; }
inline bool operator>=(String a, char const *b) { return std::strcmp(a.c_str(), b) >= 0; }

inline bool operator==(char const *a, String b) { return std::strcmp(a, b.c_str()) == 0; }
inline bool operator!=(char const *a, String b) { return std::strcmp(a, b.c_str()) != 0; }
inline bool operator< (char const *a, String b) { return std::strcmp(a, b.c_str()) <  0; }
inline bool operator> (char const *a, String b) { return std::strcmp(a, b.c_str()) >  0; }
inline bool operator<=(char const *a, String b) { return std::strcmp(a, b.c_str()) <= 0; }
inline bool operator>=(char const *a, String b) { return std::strcmp(a, b.c_str()) >= 0; }

inline std::ostream &operator<<(std::ostream &out, String x) {
    out << x.c_str();
    return out;
}

// {{{1 declaration of Signature (flyweight)

class Sig {
public:
    Sig(String name, uint32_t arity, bool sign);
    explicit Sig(uint64_t rep) : rep_(rep) {  }

    String name() const;
    Sig flipSign() const;
    uint32_t arity() const;
    bool sign() const;
    size_t hash() const;
    uint64_t const &rep() const { return rep_; }
    bool match(String n, uint32_t a, bool s = false) const {
        return name() == n && arity() == a && sign() == s;
    }
    bool match(char const *n, uint32_t a, bool s = false) const {
        return name() == n && arity() == a && sign() == s;
    }
    bool operator==(Sig s) const;
    bool operator!=(Sig s) const;
    bool operator<(Sig s) const;
    bool operator>(Sig s) const;
    bool operator<=(Sig s) const;
    bool operator>=(Sig s) const;

private:
    uint64_t rep_;
};

inline std::ostream &operator<<(std::ostream &out, Sig x) {
    if (x.sign()) { out << "-"; }
    out << x.name() << "/" << x.arity();
    return out;
}

// {{{1 declaration of Symbol (flyweight)

enum class SymbolType : uint8_t {
    Inf     = 0,
    Num     = 1,
    Str     = 4,
    Fun     = 5,
    Special = 6,
    Sup     = 7
};

inline std::ostream &operator<<(std::ostream &out, SymbolType sym) {
    switch (sym) {
        case SymbolType::Inf: { out << "Inf"; break; }
        case SymbolType::Num: { out << "Num"; break; }
        case SymbolType::Str: { out << "Str"; break; }
        case SymbolType::Fun: { out << "Fun"; break; }
        case SymbolType::Special: { out << "Special"; break; }
        case SymbolType::Sup: { out << "Sup"; break; }
    }
    return out;
}

class Symbol;
using SymVec = std::vector<Symbol>;
using SymSpan = Potassco::Span<Symbol>;
using IdSymMap = std::unordered_map<String, Symbol>;

class Symbol {
public:
    // construction
    Symbol(); // createSpecial
    explicit Symbol(uint64_t sym) : rep_(sym) { };
    static Symbol createId(String val, bool sign = false);
    static Symbol createStr(String val);
    static Symbol createNum(int num);
    static Symbol createInf();
    static Symbol createSup();
    static Symbol createTuple(SymSpan args);
    static Symbol createTuple(SymVec const &args) {
        return createTuple(Potassco::toSpan(args));
    }
    static Symbol createFun(String name, SymSpan args, bool sign = false);
    static Symbol createFun(String name, SymVec const &args, bool sign = false) {
        return createFun(name, Potassco::toSpan(args), sign);
    }

    // value retrieval
    SymbolType type() const;
    int num() const;
    String string() const;
    Sig sig() const;
    bool hasSig() const;
    uint32_t arity() const { return sig().arity(); }
    String name() const;
    SymSpan args() const;
    bool sign() const;

    // modifying values
    Symbol replace(IdSymMap const &map) const;
    Symbol flipSign() const;

    // comparison
    size_t hash() const;
    bool operator==(Symbol const &other) const;
    bool operator!=(Symbol const &other) const;
    bool operator<(Symbol const &other) const;
    bool operator>(Symbol const &other) const;
    bool operator<=(Symbol const &other) const;
    bool operator>=(Symbol const &other) const;

    // ouput
    void print(std::ostream& out) const;

    uint64_t const &rep () const { return rep_; }
private:
    uint64_t rep_;
};

inline std::ostream& operator<<(std::ostream& out, Symbol sym) {
    sym.print(out);
    return out;
}

// {{{1 definition of quote/unquote

inline std::string quote(StringSpan str) {
    std::string res;
    for (auto c : str) {
        switch (c) {
            case '\n': {
                res.push_back('\\');
                res.push_back('n');
                break;
            }
            case '\\': {
                res.push_back('\\');
                res.push_back('\\');
                break;
            }
            case '"': {
                res.push_back('\\');
                res.push_back('"');
                break;
            }
            default: {
                res.push_back(c);
                break;
            }
        }
    }
    return res;
}
inline std::string quote(char const *str) {
    return quote({str, strlen(str)});
}

inline std::string unquote(StringSpan str) {
    std::string res;
    bool slash = false;
    for (auto c : str) {
        if (slash) {
            switch (c) {
                case 'n': {
                    res.push_back('\n');
                    break;
                }
                case '\\': {
                    res.push_back('\\');
                    break;
                }
                case '"': {
                    res.push_back('"');
                    break;
                }
                default: {
                    assert(false);
                    break;
                }
            }
            slash = false;
        }
        else if (c == '\\') { slash = true; }
        else { res.push_back(c); }
    }
    return res;
}

inline std::string unquote(char const *str) {
    return unquote({str, strlen(str)});
}

// }}}1

} // namespace Gringo

namespace std {

// {{{1 definition of hash functions

template<>
struct hash<Gringo::String> {
    size_t operator()(Gringo::String const &str) const { return str.hash(); }
};

template<>
struct hash<Gringo::Sig> {
    size_t operator()(Gringo::Sig const &sig) const { return sig.hash(); }
};

template<>
struct hash<Gringo::Symbol> {
    size_t operator()(Gringo::Symbol const &sym) const { return sym.hash(); }
};

// }}}1

} // namespace Gringo
*/
