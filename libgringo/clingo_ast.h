// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#ifndef CLINGO_AST_H
#define CLINGO_AST_H

#include <clingo.h>

#ifdef __cplusplus
extern "C" {
#endif

// {{{1 ast

enum clingo_ast_comparison {
    clingo_ast_comparison_greater_than,
    clingo_ast_comparison_less_than,
    clingo_ast_comparison_less_equal,
    clingo_ast_comparison_greater_equal,
    clingo_ast_comparison_not_equal,
    clingo_ast_comparison_equal
};
typedef int clingo_ast_comparison_t;

enum clingo_ast_sign {
    clingo_ast_sign_none,
    clingo_ast_sign_negation,
    clingo_ast_sign_double_negation,
};
typedef int clingo_ast_sign_t;

// {{{2 terms

enum clingo_ast_term_type {
    clingo_ast_term_type_symbol            = 0,
    clingo_ast_term_type_variable          = 1,
    clingo_ast_term_type_unary_operation   = 2,
    clingo_ast_term_type_binary_operation  = 3,
    clingo_ast_term_type_interval          = 4,
    clingo_ast_term_type_function          = 5,
    clingo_ast_term_type_external_function = 6,
    clingo_ast_term_type_pool              = 7,
};
typedef int clingo_ast_term_type_t;

typedef struct clingo_ast_term_unary_operation clingo_ast_term_unary_operation_t;
typedef struct clingo_ast_term_binary_operation clingo_ast_term_binary_operation_t;
typedef struct clingo_ast_term_interval clingo_ast_term_interval_t;
typedef struct clingo_ast_term_function clingo_ast_term_function_t;
typedef struct clingo_ast_term_pool clingo_ast_term_pool_t;
typedef struct clingo_ast_term {
    clingo_location_t location;
    clingo_ast_term_type_t type;
    union {
        clingo_symbol_t symbol;
        char const *variable;
        clingo_ast_term_unary_operation_t const *unary_operation;
        clingo_ast_term_binary_operation_t const *binary_operation;
        clingo_ast_term_interval_t const *interval;
        clingo_ast_term_function_t const *function;
        clingo_ast_term_function_t const *external_function;
        clingo_ast_term_pool_t const *pool;
    };
} clingo_ast_term_t;

// unary operation

enum clingo_ast_term_unary_operator_type {
    clingo_ast_term_unary_operator_absolute = 0,
    clingo_ast_term_unary_operator_minus    = 1,
    clingo_ast_term_unary_operator_negate   = 2
};
typedef int clingo_ast_term_unary_operator_type_t;

struct clingo_ast_term_unary_operation {
    clingo_ast_term_unary_operator_type_t unary_operator;
    clingo_ast_term_t argument;
};

// binary operation

enum clingo_ast_term_binary_operator_type {
    clingo_ast_term_binary_operator_xor      = 0,
    clingo_ast_term_binary_operator_or       = 1,
    clingo_ast_term_binary_operator_and      = 2,
    clingo_ast_term_binary_operator_add      = 3,
    clingo_ast_term_binary_operator_subtract = 4,
    clingo_ast_term_binary_operator_multiply = 5,
    clingo_ast_term_binary_operator_divide   = 6,
    clingo_ast_term_binary_operator_modulo   = 7,

};
typedef int clingo_ast_term_binary_operator_type_t;

struct clingo_ast_term_binary_operation {
    clingo_ast_term_binary_operator_type_t binary_operator;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
};

// interval

struct clingo_ast_term_interval {
    clingo_ast_term_t left;
    clingo_ast_term_t right;
};

// function

struct clingo_ast_term_function {
    char const *name;
    clingo_ast_term_t *arguments;
    size_t size;
};

// pool

struct clingo_ast_term_pool {
    clingo_ast_term_t *arguments;
    size_t size;
};

// {{{2 csp

typedef struct clingo_ast_csp_multiply_term {
    clingo_location_t location;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
} clingo_ast_csp_multiply_term_t;

typedef struct clingo_ast_csp_guard {
    clingo_location_t location;
    clingo_ast_comparison_t comparison;
    clingo_ast_csp_multiply_term_t *terms;
    size_t *size;
} clingo_ast_csp_guard_t;

typedef struct clingo_ast_csp_literal {
    clingo_ast_csp_multiply_term_t *terms;
    size_t *terms_size;
    clingo_ast_csp_guard *guards;
    size_t *guards_size;
} clingo_ast_csp_literal_t;

// {{{2 ids

typedef struct clingo_ast_id {
    clingo_location_t location;
    char const *id;
} clingo_ast_id_t;

// {{{2 literals

typedef struct clingo_ast_literal_symbolic clingo_ast_literal_symbolic_t;
typedef struct clingo_ast_literal_comparison clingo_ast_literal_comparison_t;

enum clingo_ast_literal_type {
    clingo_ast_literal_type_boolean    = 0,
    clingo_ast_literal_type_symbolic   = 1,
    clingo_ast_literal_type_comparison = 2,
};
typedef int clingo_ast_literal_type_t;

typedef struct clingo_ast_literal {
    clingo_location_t location;
    clingo_ast_sign_t sign;
    clingo_ast_literal_type_t type;
    union {
        bool boolean;
        clingo_ast_term_t *symbolic;
        clingo_ast_literal_comparison_t *comparison;
    };
} clingo_ast_literal_t;

struct clingo_ast_literal_comparison {
    clingo_ast_comparison_t comparison;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
};

// {{{2 aggregates

enum clingo_ast_aggregate_function {
    // TODO
};
typedef int clingo_ast_aggregate_function_t;

typedef struct clingo_ast_conditional_literal {
    clingo_ast_literal_t literal;
    clingo_ast_literal_t *condition;
    size_t size;
} clingo_ast_conditional_literal_t;

typedef struct clingo_ast_aggregate {
    clingo_ast_conditional_literal_t *elements;
    size_t size;
} clingo_ast_aggregate_t;

typedef struct clingo_ast_aggregate_guard {
    clingo_ast_comparison_t comparison;
    clingo_ast_term_t term;
} clingo_ast_aggregate_guard_t;

typedef struct clingo_ast_body_aggregate_element {
    clingo_ast_term_t *tuple;
    size_t tuple_size;
    clingo_ast_literal_t *condition;
    size_t condition_size;
} clingo_ast_body_aggregate_element_t;

typedef struct clingo_ast_body_aggregate {
    clingo_ast_aggregate_function function;
    clingo_ast_body_aggregate_element *elements;
    size_t size;
} clingo_ast_body_aggregate_t;

typedef struct clingo_ast_head_aggregate_element {
    clingo_ast_term_t *tuple;
    size_t tuple_size;
    clingo_ast_conditional_literal_t conditional_literal;
} clingo_ast_head_aggregate_element_t;

typedef struct clingo_ast_head_aggregate {
    clingo_ast_aggregate_function function;
    clingo_ast_head_aggregate_element *elements;
    size_t size;
} clingo_ast_head_aggregate_t;

// {{{2 head literals

// TODO

// {{{2 body literals

enum clingo_ast_body_literal_type {
    // TODO
};

struct clingo_ast_body_literal {
    clingo_location_t location;
    clingo_ast_sign_t sign;
    clingo_ast_body_literal_type type;
    union {
        clingo_symbol_t symbolic;
        clingo_symbol_t conditional;
        clingo_ast_aggregate_t *aggregate;
        clingo_ast_head_aggregate_t *body_aggregate;
        // TODO: theory
        // TODO: disjoint
    };
};

/*
    // {{{2 heads
    virtual HdLitUid headlit(LitUid lit) = 0;
    virtual HdLitUid headaggr(Location const &loc, TheoryAtomUid atom) = 0;
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) = 0;
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) = 0;
    virtual HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) = 0;
    // {{{2 bodies
    virtual BdLitVecUid body() = 0;
    virtual BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) = 0;
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atom) = 0;
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) = 0;
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) = 0;
    virtual BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) = 0;
    virtual BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) = 0;
    // {{{2 csp constraint elements
    virtual CSPElemVecUid cspelemvec() = 0;
    virtual CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) = 0;
    // {{{2 statements
    virtual void rule(Location const &loc, HdLitUid head) = 0;
    virtual void rule(Location const &loc, HdLitUid head, BdLitVecUid body) = 0;
    virtual void define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &log) = 0;
    virtual void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) = 0;
    virtual void showsig(Location const &loc, Sig, bool csp) = 0;
    virtual void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) = 0;
    virtual void python(Location const &loc, String code) = 0;
    virtual void lua(Location const &loc, String code) = 0;
    virtual void block(Location const &loc, String name, IdVecUid args) = 0;
    virtual void external(Location const &loc, LitUid head, BdLitVecUid body) = 0;
    virtual void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) = 0;
    virtual void heuristic(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) = 0;
    virtual void project(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body) = 0;
    virtual void project(Location const &loc, Sig sig) = 0;
    // {{{2 theory atoms

    virtual TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) = 0;
    virtual TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermvalue(Location const &loc, Symbol val) = 0;
    virtual TheoryTermUid theorytermvar(Location const &loc, String var) = 0;

    virtual TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) = 0;
    virtual TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) = 0;

    virtual TheoryOpVecUid theoryops() = 0;
    virtual TheoryOpVecUid theoryops(TheoryOpVecUid ops, String op) = 0;

    virtual TheoryOptermVecUid theoryopterms() = 0;
    virtual TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, TheoryOptermUid opterm) = 0;
    virtual TheoryOptermVecUid theoryopterms(TheoryOptermUid opterm, TheoryOptermVecUid opterms) = 0;

    virtual TheoryElemVecUid theoryelems() = 0;
    virtual TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) = 0;

    virtual TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) = 0;
    virtual TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, String op, TheoryOptermUid opterm) = 0;

    // {{{2 theory definitions

    virtual TheoryOpDefUid theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) = 0;
    virtual TheoryOpDefVecUid theoryopdefs() = 0;
    virtual TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) = 0;

    virtual TheoryTermDefUid theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &log) = 0;
    virtual TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) = 0;
    virtual TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) = 0;

    virtual TheoryDefVecUid theorydefs() = 0;
    virtual TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) = 0;
    virtual TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) = 0;

    virtual void theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &log) = 0;

    // }}}2
*/

enum clingo_ast_rule_head_type {
};
typedef int clingo_ast_rule_head_type_t;

typedef struct clingo_ast_rule_head {
    clingo_ast_rule_head_type_t type;
    union {
        void *todo;
    };
} clingo_ast_rule_head_t;

typedef struct clingo_ast_rule {
    clingo_ast_rule_head head;
} clingo_rule_t;

enum clingo_ast_statement_type {
    clingo_ast_statement_type_rule = 0,
};
typedef int clingo_ast_statement_type_t;

typedef struct clingo_ast_statement {
    clingo_ast_statement_type_t type;
    clingo_location_t location;
    union {
        clingo_rule_t rule;
    };
} clingo_statement_t;

#ifdef __cplusplus
}
#endif

#endif
