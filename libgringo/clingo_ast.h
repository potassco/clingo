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

enum clingo_ast_comparison_operator {
    clingo_ast_comparison_operator_greater_than  = 0,
    clingo_ast_comparison_operator_less_than     = 1,
    clingo_ast_comparison_operator_less_equal    = 2,
    clingo_ast_comparison_operator_greater_equal = 3,
    clingo_ast_comparison_operator_not_equal     = 4,
    clingo_ast_comparison_operator_equal         = 5
};
typedef int clingo_ast_comparison_operator_t;

enum clingo_ast_sign {
    clingo_ast_sign_none            = 0,
    clingo_ast_sign_negation        = 1,
    clingo_ast_sign_double_negation = 2
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
    clingo_ast_term_type_pool              = 7
};
typedef int clingo_ast_term_type_t;

typedef struct clingo_ast_unary_operation clingo_ast_unary_operation_t;
typedef struct clingo_ast_binary_operation clingo_ast_binary_operation_t;
typedef struct clingo_ast_interval clingo_ast_term_interval_t;
typedef struct clingo_ast_function clingo_ast_function_t;
typedef struct clingo_ast_pool clingo_ast_pool_t;
typedef struct clingo_ast_term {
    clingo_location_t location;
    clingo_ast_term_type_t type;
    union {
        clingo_symbol_t symbol;
        char const *variable;
        clingo_ast_unary_operation_t const *unary_operation;
        clingo_ast_binary_operation_t const *binary_operation;
        clingo_ast_term_interval_t const *interval;
        clingo_ast_function_t const *function;
        clingo_ast_function_t const *external_function;
        clingo_ast_pool_t const *pool;
    };
} clingo_ast_term_t;

// unary operation

enum clingo_ast_unary_operator {
    clingo_ast_unary_operator_absolute = 0,
    clingo_ast_unary_operator_minus    = 1,
    clingo_ast_unary_operator_negate   = 2
};
typedef int clingo_ast_unary_operator_t;

struct clingo_ast_unary_operation {
    clingo_ast_unary_operator_t unary_operator;
    clingo_ast_term_t argument;
};

// binary operation

enum clingo_ast_binary_operator {
    clingo_ast_binary_operator_xor      = 0,
    clingo_ast_binary_operator_or       = 1,
    clingo_ast_binary_operator_and      = 2,
    clingo_ast_binary_operator_add      = 3,
    clingo_ast_binary_operator_subtract = 4,
    clingo_ast_binary_operator_multiply = 5,
    clingo_ast_binary_operator_divide   = 6,
    clingo_ast_binary_operator_modulo   = 7

};
typedef int clingo_ast_binary_operator_t;

struct clingo_ast_binary_operation {
    clingo_ast_binary_operator_t binary_operator;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
};

// interval

struct clingo_ast_interval {
    clingo_ast_term_t left;
    clingo_ast_term_t right;
};

// function

struct clingo_ast_function {
    char const *name;
    clingo_ast_term_t *arguments;
    size_t size;
};

// pool

struct clingo_ast_pool {
    clingo_ast_term_t *arguments;
    size_t size;
};

// {{{2 csp

typedef struct clingo_ast_csp_multiply_term {
    clingo_location_t location;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
} clingo_ast_csp_multiply_term_t;

typedef struct clingo_ast_csp_add_term {
    clingo_location_t location;
    clingo_ast_csp_multiply_term_t *terms;
    size_t size;
} clingo_ast_csp_add_term_t;

typedef struct clingo_ast_csp_guard {
    clingo_location_t location;
    clingo_ast_comparison_operator_t comparison;
    clingo_ast_csp_add_term term;
} clingo_ast_csp_guard_t;

typedef struct clingo_ast_csp_literal {
    clingo_ast_csp_add_term_t term;
    clingo_ast_csp_guard const *guards;
    size_t size;
} clingo_ast_csp_literal_t;

// {{{2 ids

typedef struct clingo_ast_id {
    clingo_location_t location;
    char const *id;
} clingo_ast_id_t;

// {{{2 literals

typedef struct clingo_ast_comparison {
    clingo_ast_comparison_operator_t comparison;
    clingo_ast_term_t left;
    clingo_ast_term_t right;
} clingo_ast_comparison_t;

enum clingo_ast_literal_type {
    clingo_ast_literal_type_boolean    = 0,
    clingo_ast_literal_type_symbolic   = 1,
    clingo_ast_literal_type_comparison = 2
};
typedef int clingo_ast_literal_type_t;

typedef struct clingo_ast_literal {
    clingo_location_t location;
    clingo_ast_sign_t sign;
    clingo_ast_literal_type_t type;
    union {
        bool boolean;
        clingo_ast_term_t const *symbol;
        clingo_ast_comparison_t const *comparison;
    };
} clingo_ast_literal_t;

// {{{2 aggregates

enum clingo_ast_aggregate_function {
    clingo_ast_aggregate_function_count = 0,
    clingo_ast_aggregate_function_sum   = 1,
    clingo_ast_aggregate_function_sump  = 2,
    clingo_ast_aggregate_function_min   = 3,
    clingo_ast_aggregate_function_max   = 4
};
typedef int clingo_ast_aggregate_function_t;

typedef struct clingo_ast_aggregate_guard {
    clingo_ast_comparison_operator_t comparison;
    clingo_ast_term_t term;
} clingo_ast_aggregate_guard_t;

typedef struct clingo_ast_conditional_literal {
    clingo_ast_literal_t literal;
    clingo_ast_literal_t const *condition;
    size_t size;
} clingo_ast_conditional_literal_t;

// lparse-style aggregate

typedef struct clingo_ast_aggregate {
    clingo_ast_conditional_literal_t const *elements;
    size_t size;
    clingo_ast_aggregate_guard const *left_guard;
    clingo_ast_aggregate_guard const *right_guard;
} clingo_ast_aggregate_t;

// body aggregate

typedef struct clingo_ast_body_aggregate_element {
    clingo_ast_term_t *tuple;
    size_t tuple_size;
    clingo_ast_literal_t const *condition;
    size_t condition_size;
} clingo_ast_body_aggregate_element_t;

typedef struct clingo_ast_body_aggregate {
    clingo_ast_aggregate_function function;
    clingo_ast_body_aggregate_element const *elements;
    size_t size;
    clingo_ast_aggregate_guard const *left_guard;
    clingo_ast_aggregate_guard const *right_guard;
} clingo_ast_body_aggregate_t;

// head aggregate

typedef struct clingo_ast_head_aggregate_element {
    clingo_ast_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_conditional_literal_t conditional_literal;
} clingo_ast_head_aggregate_element_t;

typedef struct clingo_ast_head_aggregate {
    clingo_ast_aggregate_function function;
    clingo_ast_head_aggregate_element const *elements;
    size_t size;
    clingo_ast_aggregate_guard const *left_guard;
    clingo_ast_aggregate_guard const *right_guard;
} clingo_ast_head_aggregate_t;

// disjunction

typedef struct clingo_ast_disjunction {
    clingo_ast_conditional_literal_t const *elements;
    size_t size;
} clingo_ast_disjunction_t;

// disjoint

typedef struct clingo_ast_disjoint_element {
    clingo_location_t location;
    clingo_ast_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_csp_add_term_t term;
    clingo_ast_literal_t *const condition;
    size_t condition_size;
} clingo_ast_disjoint_element_t;

typedef struct clingo_ast_disjoint {
    clingo_ast_disjoint_element const *elements;
    size_t size;
} clingo_ast_disjoint_t;

// {{{2 theory atom

enum clingo_ast_theory_term_type {
    clingo_ast_theory_term_type_symbol              = 0,
    clingo_ast_theory_term_type_variable            = 1,
    clingo_ast_theory_term_type_tuple               = 2,
    clingo_ast_theory_term_type_list                = 3,
    clingo_ast_theory_term_type_set                 = 4,
    clingo_ast_theory_term_type_function            = 5,
    clingo_ast_theory_term_type_unparsed_term_array = 6
};
typedef int clingo_ast_theory_term_type_t;

typedef struct clingo_ast_theory_function clingo_ast_theory_function_t;
typedef struct clingo_ast_theory_term_array clingo_ast_theory_term_array_t;

typedef struct clingo_ast_theory_term {
    clingo_location_t location;
    clingo_ast_theory_term_type_t type;
    union {
        clingo_symbol_t symbol;
        char const *variable;
        clingo_ast_theory_term_array_t const *tuple;
        clingo_ast_theory_term_array_t const *list;
        clingo_ast_theory_term_array_t const *set;
        clingo_ast_theory_function_t const *function;
    };
} clingo_ast_theory_term_t;

struct clingo_ast_theory_term_array {
    clingo_ast_theory_term_t const *terms;
    size_t size;
};

struct clingo_ast_theory_function {
    char const *name;
    clingo_ast_theory_term_t const *arguments;
    size_t size;
};

typedef struct clingo_ast_theory_unparsed_term {
    char const *const *operators;
    size_t size;
    clingo_ast_theory_term_t const *term;
} clingo_ast_theory_unparsed_term_t;

struct clingo_ast_theory_unparsed_term_array {
    clingo_ast_theory_unparsed_term_t const *term;
    size_t size;
};

typedef struct clingo_ast_theory_atom_element {
    clingo_ast_theory_term_t const *tuple;
    size_t tuple_size;
    clingo_literal_t const *condition;
    size_t condition_size;
} clingo_ast_theory_atom_element_t;

typedef struct clingo_ast_theory_guard {
    char const *operator_name;
    clingo_ast_theory_term term;
} clingo_ast_theory_guard_t;

typedef struct clingo_ast_theory_atom {
    clingo_ast_term_t term;
    clingo_ast_theory_atom_element const *elements;
    size_t size;
    clingo_ast_theory_guard const *guard;
} clingo_ast_theory_atom_t;

// {{{2 head literals

enum clingo_ast_head_literal_type {
    clingo_ast_head_literal_type_symbolic       = 0,
    clingo_ast_head_literal_type_disjunction    = 1,
    clingo_ast_head_literal_type_aggregate      = 2,
    clingo_ast_head_literal_type_head_aggregate = 3,
    clingo_ast_head_literal_type_theory         = 4
};
typedef int clingo_ast_head_literal_type_t;

typedef struct clingo_ast_head_literal {
    clingo_location_t location;
    clingo_ast_head_literal_type_t type;
    union {
        clingo_symbol_t symbolic;
        clingo_ast_disjunction_t const *disjunction;
        clingo_ast_aggregate_t const *aggregate;
        clingo_ast_head_aggregate_t const *head_aggregate;
        clingo_ast_theory_atom_t const *theory_atom;
    };
} clingo_ast_head_literal_t;

// {{{2 body literals

enum clingo_ast_body_literal_type {
    clingo_ast_body_literal_type_symbolic       = 0,
    clingo_ast_body_literal_type_conditional    = 1,
    clingo_ast_body_literal_type_aggregate      = 2,
    clingo_ast_body_literal_type_body_aggregate = 3,
    clingo_ast_body_literal_type_theory         = 4,
    clingo_ast_body_literal_type_disjoint       = 5
};
typedef int clingo_ast_body_literal_type_t;

typedef struct clingo_ast_body_literal {
    clingo_location_t location;
    clingo_ast_sign_t sign;
    clingo_ast_body_literal_type_t type;
    union {
        clingo_symbol_t symbolic;
        // Note: conditional literals must not have signs!!!
        clingo_ast_conditional_literal_t const *conditional;
        clingo_ast_aggregate_t const *aggregate;
        clingo_ast_body_aggregate_t const *body_aggregate;
        clingo_ast_theory_atom_t const *theory_atom;
        clingo_ast_disjoint_t const *disjoint;
    };
} clingo_ast_body_literal_t;

// {{{2 theory definitions

enum clingo_ast_theory_operator_type {
     clingo_ast_theory_operator_type_unary        = 0,
     clingo_ast_theory_operator_type_binary_left  = 1,
     clingo_ast_theory_operator_type_binary_right = 2
};
typedef int clingo_ast_theory_operator_type_t;

typedef struct clingo_ast_theory_operator_definition {
    clingo_location_t location;
    char const *name;
    unsigned priority;
    clingo_ast_theory_operator_type_t type;
} clingo_ast_theory_operator_definition_t;

typedef struct clingo_ast_theory_term_definition {
    clingo_location_t location;
    char const *name;
    clingo_ast_theory_operator_definition_t const *operators;
    size_t size;
} clingo_ast_theory_term_definition_t;

typedef struct clingo_ast_theory_guard_definition {
    char const *const *operators;
    char const *guard;
} clingo_ast_theory_guard_definition_t;

typedef struct clingo_ast_theory_atom_definition {
    clingo_location_t location;
    char const *name;
    unsigned arity;
    char const *elements;
    clingo_ast_theory_guard_definition_t const *guard;
} clingo_ast_theory_atom_definition_t;

typedef struct clingo_ast_theory_definition {
    char const *name;
    clingo_ast_theory_atom_definition const *atoms;
    size_t size;
} clingo_ast_theory_definition_t;

// {{{2 statements

// rule

typedef struct clingo_ast_rule {
    clingo_ast_head_literal_t head;
    clingo_ast_head_literal_t const *body;
    size_t size;
} clingo_ast_rule_t;

// show

typedef struct clingo_ast_show_signature {
    clingo_signature_t signature;
    bool csp;
} clingo_ast_show_signature_t;

typedef struct clingo_ast_show_term {
    clingo_ast_term_t const *term;
    clingo_ast_body_literal_t const *body;
    size_t size;
    bool csp;
} clingo_ast_show_term_t;

// minimize

typedef struct clingo_ast_minimize {
    clingo_ast_term_t const *weight;
    clingo_ast_term_t const *priority;
    clingo_ast_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_body_literal_t const *body;
    size_t body_size;
} clingo_ast_minimize_t;

// script

enum clingo_ast_script_type {
    clingo_ast_script_type_lua    = 0,
    clingo_ast_script_type_python = 1
};
typedef int clingo_ast_script_type_t;

typedef struct clingo_ast_script {
    clingo_ast_script_type_t type;
    char const *code;
} clingo_ast_script_t;

// program

typedef struct clingo_ast_program {
    char const *name;
    clingo_ast_id_t const *parameters;
    size_t size;
} clingo_ast_program_t;

// external

typedef struct clingo_ast_external {
    clingo_ast_term_t atom;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_external_t;

// edge

typedef struct clingo_ast_edge {
    clingo_ast_term_t u;
    clingo_ast_term_t v;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_edge_t;

// heuristic

typedef struct clingo_ast_heuristic {
    clingo_ast_term_t atom;
    clingo_ast_body_literal_t const *body;
    size_t size;
    clingo_ast_term_t bias;
    clingo_ast_term_t priority;
    clingo_ast_term_t modifier;
} clingo_ast_heuristic_t;

// project

typedef struct clingo_ast_project {
    clingo_ast_term_t atom;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_project_t;

// statement

enum clingo_ast_statement_type {
    clingo_ast_statement_type_rule              = 0,
    clingo_ast_statement_type_show_signature    = 1,
    clingo_ast_statement_type_show_term         = 2,
    clingo_ast_statement_type_minimize          = 3,
    clingo_ast_statement_type_script            = 4,
    clingo_ast_statement_type_program           = 5,
    clingo_ast_statement_type_external          = 6,
    clingo_ast_statement_type_edge              = 7,
    clingo_ast_statement_type_heuristic         = 8,
    clingo_ast_statement_type_project           = 9,
    clingo_ast_statement_type_project_signatrue = 10,
    clingo_ast_statement_type_theory_definition = 11
};
typedef int clingo_ast_statement_type_t;

typedef struct clingo_ast_statement {
    clingo_location_t location;
    clingo_ast_statement_type_t type;
    union {
        clingo_ast_rule_t const *rule;
        clingo_ast_show_signature_t const *show_signature;
        clingo_ast_show_term_t const *show_term;
        clingo_ast_minimize_t const *minimize;
        clingo_ast_script_t const *script;
        clingo_ast_program_t const *program;
        clingo_ast_external_t const *external;
        clingo_ast_edge_t const *edge;
        clingo_ast_heuristic_t const *heuristic;
        clingo_ast_project_t const *project;
        clingo_signature_t project_signature;
        clingo_ast_theory_definition_t const *theory_definition;
    };
} clingo_statement_t;

// }}}1

#ifdef __cplusplus
}
#endif

#endif
