// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include <gringo/symbol.hh>
#include <gringo/hash_set.hh>
#include <mutex>
#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used: zero-sized array in struct/union
#endif

namespace Gringo {

namespace {

// {{{1 auxiliary functions

constexpr const uint16_t upperMax = std::numeric_limits<uint16_t>::max();
constexpr const uint16_t lowerMax = 3;

uint16_t upper(uint64_t rep) {
    return rep >> 48;
}

uint8_t lower(uint64_t rep) {
    return rep & 3;
}

uintptr_t ptr(uint64_t rep) {
    return static_cast<uintptr_t>(rep & 0xFFFFFFFFFFFC);
}

uint64_t combine(uint16_t u, uintptr_t ptr, uint8_t l) {
    static_cast<void>(lowerMax);
    assert(l <= lowerMax);
    return static_cast<uint64_t>(u) << 48 | ptr | l;
}

uint64_t combine(uint16_t u, int32_t num) {
    return static_cast<uint64_t>(u) << 48 | static_cast<uint64_t>(static_cast<uint32_t>(num));
}

uint64_t setUpper(uint16_t u, uint64_t rep) {
    return combine(u, 0, 0) | (rep & 0xFFFFFFFFFFFF);
}

//uint64_t setLower(uint8_t l, uint8_t rep) {
//    assert(l <= lowerMax);
//    return combine(0, 0, l) | (rep & 0xFFFFFFFFFFFFFFFC);
//}

enum class SymbolType_ : uint8_t {
    Inf = 0,
    Num = 1,
    IdP = 2,
    IdN = 3,
    Str = 4,
    Fun = 5,
    Special = 6,
    Sup = 7
};

SymbolType_ symbolType_(uint64_t rep) {
    return static_cast<SymbolType_>(upper(rep));
}

template <class T>
T const *cast(uint64_t rep) {
    return reinterpret_cast<T const *>(ptr(rep)); // NOLINT
}

String toString(uint64_t rep) {
    return String::fromRep(ptr(rep));
}

// {{{1 definition of Unique

template <class T>
class Unique {
public:
    using Type = typename T::Type;
    struct Hash {
        size_t operator()(Unique const &s) const {
            return T::hash(*s.ptr_);
        }
        template <class U>
        size_t operator()(U const &s) const {
            return T::hash(s);
        }
    };
    struct Create { };
    struct Open { };
    struct Deleted { };
    struct EqualTo {
        bool operator()(Unique const &a, Unique const &b) const {
            return a.ptr_ == b.ptr_;
        }

        template <class U>
        bool operator()(Unique const &a, U const &b) const {
            return T::equal(*a.ptr_, b);
        }
    };
    struct Literals {
        static constexpr Deleted deleted = {};
        static constexpr Open open = {};
    };

    Unique() = default;
    Unique(Unique &&s) noexcept { std::swap(ptr_, s.ptr_); }
    Unique(Unique const &other) = delete;
    Unique &operator=(Unique &&s) noexcept { std::swap(ptr_, s.ptr_); return *this; }
    Unique &operator=(Unique const &other) = delete;

    template <class U>
    Unique(U &&t) // NOLINT
    : ptr_(T::construct(std::forward<U>(t))) { }

    ~Unique() noexcept {
        if (ptr_ && ptr_ != deleted_) { T::destroy(ptr_); }
    }

    Unique &operator=(Open tag) noexcept {
        static_cast<void>(tag);
        this->~Unique();
        ptr_ = nullptr;
        return *this;
    }

    Unique &operator=(Deleted tag) noexcept {
        static_cast<void>(tag);
        this->~Unique();
        ptr_ = deleted_;
        return *this;
    }

    bool operator==(Open tag) const {
        static_cast<void>(tag);
        return ptr_ == nullptr;
    }

    bool operator==(Deleted tag) const {
        static_cast<void>(tag);
        return ptr_ == deleted_;
    }

    template <class U>
    static Type const *encode(U &&x) {
        std::lock_guard<std::mutex> g(mutex_);
        return set_.insert(Hash(), EqualTo(), std::forward<U>(x)).first.ptr_;
    }

private:
    using Set = HashSet<Unique, Literals>;
    static Set set_; // NOLINT
    static std::mutex mutex_; // NOLINT
    static Type const *deleted_; // NOLINT
    Type *ptr_ = nullptr;

};

template <class T>
constexpr typename Unique<T>::Deleted Unique<T>::Literals::deleted;

template <class T>
constexpr typename Unique<T>::Open Unique<T>::Literals::open;

template <class T>
typename Unique<T>::Set Unique<T>::set_; // NOLINT
                                         //
template <class T>
std::mutex Unique<T>::mutex_; // NOLINT
                              //
// NOTE: this is just a sentinel address that is never malloced and never dereferenced
template <class T>
typename Unique<T>::Type const *Unique<T>::deleted_ = reinterpret_cast<typename Unique<T>::Type const *>(&Unique<T>::deleted_); // NOLINT


// {{{1 definition of UString

struct MString {
    using Type = char;

    static size_t hash(char const &str) {
        return strhash(&str);
    }

    static size_t hash(StringSpan str) {
        return strhash(str);
    }

    static bool equal(char const &a, char const &b) {
        return std::strcmp(&a, &b) == 0;
    }

    static bool equal(char const &a, StringSpan b) {
        return std::strncmp(&a, b.first, b.size) == 0 && (&a)[b.size] == '\0'; // NOLINT
    }

    static char *construct(char const &str) {
        std::unique_ptr<char[]> buf{new char[std::strlen(&str) + 1]}; // NOLINT
        std::strcpy(buf.get(), &str); // NOLINT
        return buf.release();
    }

    static char *construct(StringSpan str) {
        std::unique_ptr<char[]> buf{new char[str.size + 1]}; // NOLINT
        std::memcpy(buf.get(), str.first, sizeof(char) * str.size);
        buf[str.size] = '\0';
        return buf.release();
    }

    static void destroy(char const *str) {
        delete [] str; // NOLINT
    }
};

using UString = Unique<MString>;

// {{{1 definition of USig

struct MSig {
    using Type = std::pair<String, uint32_t>;

    static size_t hash(Type const &sig) {
        return get_value_hash(sig);
    }

    static bool equal(Type const &a, Type const &b) {
        return a == b;
    }

    static Type *construct(Type const &sig) {
        return gringo_make_unique<Type>(sig).release();
    }

    static void destroy(Type *sig) {
        delete sig; // NOLINT
    }
};
using USig = Unique<MSig>;

uint64_t encodeSig(String name, uint32_t arity, bool sign) {
    uint8_t isign = sign ? 1 : 0;
    return arity < upperMax
        ? combine(arity, String::toRep(name), isign)
        : combine(upperMax,
                  reinterpret_cast<uintptr_t>(USig::encode(MSig::Type{name, arity})), // NOLINT
                  isign);
}

// {{{1 definition of Fun

class Fun {
public:
    Fun(Fun const &other) = delete;
    Fun(Fun &&other) noexcept = delete;
    Fun &operator=(Fun const &other) = delete;
    Fun &operator=(Fun &&other) noexcept = delete;

    Sig sig() const {
        return sig_;
    }

    SymSpan args() const {
        return {args_, sig().arity()};
    }

    static Fun *make(Sig sig, SymSpan args) {
        auto *mem = ::operator new(sizeof(Fun) + args.size * sizeof(Symbol));
        return new(mem) Fun(sig, args); // NOLINT
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
    Symbol args_[0]; // NOLINT
};

struct MFun {
    using Type = Fun;
    using Cons = std::pair<Sig, SymSpan>;

    static size_t hash(Type const &fun) {
        return Fun::hash(fun.sig(), fun.args());
    }

    static size_t hash(Cons const &fun) {
        return Fun::hash(fun.first, fun.second);
    }

    static size_t cons(Type const &fun) {
        return Fun::hash(fun.sig(), fun.args());
    }

    static bool equal(Type const &a, Type const &b) {
        return a.equal(b.sig(), b.args());
    }

    static bool equal(Type const &a, Cons const &b) {
        return a.equal(b.first, b.second);
    }

    static Type *construct(Cons fun) {
        return Fun::make(fun.first, fun.second);
    }

    static void destroy(Type *fun) {
        const_cast<Fun*>(fun)->destroy(); // NOLINT
    }
};
using UFun = Unique<MFun>;

// }}}1

} // namespace

// {{{1 definition of String

String::String(char const *str)
: str_(UString::encode(*str)) { }

String::String(StringSpan str)
: str_(UString::encode(str)) { }

String::String(uintptr_t r) noexcept
: str_(reinterpret_cast<char const *>(r)) { } // NOLINT

bool String::empty() const {
    return str_[0] == '\0'; // NOLINT
}

size_t String::hash() const {
    return reinterpret_cast<uintptr_t>(str_); // NOLINT
}

uintptr_t String::toRep(String s) noexcept {
    return reinterpret_cast<uintptr_t>(s.str_); // NOLINT
}

String String::fromRep(uintptr_t t) noexcept {
    return {t};
}

// {{{1 definition of Signature

Sig::Sig(String name, uint32_t arity, bool sign)
: Sig{encodeSig(name, arity, sign)} { }

String Sig::name() const {
    uint16_t u = upper(rep());
    return u < upperMax ? toString(rep()) : cast<USig::Type>(rep())->first;
}

Sig Sig::flipSign() const {
    return Sig(rep() ^ 1);
}

uint32_t Sig::arity() const {
    uint16_t u = upper(rep());
    return u < upperMax ? u : cast<USig::Type>(rep())->second;
}

bool Sig::sign() const {
    return lower(rep()) != 0;
}

size_t Sig::hash() const {
    return get_value_hash(rep());
}

namespace {

bool less(Sig const &a, Sig const &b) {
    if (a.sign() != b.sign()) { return !a.sign() && b.sign(); }
    if (a.arity() != b.arity()) { return a.arity() < b.arity(); }
    return a.name() < b.name();
}

} // namespace

bool Sig::operator==(Sig s) const {
    return rep() == s.rep();
}

bool Sig::operator!=(Sig s) const {
    return rep() != s.rep();
}

bool Sig::operator<(Sig s) const {
    return *this != s && less(*this, s);
}

bool Sig::operator>(Sig s) const {
    return *this != s && less(s, *this);
}

bool Sig::operator<=(Sig s) const {
    return *this == s || less(*this, s);
}

bool Sig::operator>=(Sig s) const {
    return *this == s || less(s, *this);
}

// {{{1 definition of Symbol

// {{{2 construction

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
        ?  Symbol(combine(static_cast<uint16_t>(SymbolType_::Fun),
                          reinterpret_cast<uintptr_t>(UFun::encode(std::make_pair(Sig(name, numeric_cast<uint32_t>(args.size), sign), args))), // NOLINT
                          0))
        : createId(name, sign);
}

// {{{2 inspection

SymbolType Symbol::type() const {
    auto t = symbolType_(rep_);
    switch (t) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: { return SymbolType::Fun; }
        default:               { return static_cast<SymbolType>(t); }
    }
}

int32_t Symbol::num() const {
    assert(type() == SymbolType::Num);
    return static_cast<int32_t>(static_cast<uint32_t>(rep_));
}

String Symbol::string() const {
    assert(type() == SymbolType::Str);
    return toString(rep_);
}

Sig Symbol::sig() const{
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep_)) {
        case SymbolType_::IdP: { return {toString(rep_), 0, false}; }
        case SymbolType_::IdN: { return {toString(rep_), 0, true}; }
        default:               { return cast<Fun>(rep_)->sig(); }
    }
}

bool Symbol::hasSig() const {
    return type() == SymbolType::Fun;
}

String Symbol::name() const {
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep_)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: { return toString(rep_); }
        default:               { return cast<Fun>(rep_)->sig().name(); }
    }
}

SymSpan Symbol::args() const {
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep_)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: { return SymSpan{nullptr, 0}; }
        default:               { return cast<Fun>(rep_)->args(); }
    }
}

bool Symbol::sign() const {
    assert(type() == SymbolType::Fun || type() == SymbolType::Num);
    switch (symbolType_(rep_)) {
        case SymbolType_::Num: { return num() < 0; }
        case SymbolType_::IdP: { return false; }
        case SymbolType_::IdN: { return true; }
        default:               { return cast<Fun>(rep_)->sig().sign(); }
    }
}

// {{{2 modification

Symbol Symbol::flipSign() const {
    assert(type() == SymbolType::Fun || type() == SymbolType::Num);
    switch (symbolType_(rep_)) {
        case SymbolType_::Num: { return Symbol::createNum(-num()); }
        case SymbolType_::IdP: { return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdN), rep_)); }
        case SymbolType_::IdN: { return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdP), rep_)); }
        default: {
            auto const *f = cast<Fun>(rep_);
            auto s = f->sig();
            return createFun(s.name(), f->args(), !s.sign());
        }
    }
}

Symbol Symbol::replace(IdSymMap const &map) const {
    assert(symbolType_(rep_) != SymbolType_::IdN);
    switch(symbolType_(rep_)) {
        case SymbolType_::Fun: {
            SymVec vals;
            for (auto const &x : args()) {
                vals.emplace_back(x.replace(map));
            }
            return createFun(name(), Potassco::toSpan(vals));
        }
        case SymbolType_::IdP: {
            auto it = map.find(name());
            if (it != map.end()) {
                return it->second;
            }
        }
        default: {
            return *this;
        }
    }
}

// {{{2 comparison

size_t Symbol::hash() const {
    return get_value_hash(rep_);
}

namespace {

bool less(Symbol const &a, Symbol const &b) {
    auto ta = symbolType_(a.rep());
    auto tb = symbolType_(b.rep());
    if (ta != tb) { return ta < tb; }
    switch(ta) {
        case SymbolType_::Num: { return a.num() < b.num(); }
        case SymbolType_::IdN:
        case SymbolType_::IdP: { return a.name() < b.name(); }
        case SymbolType_::Str: { return a.string() < b.string(); }
        case SymbolType_::Inf:
        case SymbolType_::Sup: { return false; }
        case SymbolType_::Fun: {
            auto sa = a.sig();
            auto sb = b.sig();
            if (sa != sb) {
                return sa < sb;
            }
            auto aa = a.args();
            auto ab = b.args();
            return std::lexicographical_compare(begin(aa), end(aa), begin(ab), end(ab));
        }
        case SymbolType_::Special: { return false; }
    }
    assert(false);
    return false;
}

} // namespace

bool Symbol::operator==(Symbol const &other) const {
    return rep_ == other.rep_;
}

bool Symbol::operator!=(Symbol const &other) const {
    return rep_ != other.rep_;
}

bool Symbol::operator<(Symbol const &other) const {
    return (*this != other) && less(*this, other);
}

bool Symbol::operator>(Symbol const &other) const {
    return (*this != other) && less(other, *this);
}

bool Symbol::operator<=(Symbol const &other) const {
    return (*this == other) || less(*this, other);
}

bool Symbol::operator>=(Symbol const &other) const {
    return (*this == other) || less(other, *this);
}

// {{{2 output

void Symbol::print(std::ostream& out) const {
    switch(symbolType_(rep_)) {
        case SymbolType_::Num: { out << num(); break; }
        case SymbolType_::IdN: { out << "-"; }
        case SymbolType_::IdP: {
            char const *n = name().c_str();
            out << (n[0] != '\0' ? n : "()"); // NOLINT
            break;
        }
        case SymbolType_::Str: { out << '"' << quote(string().c_str()) << '"'; break; }
        case SymbolType_::Inf: { out << "#inf"; break; }
        case SymbolType_::Sup: { out << "#sup"; break; }
        case SymbolType_::Fun: {
            auto s = sig();
            if (s.sign()) {
                out << "-";
            }
            out << s.name();
            auto a = args();
            out << "(";
            if (a.size > 0) {
                std::copy(begin(a), end(a) - 1, std::ostream_iterator<Symbol>(out, ",")); // NOLINT
                out << *(end(a) - 1); // NOLINT
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

