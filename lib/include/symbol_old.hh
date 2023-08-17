#pragma once

#include <shared_mutex>
#include <string>
#include <tsl/hopscotch_set.h>
#include <util/hash.hh>

namespace Gringo {

template <class Key, class Hash = Util::value_hasher<Key>, class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<Key>, unsigned int NeighborhoodSize = 62, bool StoreHash = false> // NOLINT
using hash_set = tsl::hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash>;

class SymbolStore;

class String {
  public:
    [[nodiscard]] auto c_str() const -> const char *;
    [[nodiscard]] auto view() const -> std::string_view;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto size() const -> size_t;
    [[nodiscard]] auto starts_with(char const *prefix) const -> bool;
    [[nodiscard]] auto hash() const -> size_t;
    friend auto to_rep(String s) noexcept -> uintptr_t;
    friend auto from_rep(uintptr_t t) noexcept -> String;
    friend struct std::hash<String>;
    friend auto operator==(String a, String b) -> bool { return a.impl_ == b.impl_; }
    friend auto operator==(String a, std::string_view b) -> bool { return a.view() == b; }
    friend auto operator==(std::string_view a, String b) -> bool { return a == b.view(); }

  private:
    friend class SymbolStore;
    struct Impl;

    String(Impl *impl) noexcept;
    Impl *impl_;
};

} // namespace Gringo

namespace std {

template <> struct std::hash<Gringo::String> {
    auto operator()(Gringo::String str) const -> size_t;
};

} // namespace std

namespace Gringo {

class SymbolStore {
  public:
    [[nodiscard]] auto string(std::string_view str) -> String;
    void destroy(String str) noexcept;
    ~SymbolStore() noexcept;

  private:
    mutable std::shared_mutex mut_strings_;
    hash_set<String> strings_;
};

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
*/

} // namespace Gringo
