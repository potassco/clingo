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

#ifndef CLINGO_AST_HH
#define CLINGO_AST_HH

#include <gringo/input/programbuilder.hh>
#include <clingo.h>

namespace Gringo { namespace Input {

class AST;
class Value;
using SValue = std::shared_ptr<Value>;
using SAST = std::shared_ptr<AST>;

enum class ValueType : int {
    Num = 0,
    Sym = 1,
    Str = 2,
    AST = 3,
    StrVec = 4,
    ASTVec = 5
};

class AST {
public:

private:
    std::map<String, Value> values_;
};

class Value {
public:
    using SStrVec = std::shared_ptr<std::vector<String>>;
    using SASTVec = std::shared_ptr<std::vector<SAST>>;
    Value(int num)
    : type_{ValueType::Num}
    , num_{num} { }
    Value(Symbol sym)
    : type_{ValueType::Sym}
    , sym_{sym} { }
    Value(String str)
    : type_{ValueType::Str}
    , str_{str} { }
    Value(SStrVec strs)
    : type_{ValueType::StrVec} {
        new (&strs_) SStrVec{std::move(strs)};
    }
    Value(SASTVec asts)
    : type_{ValueType::ASTVec} {
        new (&asts_) SASTVec{std::move(asts)};
    }
    Value(Value const &val);
    Value(Value &&val) noexcept;
    Value &operator=(Value const &val);
    Value &operator=(Value &&val) noexcept;
    ~Value();
    ValueType type() const;
    int &num() {
        if (type_ != ValueType::Num) {
            throw std::runtime_error("invalid attribute");
        }
        return num_;
    }
    int const &num() const {
        if (type_ != ValueType::Num) {
            throw std::runtime_error("invalid attribute");
        }
        return num_;
    }
    String &str() {
        if (type_ != ValueType::Str) {
            throw std::runtime_error("invalid attribute");
        }
        return str_;
    }
    String const &str() const {
        if (type_ != ValueType::Str) {
            throw std::runtime_error("invalid attribute");
        }
        return str_;
    }
    SAST &ast() {
        if (type_ != ValueType::AST) {
            throw std::runtime_error("invalid attribute");
        }
        return ast_;
    }
    SAST const &ast() const {
        if (type_ != ValueType::AST) {
            throw std::runtime_error("invalid attribute");
        }
        return ast_;
    }
    SASTVec &asts() {
        if (type_ != ValueType::ASTVec) {
            throw std::runtime_error("invalid attribute");
        }
        return asts_;
    }
    SASTVec const &asts() const {
        if (type_ != ValueType::ASTVec) {
            throw std::runtime_error("invalid attribute");
        }
        return asts_;
    }
    SStrVec &strs() {
        if (type_ != ValueType::StrVec) {
            throw std::runtime_error("invalid attribute");
        }
        return strs_;
    }
    SStrVec const &strs() const {
        if (type_ != ValueType::StrVec) {
            throw std::runtime_error("invalid attribute");
        }
        return strs_;
    }

private:
    ValueType type_;
    union {
        int num_;
        Symbol sym_;
        String str_;
        SAST ast_;
        SStrVec strs_;
        SASTVec asts_;
    };
};

} } // namespace Input Gringo

#endif // CLINGO_AST_HH
