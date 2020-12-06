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

    Value(int num);
    Value(Symbol sym);
    Value(String str);
    Value(SAST ast);
    Value(SStrVec strs);
    Value(SASTVec asts);
    Value(Value const &val);
    Value(Value &&val) noexcept;
    Value &operator=(Value const &val);
    Value &operator=(Value &&val) noexcept;
    ~Value();

    ValueType type() const;
    int &num();
    int num() const;
    String &str();
    String str() const;
    SAST &ast();
    SAST const &ast() const;
    SASTVec &asts();
    SASTVec const &asts() const;
    SStrVec &strs();
    SStrVec const &strs() const;

private:
    void clear_();
    void require_(ValueType type) const;

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
