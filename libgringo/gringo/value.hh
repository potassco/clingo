// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

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

#ifndef _GRINGO_VALUE_HH
#define _GRINGO_VALUE_HH

#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <utility>
#include <gringo/flyweight.hh>

namespace Gringo {

struct Value;
using ValVec   = std::vector<Value>;
using FWValVec = FlyweightVec<Value>;
using FWString = Flyweight<std::string>;
using IdValMap = std::unordered_map<FWString, Value>;

inline std::ostream &operator<<(std::ostream &out, FWString const &x) {
    out << *x;
    return out;
}

// {{{ declaration of quote/unquote

std::string quote(std::string const &str);
std::string unquote(std::string const &str);

// }}}
// {{{ declaration of Signature

struct FWSignature;
struct Signature {
    friend struct FWSignature;

    Signature(FWString name, unsigned length, bool sign = false);

    FWString name() const;
    unsigned length() const;
    bool sign() const;
    Signature flipSign() const;

    template <bool enc>
    bool encode(unsigned &uid) const;

    size_t hash() const;
    bool operator==(Signature const &other) const;
    bool operator!=(Signature const &other) const;
    bool operator<(Signature const &other) const;

private:
    Flyweight<std::string> name_;
    unsigned length_;
};

inline std::ostream &operator<<(std::ostream &out, Signature const &x) {
    if (x.sign()) { out << "-"; }
    out << *x.name() << "/" << x.length();
    return out;
}

// }}}
// {{{ declaration of FWSignature

struct FWSignature {
    using FWSig = Flyweight<Signature>;

    FWSignature(FWString name, unsigned length, bool sign = false);
    FWSignature(Signature const &sig);
    FWSignature(unsigned repr);
    unsigned uid() const;
    Signature operator*() const;

    bool operator==(FWSignature const &other) const;
    bool operator!=(FWSignature const &other) const;
    bool operator<(FWSignature const &other) const;
    bool operator>(FWSignature const &other) const;
    bool operator<=(FWSignature const &other) const;
    bool operator>=(FWSignature const &other) const;

    unsigned repr;
};

inline std::ostream &operator<<(std::ostream &out, FWSignature const &x) {
    out << *x;
    return out;
}

// }}}
// {{{ declaration of Value

struct Value {
    enum Type : unsigned { INF, NUM, ID, STRING, FUNC, SPECIAL, SUP };
    struct POD;

    // construction
    Value(); // createSpecial
    static Value createId(FWString val, bool sign = false);
    static Value createStr(FWString val);
    static Value createNum(int num);
    static Value createInf();
    static Value createSup();
    static Value createTuple(FWValVec val);
    static Value createFun(FWString name, FWValVec val, bool sign = false);

    // value retrieval
    Type type() const;
    int num() const;
    FWString string() const;
    FWSignature sig() const;
    bool hasSig() const;
    FWString name() const;
    FWValVec args() const;
    bool sign() const;
    Value replace(IdValMap const &rep) const;

    // modifying values
    Value flipSign() const;

    // comparison
    size_t hash() const;
    bool operator==(Value const &other) const;
    bool less(Value const &other) const;
    bool operator!=(Value const &other) const;
    bool operator<(Value const &other) const;
    bool operator>(Value const &other) const;
    bool operator<=(Value const &other) const;
    bool operator>=(Value const &other) const;

    // ouput
    void print(std::ostream& out) const;

    operator POD&();
    operator POD const &() const;

    static unsigned const typeBits = 4u;
    static unsigned const typeMask = 1u+2u+4u+8u;

private:
    Value (unsigned type, unsigned value);

    unsigned type_;
    unsigned value_;
};

struct Value::POD {
    operator Value&();
    operator Value const &() const;
    Value *operator->();
    Value const *operator->() const;
    unsigned type;
    unsigned value;
};

std::ostream& operator<<(std::ostream& out, const Gringo::Value& val);

// }}}

} // namespace Gringo

namespace std {

// {{{ declaration of hash functions for Signature and Value

template<>
struct hash<Gringo::Signature> {
    size_t operator()(Gringo::Signature const &sig) const;
};

template<>
struct hash<Gringo::FWSignature> {
    size_t operator()(Gringo::FWSignature const &sig) const;
};

template<>
struct hash<Gringo::Value> {
    size_t operator()(Gringo::Value const &val) const;
};

// }}}

} // namespace std

namespace Gringo {

// {{{ definition of FWSignature

inline FWSignature::FWSignature(FWString name, unsigned length, bool sign)
: FWSignature(Signature(name, length, sign)) { }

inline FWSignature::FWSignature(Signature const &sig) {
    if (!sig.encode<sizeof(unsigned) >= 4>(repr)) {
        repr = FWSig(sig).uid() << 1u;
    }
}

inline FWSignature::FWSignature(unsigned uid) : repr(uid) { }

inline unsigned FWSignature::uid() const { return repr; }

inline Signature FWSignature::operator*() const {
    return repr & 1u ? Signature(repr >> 4u, repr >> 1u & (1u + 2u + 4u)) : *FWSig(repr >> 1u);
}

inline bool FWSignature::operator==(FWSignature const &other) const { return repr == other.repr; }
inline bool FWSignature::operator!=(FWSignature const &other) const { return repr != other.repr; }
inline bool FWSignature::operator<(FWSignature const &other) const  { return **this < *other; }
inline bool FWSignature::operator>(FWSignature const &other) const  { return *other < **this; }
inline bool FWSignature::operator<=(FWSignature const &other) const { return repr == other.repr || **this < *other; }
inline bool FWSignature::operator>=(FWSignature const &other) const { return repr == other.repr || *other < **this; }

// }}}
// {{{ definition of Signature

inline Signature::Signature(FWString name, unsigned length, bool sign)
: name_(name)
, length_(sign | (length << 1)) { }

inline bool Signature::sign() const {
    return length_ & 1;
}
inline FWString Signature::name() const {
    return name_;
}

inline Signature Signature::flipSign() const {
    return Signature(name(), length(), !sign());
}

inline unsigned Signature::length() const {
    return length_ >> 1;
}

inline size_t Signature::hash() const {
    return get_value_hash(name_.uid(), length_);
}

inline bool Signature::operator==(Signature const &other) const {
    return name_ == other.name_ && length_ == other.length_;
}

inline bool Signature::operator!=(Signature const &other) const {
    return !operator==(other);
}

inline bool Signature::operator<(Signature const &other) const {
    if (sign()   != other.sign())   { return sign()  < other.sign(); }
    if (length() != other.length()) { return length() < other.length(); }
    return name_ < other.name_;
}

template<bool enc>
bool Signature::encode(unsigned &uid) const {
    // Note: the number of different signatures is restricted to 2^28
    if (enc && !sign() && length() < 8 && name_.uid() < 16777216u) {
        uid = (name_.uid() << 4u) | (length() << 1u) | 1u;
        return true;
    }
    return false;
}

// }}}
// {{{ definition of Value::POD

inline Value::POD::operator Value&() {
    // NOTE: strictly speaking this is not legal
    //       but I think that this would be okay,
    //       if POD where a *member* of Value
    return *reinterpret_cast<Value*>(this);
}

inline Value::POD::operator Value const &() const {
    return *reinterpret_cast<Value const *>(this);
}

inline Value *Value::POD::operator->() {
    return reinterpret_cast<Value *>(this);
}

inline Value const *Value::POD::operator->() const {
    return reinterpret_cast<Value const *>(this);
}

// }}}
// {{{ definition of Value

inline Value::Value(unsigned type, unsigned value)
    : type_(type)
    , value_(value) { }

inline Value::Value()
    : Value(SPECIAL, 0) { }

inline Value Value::createInf() {
    return Value(INF, 0);
}

inline Value Value::createSup() {
    return Value(SUP, 0);
}

inline Value Value::createNum(int val) {
    return Value(NUM, static_cast<unsigned>(val));
}

inline Value Value::createId(FWString val, bool sign) {
    return Value(ID, val.uid() << 1 | sign);
}

inline Value Value::createStr(FWString val) {
    return Value(STRING, val.uid());
}

inline Value Value::createTuple(FWValVec args) {
    return createFun("", args);
}

inline Value Value::createFun(FWString name, FWValVec args, bool sign) {
    return Value(FUNC | (FWSignature(name, args.size(), sign).uid() << typeBits), args.offset());
}

inline void Value::print(std::ostream& out) const {
    switch(type()) {
        case NUM: { out << num(); break; }
        case ID: {
            if (value_ & 1) { out << "-"; }
            out << *string();
            break;
        }
        case STRING: { out << '"' << quote(*string()) << '"'; break; }
        case INF: { out << "#inf"; break; }
        case SUP: { out << "#sup"; break; }
        case FUNC: {
            Signature s = *sig();
            if (s.sign()) { out << "-"; }
            out << s.name();
            auto a = args();
            out << "(";
            if (a.size() > 0) {
                std::copy(a.begin(), a.end() - 1, std::ostream_iterator<Value>(out, ","));
                out << *(a.end() - 1);
            }
            if (a.size() == 1 && s.name() == "") {
                out << ",";
            }
            out << ")";
            break;
        }
        case SPECIAL: { out << "#special"; break; }
    }
}

inline int ipow(int a, int b) {
    if (b < 0) { return 0; }
    else {
        int r = 1;
        while (b > 0) {
            if(b & 1) { r *= a; }
            b >>= 1;
            a *= a;
        }
        return r;
    }
}

inline size_t Value::hash() const {
    return get_value_hash(type_, value_);
}

inline bool Value::operator==(Value const &other) const {
    return type_ == other.type_ && value_ == other.value_;
}

inline bool Value::less(Value const &other) const {
    if (type() != other.type()) { return type() < other.type(); }
    switch(type()) {
        case NUM: { return num() < other.num(); }
        case ID: {
            bool sa = value_ & 1, sb = other.value_ & 1;
            if (sa != sb) { return sa < sb; }
            return string()->compare(other.string()) < 0;
        }
        case STRING: { return *string() < *other.string(); }
        case INF: { return false; }
        case SUP: { return false; }
        case FUNC: {
            Signature sa = *sig(), sb = *other.sig();
            if (sa.sign() != sb.sign()) { return sa.sign() < sb.sign(); }
            auto aa = args(), ab = other.args();
            if (aa.size() != ab.size()) { return aa.size() < ab.size(); }
            auto na = sa.name(), nb = sb.name();
            if (na != nb) { return na < nb; }
            return std::lexicographical_compare(aa.begin(), aa.end(), ab.begin(), ab.end());
        }
        case SPECIAL: { return false; }
    }
    assert(false);
    return false;
}

inline Value Value::replace(IdValMap const &rep) const {
    switch(type()) {
        case ID: {
            assert(!(value_ & 1));
            auto it = rep.find(name());
            if (it != rep.end()) { return it->second; }
        }
        case NUM:
        case INF:
        case SUP:
        case SPECIAL:
        case STRING: { return *this; }
        case FUNC: {
            ValVec vals;
            for (auto &x : args()) { vals.emplace_back(x.replace(rep)); }
            return createFun(name(), vals);
        }
    }

}

inline Value Value::flipSign() const {
    switch (type()) {
        case ID: { return Value(type_, value_ ^ 1); }
        case FUNC: {
            Signature s = *sig();
            assert(*s.name() != "");
            return createFun(s.name(), args(), !s.sign());
        }
        default: { assert(false); }
    }
    return Value();
}

inline bool Value::operator!=(Value const &other) const {
    return !(*this == other);
}

inline bool Value::operator<(Value const &other) const {
    return (*this != other) && this->less(other);
}

inline bool Value::operator>(Value const &other) const {
    return (*this != other) && other.less(*this);
}

inline bool Value::operator<=(Value const &other) const {
    return (*this == other) || this->less(other);
}

inline bool Value::operator>=(Value const &other) const {
    return (*this == other) || other.less(*this);
}

inline Value::Type Value::type() const {
    return Type(type_ & unsigned(typeMask));
}

inline int Value::num() const {
    assert(type() == NUM);
    return static_cast<int>(value_);
}

inline FWString Value::string() const {
    assert(type() == STRING || type() == ID);
    return value_ >> (type() == ID);
}

inline FWSignature Value::sig() const {
    switch (type()) {
        case FUNC: { return FWSignature(type_ >> typeBits); }
        case ID:   { return FWSignature(string(), 0, value_ & 1); }
        default:   { assert(false); }
    }
    return FWSignature("", 0);
}

inline bool Value::hasSig() const {
    switch (type()) {
        case FUNC:
        case ID:   { return true; }
        default:   { return false; }
    }
}

inline FWString Value::name() const {
    switch (type())  {
        case FUNC: { return (*FWSignature(type_ >> typeBits)).name(); }
        case ID:   { return value_ >> 1; }
        default:   { assert(false); }
    }
    return 0u;
}

inline FWValVec Value::args() const {
    assert(type() == FUNC);
    return FWValVec(FWValVec::fromOffset, (*FWSignature(type_ >> typeBits)).length(), value_);
}
inline bool Value::sign() const {
    assert(hasSig());
    return type() == Value::ID ? value_ & 1 : (*sig()).sign();
}

inline Value::operator Value::POD&() {
    return *reinterpret_cast<POD*>(this);
}

inline Value::operator Value::POD const &() const {
    return *reinterpret_cast<POD const *>(this);
}

inline std::ostream& operator<<(std::ostream& out, Gringo::Value const &val) {
    val.print(out);
    return out;
}

// }}}
// {{{ definition of quote/unquote

inline std::string quote(const std::string &str) {
    std::string res;
    for (char c : str) {
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

inline std::string unquote(const std::string &str) {
    std::string res;
    bool slash = false;
    for (char c : str) {
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

// }}}

} // namespace Gringo

namespace std {

// {{{ definition of hash functions for Signature and Value

inline size_t hash<Gringo::Signature>::operator()(Gringo::Signature const &sig) const { return sig.hash(); }
inline size_t hash<Gringo::FWSignature>::operator()(Gringo::FWSignature const &sig) const { return sig.uid(); }
inline size_t hash<Gringo::Value>::operator()(Gringo::Value const &val) const { return val.hash(); }

// }}}

} // namespace std

#endif // _GRINGO_VALUE_HH

