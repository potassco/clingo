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
#include <potassco/basic_types.h>

extern "C" {

enum clingo_symbol_type {
    clingo_symbol_type_inf = 0,
    clingo_symbol_type_num = 1,
    clingo_symbol_type_str = 4,
    clingo_symbol_type_fun = 5,
    clingo_symbol_type_sup = 7
};
typedef int clingo_symbol_type_t;
typedef struct clingo_symbol {
    uint64_t rep;
} clingo_symbol_t;

} // extern "C"

namespace Gringo {

struct Sig {
    // Sig can as well derive from a C pod
    // there are at least 16 free bits in the pointer
    // 1 bit for the sign
    // 15 bits for the arity
    // 1 special value for larger (and very unlikely) arities
    Sig(char const *name, uint32_t arity, bool sign);
    char const *name();
    uint32_t arity();
    bool sign();
};

enum class SymbolType : uint8_t {
    Inf     = clingo_symbol_type_inf,
    Num     = clingo_symbol_type_num,
    String  = clingo_symbol_type_str,
    Func    = clingo_symbol_type_fun,
    Special = clingo_symbol_type_fun+1,
    Sup     = clingo_symbol_type_sup
};

class Symbol;
using SymVec = std::vector<Symbol>;
using SymSpan = Potassco::Span<Symbol>;
using IdSymMap = std::unordered_map<char const *, Symbol>;

class Symbol : public clingo_symbol {
    Symbol(); // createSpecial
    static Symbol createId(char const *val, bool sign = false);
    static Symbol createStr(char const *val);
    static Symbol createNum(int num);
    static Symbol createInf();
    static Symbol createSup();
    static Symbol createTuple(SymVec val);
    static Symbol createFun(char const *name, SymSpan val, bool sign = false);

    // value retrieval
    SymbolType type() const;
    int num() const;
    char const *string() const;
    Sig sig() const;
    bool hasSig() const;
    char const *name() const;
    SymSpan args() const;
    bool sign() const;

    // modifying values
    Symbol replace(IdSymMap const &rep) const;
    Symbol flipSign() const;

    // comparison
    size_t hash() const;
    bool operator==(Symbol const &other) const;
    bool less(Symbol const &other) const;
    bool operator!=(Symbol const &other) const;
    bool operator<(Symbol const &other) const;
    bool operator>(Symbol const &other) const;
    bool operator<=(Symbol const &other) const;
    bool operator>=(Symbol const &other) const;

    // ouput
    void print(std::ostream& out) const;
private:
    enum Type : uint8_t {
        Inf = clingo_symbol_type_inf,
        Num = clingo_symbol_type_num,
        IdP = clingo_symbol_type_num+1,
        IdN = clingo_symbol_type_num+2,
        String = clingo_symbol_type_str,
        Func = clingo_symbol_type_fun,
        Special = clingo_symbol_type_fun+1,
        Sup = clingo_symbol_type_sup
    };
    Type type_() const;
    template <class T>
    T const *cast() const { return reinterpret_cast<T const *>(static_cast<uintptr_t>(rep & mask_)); }
    static constexpr const uint64_t shift_ = 56;
    static constexpr const uint64_t mask_ = 0xFFFFFFFFFFFFFF;
};

namespace {
    template <class T>
    struct Destroy {
        void operator()(T *t) noexcept {
            t->~T();
            ::operator delete(t);
        }
    };
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
} // nampspace

Symbol::Type Symbol::type_() const {
    return static_cast<Type>(rep >> shift_);
}

SymbolType Symbol::type() const {
    auto t = type_();
    switch (t) {
        case Type::IdP: { return SymbolType::Func; }
        case Type::IdN: { return SymbolType::Func; }
        default:        { return static_cast<SymbolType>(t); }
    }
}

int32_t Symbol::num() const {
    assert(type() == SymbolType::Num);
    return static_cast<int>(rep);
}

char const *Symbol::string() const {
    assert(type() == SymbolType::String);
    return cast<char>();
}

Sig Symbol::sig() const{
    assert(type() == SymbolType::Func);
    switch (type_()) {
        case Type::IdP: { return Sig(cast<char>(), 0, false); }
        case Type::IdN: { return Sig(cast<char>(), 0, true); }
        default:        { return cast<Function>()->sig(); }
    }
}

bool Symbol::hasSig() const {
    return type() == SymbolType::Func;
}

char const *Symbol::name() const {
    assert(type() == SymbolType::Func);
    switch (type_()) {
        case Type::IdP:
        case Type::IdN: { return cast<char>(); }
        default:        { return cast<Function>()->sig().name(); }
    }
}

SymSpan Symbol::args() const {
    assert(type() == SymbolType::Func);
    switch (type_()) {
        case Type::IdP:
        case Type::IdN: { return SymSpan{nullptr, 0}; }
        default:        { return cast<Function>()->args(); }
    }
}
bool Symbol::sign() const {
    assert(type() == SymbolType::Func);
    switch (type_()) {
        case Type::Num: { return num() < 0; }
        case Type::IdP: { return true; }
        case Type::IdN: { return false; }
        default:        { return cast<Function>()->sig().sign(); }
    }
}

Symbol Symbol::replace(IdSymMap const &rep) const {
    assert(type_() != Type::IdN);
    switch(type_()) {
        case Type::Func: {
            SymVec vals;
            for (auto &x : args()) { vals.emplace_back(x.replace(rep)); }
            return createFun(name(), Potassco::toSpan(vals));
        }
        case Type::IdP: {
            auto it = rep.find(name());
            if (it != rep.end()) { return it->second; }
        }
        default: { return *this; }
    }
}

} // namespace Gringo
