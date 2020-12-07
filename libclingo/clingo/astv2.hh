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

enum clingo_ast_type {
    // ids
    clingo_ast_type_id,
    // terms
    clingo_ast_type_variable,
    clingo_ast_type_symbol,
    clingo_ast_type_unary_operation,
    clingo_ast_type_binary_operation,
    clingo_ast_type_interval,
    clingo_ast_type_function,
    clingo_ast_type_pool,
    // csp terms
    clingo_ast_type_csp_product,
    clingo_ast_type_csp_sum,
    clingo_ast_type_csp_guard,
    // simple literals
    clingo_ast_type_boolean_constant,
    clingo_ast_type_symbolic_atom,
    clingo_ast_type_comparison,
    clingo_ast_type_csp_literal,
    // aggregates
    clingo_ast_type_aggregate_guard,
    clingo_ast_type_conditional_literal,
    clingo_ast_type_aggregate,
    clingo_ast_type_body_aggregate_element,
    clingo_ast_type_body_aggregate,
    clingo_ast_type_head_aggregate_element,
    clingo_ast_type_head_aggregate,
    clingo_ast_type_disjunction,
    clingo_ast_type_disjoint_element,
    clingo_ast_type_disjoint,
    // theory atoms
    clingo_ast_type_theory_sequence,
    clingo_ast_type_theory_function,
    clingo_ast_type_theory_unparsed_term_element,
    clingo_ast_type_theory_unparsed_term,
    clingo_ast_type_theory_guard,
    clingo_ast_type_theory_atom_element,
    clingo_ast_type_theory_atom,
    // literals
    clingo_ast_type_literal,
    // theory definition
    clingo_ast_type_theory_operator_definition,
    clingo_ast_type_theory_term_definition,
    clingo_ast_type_theory_guard_definition,
    clingo_ast_type_theory_atom_definition,
    // statements
    clingo_ast_type_rule,
    clingo_ast_type_definition,
    clingo_ast_type_show_signature,
    clingo_ast_type_show_term,
    clingo_ast_type_minimize,
    clingo_ast_type_script,
    clingo_ast_type_program,
    clingo_ast_type_external,
    clingo_ast_type_edge,
    clingo_ast_type_heuristic,
    clingo_ast_type_project_atom,
    clingo_ast_type_project_signature,
    clingo_ast_type_defined,
    clingo_ast_type_theory_definition
};

enum clingo_ast_theory_sequence_type {
    clingo_ast_theory_sequence_type_tuple,
    clingo_ast_theory_sequence_type_list,
    clingo_ast_theory_sequence_type_set
};

class ASTValue;

class AST {
public:
    bool hasValue(char const *name) const;
    ASTValue &value(char const *name);
    ASTValue const &value(char const *name) const;
    clingo_ast_type type() const;

private:
    clingo_ast_type type_;
    std::map<String, ASTValue> values_;
};

void parseStatement(INongroundProgramBuilder &prg, Logger &log, AST &ast);

using SAST = std::shared_ptr<AST>;

enum class ASTValueType : int {
    Num = 0,
    Sym = 1,
    Loc = 2,
    Str = 3,
    AST = 4,
    StrVec = 5,
    ASTVec = 6,
    Empty = 7
};

class ASTValue {
public:
    using StrVec = std::vector<String>;
    using ASTVec = std::vector<SAST>;

    ASTValue();
    ASTValue(int num);
    ASTValue(Symbol sym);
    ASTValue(Location loc);
    ASTValue(String str);
    ASTValue(SAST ast);
    ASTValue(StrVec strs);
    ASTValue(ASTVec asts);
    ASTValue(ASTValue const &val);
    ASTValue(ASTValue &&val) noexcept;
    ASTValue &operator=(ASTValue const &val);
    ASTValue &operator=(ASTValue &&val) noexcept;
    ~ASTValue();

    ASTValueType type() const;
    int &num();
    int num() const;
    String &str();
    String str() const;
    Symbol &sym();
    Symbol sym() const;
    Location &loc();
    Location const &loc() const;
    SAST &ast();
    SAST const &ast() const;
    ASTVec &asts();
    ASTVec const &asts() const;
    StrVec &strs();
    StrVec const &strs() const;
    bool empty() const;

private:
    void clear_();
    void require_(ASTValueType type) const;

    ASTValueType type_;
    union {
        int num_;
        Symbol sym_;
        Location loc_;
        String str_;
        SAST ast_;
        StrVec strs_;
        ASTVec asts_;
    };
};

} } // namespace Input Gringo

#endif // CLINGO_AST_HH
