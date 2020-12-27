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

// TODO:
// - unpooling
// - the way OAST and SAST are handled is really realy ugly

namespace Gringo { namespace Input {

class AST;

// Note: we do not need all the shared pointer functionality and also want to
//       have control about the refcount for the C binding.
class SAST {
public:
    SAST();
    SAST(SAST const &ast);
    SAST(SAST &&ast) noexcept;
    SAST &operator=(SAST const &ast);
    SAST &operator=(SAST &&ast) noexcept;
    AST *operator->() const;
    AST &operator*() const;

    explicit SAST(clingo_ast_type type);
    explicit SAST(AST *ast);
    AST *get() const;
    AST *release();
    unsigned use_count() const;
    void clear();
    ~SAST();
private:
    AST *ast_;
};

using SASTCallback = std::function<void (SAST ast)>;

struct OAST {
    SAST ast;
};

class AST {
public:
    using StrVec = std::vector<String>;
    using ASTVec = std::vector<SAST>;
    using Value = mpark::variant<int, Symbol, Location, String, SAST, OAST, StrVec, ASTVec>;
    using AttributeVector = std::vector<std::pair<clingo_ast_attribute, Value>>;

    AST(clingo_ast_type type);
    AST() = delete;
    AST(AST const &) = delete;
    AST(AST &&) noexcept = default;
    AST &operator=(AST const &) = default;
    AST &operator=(AST &&) noexcept = default;
    ~AST() = default;

    bool hasValue(clingo_ast_attribute name) const;
    Value &value(clingo_ast_attribute name);
    Value const &value(clingo_ast_attribute name) const;
    void value(clingo_ast_attribute name, Value value);
    clingo_ast_type type() const;
    SAST copy();
    SAST deepcopy();

    size_t hash() const;
    friend bool operator<(AST const &a, AST const &b);
    friend bool operator==(AST const &a, AST const &b);

    void incRef();
    void decRef();
    unsigned refCount() const;
    bool unique() const;
private:
    AttributeVector::iterator find_(clingo_ast_attribute name);
    AttributeVector::const_iterator find_(clingo_ast_attribute name) const;

    clingo_ast_type type_;
    unsigned refCount_{0};
    AttributeVector values_;
};

std::ostream &operator<<(std::ostream &out, AST const &ast);

std::unique_ptr<INongroundProgramBuilder> build(SASTCallback cb);
void parse(INongroundProgramBuilder &prg, Logger &log, AST const &ast);

} } // namespace Input Gringo

struct clingo_ast {
    Gringo::Input::AST ast;
};

#endif // CLINGO_AST_HH
