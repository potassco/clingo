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
#include <clingo/variant.hh>

namespace Gringo { namespace Input {

class SAST;
using SASTCallback = std::function<void (SAST ast)>;

class AST {
public:
    using StrVec = std::vector<String>;
    using ASTVec = std::vector<SAST>;
    using Value = mpark::variant<mpark::monostate, int, Symbol, Location, String, SAST, StrVec, ASTVec>;

    AST(clingo_ast_type type);

    bool hasValue(clingo_ast_attribute name) const;
    Value &value(clingo_ast_attribute name);
    Value const &value(clingo_ast_attribute name) const;
    void value(clingo_ast_attribute name, Value value);
    clingo_ast_type type() const;
    SAST copy();
    SAST deepcopy();

    void incRef();
    void decRef();
    unsigned refCount() const;
    bool unique() const;
private:
    clingo_ast_type type_;
    unsigned refCount_{0};
    std::map<clingo_ast_attribute, Value> values_;
};

// Note: we do not need all the shared pointer functionality and also want to
//       have control about the refcount without.
class SAST {
public:
    SAST();
    SAST(SAST const &ast);
    SAST(SAST &&ast) noexcept;
    SAST &operator=(SAST const &ast);
    SAST &operator=(SAST &&ast) noexcept;
    AST *operator->();
    AST *operator->() const;
    AST &operator*();
    AST &operator*() const;

    explicit SAST(clingo_ast_type type);
    explicit SAST(AST const &ast);
    explicit SAST(AST *ast);
    AST *get();
    unsigned use_count() const;
    void clear();
    ~SAST();
private:
    AST *ast_;
};

std::unique_ptr<INongroundProgramBuilder> build(SASTCallback cb);
void parse(INongroundProgramBuilder &prg, Logger &log, AST &ast);

} } // namespace Input Gringo

#endif // CLINGO_AST_HH
