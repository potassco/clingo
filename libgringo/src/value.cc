// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#include <gringo/symbol.hh>
#include <gringo/hash_set.hh>
#include <mutex>

namespace Gringo {

namespace {

// {{{1 auxiliary functions

static constexpr const uint16_t upperMax = std::numeric_limits<uint16_t>::max();
static constexpr const uint16_t lowerMax = 3;
uint16_t upper(uint64_t rep) { return rep >> 48; }
uint8_t lower(uint64_t rep) { return rep & 3; }
uintptr_t ptr(uint64_t rep) { return static_cast<uintptr_t>(rep & 0xFFFFFFFFFFFC); }
uint64_t combine(uint16_t u, uintptr_t ptr, uint8_t l) {
    assert(l <= lowerMax);
    return static_cast<uint64_t>(u) << 48 | ptr | l;
}
uint64_t combine(uint16_t u, int32_t num) { return static_cast<uint64_t>(u) << 48 | static_cast<uint64_t>(static_cast<uint32_t>(num)); }
uint64_t setUpper(uint16_t u, uint64_t rep) { return combine(u, 0, 0) | (rep & 0xFFFFFFFFFFFF); }
uint64_t setLower(uint8_t l, uint8_t rep) {
    assert(l <= lowerMax);
    return combine(0, 0, l) | (rep & 0xFFFFFFFFFFFFFFFC);
}

enum class SymbolType_ : uint8_t {
    Inf = clingo_symbol_type_inf,
    Num = clingo_symbol_type_num,
    IdP = clingo_symbol_type_num+1,
    IdN = clingo_symbol_type_num+2,
    Str = clingo_symbol_type_str,
    Fun = clingo_symbol_type_fun,
    Special = clingo_symbol_type_fun+1,
    Sup = clingo_symbol_type_sup
};
SymbolType_ symbolType_(uint64_t rep) { return static_cast<SymbolType_>(upper(rep)); }
template <class T>
T const *cast(uint64_t rep) { return reinterpret_cast<T const *>(ptr(rep)); }
String toString(uint64_t rep) { return String::fromRep(ptr(rep)); }

// {{{1 definition of Unique

template <class T>
class Unique {
public:
    using Type = typename T::Type;
    struct Hash {
        size_t operator()(Unique const &s) const { return T::hash(*s.ptr_); }
        template <class U>
        size_t operator()(U const &s) const { return T::hash(s); }
    };
    struct Open { };
    struct Deleted { };
    struct EqualTo {
        bool operator()(Unique const &a, Unique const &b) const { return a.ptr_ == b.ptr_; }
        template <class U>
        bool operator()(Unique const &a, U const &b) const { return T::equal(*a.ptr_, b); }
    };
    struct Literals {
        static constexpr Deleted deleted = {};
        static constexpr Open open = {};
    };
    Unique() = default;
    template <class U>
    Unique(U &&t) : ptr_(T::construct(std::forward<U>(t))) { }
    Unique(Unique &&s) noexcept { std::swap(ptr_, s.ptr_); }
    Unique(Unique const &) = delete;
    Unique &operator=(Unique &&s) noexcept { std::swap(ptr_, s.ptr_); return *this; }
    Unique &operator=(Unique const &) = delete;
    ~Unique() noexcept {
        if (ptr_ && ptr_ != deleted_) { T::destroy(ptr_); }
    }
    Unique &operator=(Open) noexcept {
        this->~Unique();
        ptr_ = nullptr;
        return *this;
    }
    Unique &operator=(Deleted) noexcept {
        this->~Unique();
        ptr_ = deleted_;
        return *this;
    }
    bool operator==(Open) const { return ptr_ == nullptr; }
    bool operator==(Deleted) const { return ptr_ == deleted_; }
    template <class U>
    static Type const *encode(U &&x) {
        std::lock_guard<std::mutex> g(mutex_);
        return set_.insert(Hash(), EqualTo(), std::forward<U>(x)).first.ptr_;
    }
private:
    using Set = HashSet<Unique, Literals>;
    static Set set_;
    static std::mutex mutex_;
    static Type const *deleted_;
    Type *ptr_ = nullptr;

};
template <class T>
constexpr typename Unique<T>::Deleted Unique<T>::Literals::deleted;
template <class T>
constexpr typename Unique<T>::Open Unique<T>::Literals::open;
template <class T>
typename Unique<T>::Set Unique<T>::set_;
template <class T>
std::mutex Unique<T>::mutex_;
// NOTE: this is just a sentinel address that is never malloced and never dereferenced
template <class T>
typename Unique<T>::Type const *Unique<T>::deleted_ = reinterpret_cast<typename Unique<T>::Type const *>(&Unique<T>::deleted_);


// {{{1 definition of UString

struct MString {
    using Type = char;
    static size_t hash(char const &str) { return strhash(&str); }
    static size_t hash(StringSpan str) { return strhash(str); }
    static bool equal(char const &a, char const &b) { return std::strcmp(&a, &b) == 0; }
    static bool equal(char const &a, StringSpan b) { return std::strncmp(&a, b.first, b.size) == 0 && (&a)[b.size] == '\0'; }
    static char *construct(char const &str) {
        std::unique_ptr<char[]> buf{new char[std::strlen(&str) + 1]};
        std::strcpy(buf.get(), &str);
        return buf.release();
    }
    static char *construct(StringSpan str) {
        std::unique_ptr<char[]> buf{new char[str.size + 1]};
        std::memcpy(buf.get(), str.first, sizeof(char) * str.size);
        buf[str.size] = '\0';
        return buf.release();
    }
    static void destroy(char *str) { delete [] str; }
};

using UString = Unique<MString>;

// {{{1 definition of USig

struct MSig {
    using Type = std::pair<String, uint32_t>;
    static size_t hash(Type const &sig) { return get_value_hash(sig); }
    static bool equal(Type const &a, Type const &b) { return a == b; }
    static Type *construct(Type const &sig) { return gringo_make_unique<Type>(sig).release(); }
    static void destroy(Type *sig) { delete sig; }
};
using USig = Unique<MSig>;
uint64_t encodeSig(String name, uint32_t arity, bool sign) {
    return arity < upperMax
        ? combine(arity, String::toRep(name), sign)
        : combine(upperMax, reinterpret_cast<uintptr_t>(USig::encode(MSig::Type{name, arity})), sign);
}

// {{{1 definition of Fun

class Fun {
public:
    Sig sig() const {
        return sig_;
    }
    SymSpan args() const {
        return {args_, sig().arity()};
    }
    static Fun *make(Sig sig, SymSpan args) {
        auto *mem = ::operator new(sizeof(Fun) + args.size * sizeof(Symbol));
        return new(mem) Fun(sig, args);
    }
    bool equal(Sig sig, SymSpan args) const {
        return sig_ == sig && std::equal(begin(args), end(args), args_);
    }
    static size_t hash(Sig sig, SymSpan args) {
        return get_value_hash(sig, hash_range(begin(args), end(args)));
    }
    void destroy() noexcept {
        this->~Fun();
        ::operator delete(this);
    }
private:
    ~Fun() noexcept = default;
    Fun(Sig sig, SymSpan args) noexcept
    : sig_(sig) {
        std::memcpy(static_cast<void*>(args_), args.first, args.size * sizeof(Symbol));
    }
    Sig const sig_;
    Symbol args_[0];
};

struct MFun {
    using Type = Fun;
    using Cons = std::pair<Sig, SymSpan>;
    static size_t hash(Type const &fun) { return Fun::hash(fun.sig(), fun.args()); }
    static size_t hash(Cons const &fun) { return Fun::hash(fun.first, fun.second); }
    static size_t cons(Type const &fun) { return fun.hash(fun.sig(), fun.args()); }
    static bool equal(Type const &a, Type const &b) { return a.equal(b.sig(), b.args()); }
    static bool equal(Type const &a, Cons const &b) { return a.equal(b.first, b.second); }
    static Type *construct(Cons fun) { return Fun::make(fun.first, fun.second); }
    static void destroy(Type *fun) { const_cast<Fun*>(fun)->destroy(); }
};
using UFun = Unique<MFun>;

// }}}1

} // namespace

// {{{1 definition of String

String::String(char const *str)
: str_(UString::encode(*str)) { }

String::String(StringSpan str)
: str_(UString::encode(str)) { }

String::String(uintptr_t r)
: str_(reinterpret_cast<char const *>(r)) { }

bool String::empty() const { return str_[0] == '\0'; }

size_t String::hash() const { return reinterpret_cast<uintptr_t>(str_); }
uintptr_t String::toRep(String s) { return reinterpret_cast<uintptr_t>(s.str_); }
String String::fromRep(uintptr_t t) { return String(t); }

// {{{1 definition of Signature

Sig::Sig(String name, uint32_t arity, bool sign)
: rep_(encodeSig(name, arity, sign)) { }

Sig::Sig(uint64_t rep)
: rep_(rep) { }

String Sig::name() const {
    uint16_t u = upper(rep_);
    return u < upperMax ? toString(rep_) : cast<USig::Type>(rep_)->first;
}

Sig Sig::flipSign() const {
    return Sig(rep_ ^ 1);
}

uint32_t Sig::arity() const {
    uint16_t u = upper(rep_);
    return u < upperMax ? u : cast<USig::Type>(rep_)->second;
}

bool Sig::sign() const { return lower(rep_); }

size_t Sig::hash() const { return get_value_hash(rep_); }

namespace {
bool less(Sig const &a, Sig const &b) {
    if (a.sign() != b.sign()) { return a.sign() < b.sign(); }
    if (a.arity() != b.arity()) { return a.arity() < b.arity(); }
    return a.name() < b.name();
}
}

bool Sig::operator==(Sig s) const { return rep_ == s.rep_; }
bool Sig::operator!=(Sig s) const { return rep_ != s.rep_; }
bool Sig::operator<(Sig s) const { return *this != s && less(*this, s); }
bool Sig::operator>(Sig s) const { return *this != s && less(s, *this); }
bool Sig::operator<=(Sig s) const { return *this == s || less(*this, s); }
bool Sig::operator>=(Sig s) const { return *this == s || less(s, *this); }

// {{{1 definition of Symbol

// {{{2 construction

Symbol::Symbol(clingo_symbol sym)
: clingo_symbol{sym.rep} { }

Symbol::Symbol(uint64_t rep)
: clingo_symbol{rep} { }

Symbol::Symbol()
: Symbol(combine(static_cast<uint16_t>(SymbolType_::Special), 0, 0)) { }

Symbol Symbol::createInf() {
    return Symbol(combine(static_cast<uint16_t>(SymbolType_::Inf), 0, 0));
}

Symbol Symbol::createSup() {
    return Symbol(combine(static_cast<uint16_t>(SymbolType_::Sup), 0, 0));
}

Symbol Symbol::createNum(int num) {
    return Symbol(combine(static_cast<uint16_t>(SymbolType_::Num), num));
}

Symbol Symbol::createId(String val, bool sign) {
    return Symbol(combine(static_cast<uint16_t>(sign ? SymbolType_::IdN : SymbolType_::IdP), String::toRep(val), 0));
}

Symbol Symbol::createStr(String val) {
    return Symbol(combine(static_cast<uint16_t>(SymbolType_::Str), String::toRep(val), 0));
}

Symbol Symbol::createTuple(SymSpan args) {
    return createFun("", args, false);
}

Symbol Symbol::createFun(String name, SymSpan args, bool sign) {
    return args.size != 0
        ?  Symbol(combine(static_cast<uint16_t>(SymbolType_::Fun), reinterpret_cast<uintptr_t>(UFun::encode(std::make_pair(Sig(name, args.size, sign), args))), 0))
        : createId(name, sign);
}

// {{{2 inspection

SymbolType Symbol::type() const {
    auto t = symbolType_(rep);
    switch (t) {
        case SymbolType_::IdP: { return SymbolType::Fun; }
        case SymbolType_::IdN: { return SymbolType::Fun; }
        default:               { return static_cast<SymbolType>(t); }
    }
}

int32_t Symbol::num() const {
    assert(type() == SymbolType::Num);
    return static_cast<int32_t>(static_cast<uint32_t>(rep));
}

String Symbol::string() const {
    assert(type() == SymbolType::Str);
    return toString(rep);
}

Sig Symbol::sig() const{
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep)) {
        case SymbolType_::IdP: { return Sig(toString(rep), 0, false); }
        case SymbolType_::IdN: { return Sig(toString(rep), 0, true); }
        default:               { return cast<Fun>(rep)->sig(); }
    }
}

bool Symbol::hasSig() const {
    return type() == SymbolType::Fun;
}

String Symbol::name() const {
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: { return toString(rep); }
        default:               { return cast<Fun>(rep)->sig().name(); }
    }
}

SymSpan Symbol::args() const {
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: { return SymSpan{nullptr, 0}; }
        default:               { return cast<Fun>(rep)->args(); }
    }
}
bool Symbol::sign() const {
    assert(type() == SymbolType::Fun || type() == SymbolType::Num);
    switch (symbolType_(rep)) {
        case SymbolType_::Num: { return num() < 0; }
        case SymbolType_::IdP: { return false; }
        case SymbolType_::IdN: { return true; }
        default:               { return cast<Fun>(rep)->sig().sign(); }
    }
}

// {{{2 modification

Symbol Symbol::flipSign() const {
    assert(type() == SymbolType::Fun || type() == SymbolType::Num);
    switch (symbolType_(rep)) {
        case SymbolType_::Num: { return Symbol::createNum(-num()); }
        case SymbolType_::IdP: { return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdN), rep)); }
        case SymbolType_::IdN: { return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdP), rep)); }
        default: {
            auto f = cast<Fun>(rep);
            auto s = f->sig();
            return createFun(s.name(), f->args(), !s.sign());
        }
    }
}

Symbol Symbol::replace(IdSymMap const &map) const {
    assert(symbolType_(rep) != SymbolType_::IdN);
    switch(symbolType_(rep)) {
        case SymbolType_::Fun: {
            SymVec vals;
            for (auto &x : args()) { vals.emplace_back(x.replace(map)); }
            return createFun(name(), Potassco::toSpan(vals));
        }
        case SymbolType_::IdP: {
            auto it = map.find(name());
            if (it != map.end()) { return it->second; }
        }
        default: { return *this; }
    }
}

// {{{2 comparison

size_t Symbol::hash() const { return get_value_hash(rep); }

namespace {
bool less(Symbol const &a, Symbol const &b) {
    auto ta = symbolType_(a.rep), tb = symbolType_(b.rep);
    if (ta != tb) { return ta < tb; }
    switch(ta) {
        case SymbolType_::Num: { return a.num() < b.num(); }
        case SymbolType_::IdN:
        case SymbolType_::IdP: { return a.name() < b.name(); }
        case SymbolType_::Str: { return a.string() < b.string(); }
        case SymbolType_::Inf: { return false; }
        case SymbolType_::Sup: { return false; }
        case SymbolType_::Fun: {
            auto sa = a.sig(), sb = b.sig();
            if (sa != sb) { return sa < sb; }
            auto aa = a.args(), ab = b.args();
            return std::lexicographical_compare(begin(aa), end(aa), begin(ab), end(ab));
        }
        case SymbolType_::Special: { return false; }
    }
    assert(false);
    return false;
}
}

bool Symbol::operator==(Symbol const &other) const { return rep == other.rep; }
bool Symbol::operator!=(Symbol const &other) const { return rep != other.rep; }

bool Symbol::operator<(Symbol const &other) const  { return (*this != other) && less(*this, other); }
bool Symbol::operator>(Symbol const &other) const  { return (*this != other) && less(other, *this); }
bool Symbol::operator<=(Symbol const &other) const { return (*this == other) || less(*this, other); }
bool Symbol::operator>=(Symbol const &other) const { return (*this == other) || less(other, *this); }

// {{{2 output

void Symbol::print(std::ostream& out) const {
    switch(symbolType_(rep)) {
        case SymbolType_::Num: { out << num(); break; }
        case SymbolType_::IdN: { out << "-"; }
        case SymbolType_::IdP: {
            char const *n = name().c_str();
            out << (n[0] != '\0' ? n : "()"); break;
        }
        case SymbolType_::Str: { out << '"' << quote(string().c_str()) << '"'; break; }
        case SymbolType_::Inf: { out << "#inf"; break; }
        case SymbolType_::Sup: { out << "#sup"; break; }
        case SymbolType_::Fun: {
            auto s = sig();
            if (s.sign()) { out << "-"; }
            out << s.name();
            auto a = args();
            out << "(";
            if (a.size > 0) {
                std::copy(begin(a), end(a) - 1, std::ostream_iterator<Symbol>(out, ","));
                out << *(end(a) - 1);
            }
            if (a.size == 1 && s.name() == "") {
                out << ",";
            }
            out << ")";
            break;
        }
        case SymbolType_::Special: { out << "#special"; break; }
    }
}

// }}}2

// }}}1

} // namespace Gringo

