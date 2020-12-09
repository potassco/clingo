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

enum clingo_ast_value_type {
    clingo_ast_value_type_empty = 0,
    clingo_ast_value_type_number = 1,
    clingo_ast_value_type_symbol = 2,
    clingo_ast_value_type_location = 3,
    clingo_ast_value_type_string = 4,
    clingo_ast_value_type_ast = 5,
    clingo_ast_value_type_string_array = 6,
    clingo_ast_value_type_ast_array = 7,
};
typedef int clingo_ast_value_type_t;

class AST;
using SAST = std::shared_ptr<AST>;
using SASTCallback = std::function<void (SAST ast)>;

class AST {
public:
    using StrVec = std::vector<String>;
    using ASTVec = std::vector<SAST>;
    using Value = mpark::variant<mpark::monostate, int, Symbol, Location, String, SAST, StrVec, ASTVec>;

    AST(clingo_ast_type type);

    bool hasValue(char const *name) const;
    Value &value(char const *name);
    Value const &value(char const *name) const;
    void value(char const *name, Value value);
    clingo_ast_type type() const;
    SAST copy();
    SAST deepcopy();

private:
    clingo_ast_type type_;
    std::map<String, Value> values_;
};

std::unique_ptr<INongroundProgramBuilder> build(SASTCallback cb);
void parse(INongroundProgramBuilder &prg, Logger &log, AST &ast);

} } // namespace Input Gringo

#endif // CLINGO_AST_HH
