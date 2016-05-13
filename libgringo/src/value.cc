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

#include <gringo/value.hh>
#include <mutex>

namespace Gringo {

namespace {

// {{{1 auxiliary functions

template <class F>
auto doLocked(F f) -> decltype(f()) {
    static std::mutex m;
    std::lock_guard<std::mutex> g(m);
    return f();
}

template <class T>
struct Destroy {
    void operator()(T *t) noexcept {
        t->~T();
        ::operator delete(t);
    }
};

static constexpr const uint16_t upperMax = std::numeric_limits<uint16_t>::max();
static constexpr const uint16_t lowerMax = std::numeric_limits<uint8_t>::max();
uint16_t upper(uint64_t rep) { return rep >> 48; }
uint8_t lower(uint64_t rep) { return rep & 7; }
uintptr_t ptr(uint64_t rep) { return reinterpret_cast<uintptr_t>(rep & 0xFFFFFFFFFF00); }
uint64_t combine(uint16_t u, uintptr_t ptr, uint8_t l) { return static_cast<uint64_t>(u) << 48 | ptr | l; }
uint64_t combine(uint16_t u, int32_t num) { return static_cast<uint64_t>(u) << 48 | static_cast<uint64_t>(static_cast<uint32_t>(num)); }
uint64_t setUpper(uint64_t rep) { return combine(rep, 0, 0) | (rep & 0xFFFFFFFFFFFF); }

enum class SymbolType_ : uint8_t {
    Inf = clingo_symbol_type_inf,
    Num = clingo_symbol_type_num,
    IdP = clingo_symbol_type_num+1,
    IdN = clingo_symbol_type_num+2,
    String = clingo_symbol_type_str,
    Func = clingo_symbol_type_fun,
    Special = clingo_symbol_type_fun+1,
    Sup = clingo_symbol_type_sup
};
SymbolType_ symbolType_(uint64_t rep) { return static_cast<SymbolType_>(upper(rep)); }
template <class T>
T const *cast(uint64_t rep) { return reinterpret_cast<T const *>(ptr(rep)); }
String toString(uint64_t rep) { return String::fromRep(ptr(rep)); }

// {{{1 definition of UString

class UString {
public:
    struct Open { };
    struct Deleted { };
    struct Hash {
        size_t operator()(char const *s) const { return strhash(s); }
    };
    struct EqualTo {
        template <class T>
        bool operator()(UString const &a, T const &b) const { return a == b; }
    };
    struct Literals {
        static constexpr UString::Deleted deleted = {};
        static constexpr UString::Open open = {};
    };

    UString() = default;
    UString(char const *str) {
        std::unique_ptr<char[]> buf{new char[std::strlen(str) + 1]};
        std::strcpy(buf.get(), str);
        str_ = buf.release();
    }
    UString(UString &&s) noexcept { std::swap(str_, s.str_); }
    UString(UString const &) = delete;
    UString &operator=(UString &&s) noexcept { std::swap(str_, s.str_); return *this; }
    UString &operator=(UString const &) = delete;
    ~UString() noexcept {
        if (str_ && str_ != &deleted_) { delete [] str_; }
    }
    UString &operator=(Open) noexcept {
        this->~UString();
        str_ = nullptr;
        return *this;
    }
    UString &operator=(Deleted) noexcept {
        this->~UString();
        str_ = &deleted_;
        return *this;
    }
    operator const char *() const { return str_; }
    bool operator==(UString const &s) const { return str_ == s.str_; }
    bool operator==(char const *s) const { return std::strcmp(str_, s) == 0; }
    bool operator==(Open) const { return str_ == nullptr; }
    bool operator==(Deleted) const { return str_ == &deleted_; }
    static char const *encode(char const *str) {
        return doLocked([str]() { return static_cast<char const *>(strings_.insert(Hash(), EqualTo(), str).first); });
    }
private:
    using StringSet = HashSet<UString, UString::Literals>;
    static StringSet strings_;
    static char deleted_;
    char const *str_ = nullptr;
};
char UString::deleted_;
UString::StringSet UString::strings_;

// {{{1 definition of USig

// FIXME: c/p of UString
struct USig {
    using Rep = std::pair<String,uint32_t>;
    struct Open { };
    struct Deleted { };
    struct Hash {
        size_t operator()(USig const &s) const { return get_value_hash(*s.sig_); }
    };
    struct EqualTo {
        template <class T>
        bool operator()(USig const &a, T const &b) const { return a == b; }
    };
    struct Literals {
        static constexpr USig::Deleted deleted = {};
        static constexpr USig::Open open = {};
    };

    USig() = default;
    USig(Rep sig)
    : sig_(gringo_make_unique<Rep>(sig).release()) { }
    USig(USig &&s) noexcept { std::swap(sig_, s.sig_); }
    USig(USig const &) = delete;
    USig &operator=(USig &&s) noexcept { std::swap(sig_, s.sig_); return *this; }
    USig &operator=(USig const &) = delete;
    ~USig() noexcept {
        if (sig_ && sig_ != &deleted_) { delete [] sig_; }
    }
    USig &operator=(Open) noexcept {
        this->~USig();
        sig_ = nullptr;
        return *this;
    }
    USig &operator=(Deleted) noexcept {
        this->~USig();
        sig_ = &deleted_;
        return *this;
    }
    bool operator==(USig const &s) const { return sig_ == s.sig_; }
    bool operator==(Rep s) const { return *sig_ == s; }
    bool operator==(Open) const { return sig_ == nullptr; }
    bool operator==(Deleted) const { return sig_ == &deleted_; }

    static uint64_t encode(String name, uint32_t arity, bool sign) {
        return arity < upperMax
            ? combine(arity, String::toRep(name), sign)
            : combine(upperMax, doLocked([name, arity]() { return reinterpret_cast<uintptr_t>(sigs_.insert(Hash(), EqualTo(), USig({name, arity})).first.sig_); }), sign);
    }
private:
    static Rep deleted_;
    static HashSet<USig, Literals> sigs_;
    Rep *sig_ = nullptr;
};
USig::Rep USig::deleted_ = {nullptr, 0};
HashSet<USig, USig::Literals> USig::sigs_;

// {{{1 definition of Function

class Function {
public:
    using Ptr = std::unique_ptr<Function, Destroy<Function>>;
    Sig sig() const {
        return sig_;
    }
    SymSpan args() const {
        return {args_, sig().arity()};
    }
    static Ptr make(Sig sig, SymSpan args) {
        auto *mem = ::operator new(sizeof(Function) + args.size * sizeof(Symbol));
        return Ptr{new(mem) Function(sig, args)};
    }
    ~Function() noexcept = default;
private:
    Function(Sig sig, SymSpan args) noexcept
    : sig_(sig) {
        std::memcpy(static_cast<void*>(args_), args.first, args.size * sizeof(Symbol));
    }
    Sig const sig_;
    Symbol args_[0];
};

// }}}1

} // namespace

// {{{1 definition of String

String::String(char const *str)
: str_(UString::encode(str)) { }

String::String(uintptr_t r)
: str_(reinterpret_cast<char const *>(r)) { }

size_t String::hash() const { return reinterpret_cast<uintptr_t>(str_); }
uintptr_t String::toRep(String s) { return reinterpret_cast<uintptr_t>(s.str_); }
String String::fromRep(uintptr_t t) { return String(t); }

// {{{1 definition of Signature

Sig::Sig(String name, uint32_t arity, bool sign)
: rep_(USig::encode(name, arity, sign)) { }

String Sig::name() const {
    uint16_t u = upper(rep_);
    return u < upperMax ? toString(rep_) : cast<USig::Rep>(rep_)->first;
}

uint32_t Sig::arity() const {
    uint16_t u = upper(rep_);
    return u < upperMax ? u : cast<USig::Rep>(rep_)->second;
}

bool Sig::sign() const { return lower(rep_); }

size_t Sig::hash() const { return get_value_hash(rep_); }

bool Sig::operator==(Sig s) const { return rep_ == s.rep_; }
bool Sig::operator!=(Sig s) const { return rep_ != s.rep_; }
bool Sig::operator<(Sig s) const {
    if (sign() != s.sign()) { return sign() < s.sign(); }
    if (arity() < s.arity()) { return arity() < s.arity(); }
    return name() < s.name();
}
bool Sig::operator>(Sig s) const { return !(s < *this); }
bool Sig::operator<=(Sig s) const { return *this == s || *this < s; }
bool Sig::operator>=(Sig s) const { return *this == s || *this > s; }

// {{{1 definition of Symbol

// {{{2 construction

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
    return Symbol(combine(static_cast<uint16_t>(SymbolType_::Sup), num));
}

Symbol Symbol::createId(String val, bool sign) {
    return Symbol(combine(static_cast<uint16_t>(sign ? SymbolType_::IdN : SymbolType_::IdP), String::toRep(val), 0));
}

Symbol Symbol::createStr(String val) {
    return Symbol(combine(static_cast<uint16_t>(SymbolType_::String), String::toRep(val), 0));
}

Symbol Symbol::createTuple(SymSpan args) {
    throw std::logic_error("implement me!!!");
}

Symbol Symbol::createFun(String name, SymSpan args, bool sign) {
    throw std::logic_error("implement me!!!");
}

// {{{2 inspection

SymbolType Symbol::type() const {
    auto t = symbolType_(rep);
    switch (t) {
        case SymbolType_::IdP: { return SymbolType::Func; }
        case SymbolType_::IdN: { return SymbolType::Func; }
        default:               { return static_cast<SymbolType>(t); }
    }
}

int32_t Symbol::num() const {
    assert(type() == SymbolType::Num);
    return static_cast<int32_t>(static_cast<uint32_t>(rep));
}

String Symbol::string() const {
    assert(type() == SymbolType::String);
    return toString(rep);
}

Sig Symbol::sig() const{
    assert(type() == SymbolType::Func);
    switch (symbolType_(rep)) {
        case SymbolType_::IdP: { return Sig(toString(rep), 0, false); }
        case SymbolType_::IdN: { return Sig(toString(rep), 0, true); }
        default:               { return cast<Function>(rep)->sig(); }
    }
}

bool Symbol::hasSig() const {
    return type() == SymbolType::Func;
}

String Symbol::name() const {
    assert(type() == SymbolType::Func);
    switch (symbolType_(rep)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: { return toString(rep); }
        default:               { return cast<Function>(rep)->sig().name(); }
    }
}

SymSpan Symbol::args() const {
    assert(type() == SymbolType::Func);
    switch (symbolType_(rep)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: { return SymSpan{nullptr, 0}; }
        default:               { return cast<Function>(rep)->args(); }
    }
}
bool Symbol::sign() const {
    assert(type() == SymbolType::Func || type() == SymbolType::Num);
    switch (symbolType_(rep)) {
        case SymbolType_::Num: { return num() < 0; }
        case SymbolType_::IdP: { return true; }
        case SymbolType_::IdN: { return false; }
        default:               { return cast<Function>(rep)->sig().sign(); }
    }
}

// {{{2 modification

Symbol Symbol::flipSign() const {
    assert(type() == SymbolType::Func || type() == SymbolType::Num);
    switch (symbolType_(rep)) {
        case SymbolType_::Num: { return Symbol::createNum(-num()); }
        case SymbolType_::IdP: { return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdN))); }
        case SymbolType_::IdN: { return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdP))); }
        default: {
            auto f = cast<Function>(rep);
            auto s = f->sig();
            return createFun(s.name(), f->args(), s.sign());
        }
    }
}

Symbol Symbol::replace(IdSymMap const &map) const {
    assert(symbolType_(rep) != SymbolType_::IdN);
    switch(symbolType_(rep)) {
        case SymbolType_::Func: {
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

bool Symbol::operator==(Symbol const &other) const { return rep == other.rep; }
bool Symbol::operator!=(Symbol const &other) const { return rep != other.rep; }

bool Symbol::operator<(Symbol const &other) const {
    auto ta = symbolType_(rep), tb = symbolType_(other.rep);
    if (ta != tb) { return ta < tb; }
    switch(ta) {
        case SymbolType_::Num: { return num() < other.num(); }
        case SymbolType_::IdN:
        case SymbolType_::IdP: { return name() < other.name(); }
        case SymbolType_::String: { return string() < other.string(); }
        case SymbolType_::Inf: { return false; }
        case SymbolType_::Sup: { return false; }
        case SymbolType_::Func: {
            auto sa = sig(), sb = other.sig();
            if (sa != sb) { return sa < sb; }
            auto aa = args(), ab = other.args();
            return std::lexicographical_compare(begin(aa), end(aa), begin(ab), end(ab));
        }
        case SymbolType_::Special: { return false; }
    }
    assert(false);
    return false;
}

bool Symbol::operator>(Symbol const &other) const { return !(other < *this); }
bool Symbol::operator<=(Symbol const &other) const { return rep == other.rep || *this < other; }
bool Symbol::operator>=(Symbol const &other) const { return rep == other.rep || *this > other; }

// {{{2 output

void Symbol::print(std::ostream& out) const {
    switch(symbolType_(rep)) {
        case SymbolType_::Num: { out << num(); break; }
        case SymbolType_::IdN: { out << "-"; }
        case SymbolType_::IdP: { out << *string(); break; }
        case SymbolType_::String: { out << '"' << quote(string()) << '"'; break; }
        case SymbolType_::Inf: { out << "#inf"; break; }
        case SymbolType_::Sup: { out << "#sup"; break; }
        case SymbolType_::Func: {
            auto s = sig();
            if (s.sign()) { out << "-"; }
            out << static_cast<char const *>(s.name());
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
