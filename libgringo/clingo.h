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

#ifndef CLINGO_H
#define CLINGO_H

#define CLINGO_VERSION "5.0.0"
#define CLINGO_VERSION_MAJOR 5
#define CLINGO_VERSION_MINOR 0
#define CLINGO_VERSION_REVISION 0

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// {{{1 basic types

typedef int32_t clingo_literal_t;
typedef uint32_t clingo_id_t;
typedef int32_t clingo_weight_t;
typedef uint32_t clingo_atom_t;

// {{{1 errors and warnings

// Note: Errors can be recovered from. If a control function fails, the
//       corresponding control object must be destroyed.
enum clingo_error {
    clingo_error_success   = 0,
    clingo_error_fatal     = 1,
    clingo_error_runtime   = 2,
    clingo_error_logic     = 3,
    clingo_error_bad_alloc = 4,
    clingo_error_unknown   = 5
};
typedef int clingo_error_t;

char const *clingo_error_string(clingo_error_t code);
char const *clingo_error_message();

enum clingo_warning {
    clingo_warning_operation_undefined = 0, //< Undefined arithmetic operation or weight of aggregate.
    clingo_warning_fatal               = 1, //< To report multiple errors, a corresponding error is raised later
    clingo_warning_atom_undefined      = 2, //< Undefined atom in program.
    clingo_warning_file_included       = 3, //< Same file included multiple times.
    clingo_warning_variable_unbounded  = 4, //< CSP Domain undefined.
    clingo_warning_global_variable     = 5, //< Global variable in tuple of aggregate element.
    clingo_warning_other               = 6, //< Other kinds of warnings
};
typedef int clingo_warning_t;

char const *clingo_warning_string(clingo_warning_t code);
typedef void clingo_logger_t(clingo_warning_t, char const *, void *);

// {{{1 signature

typedef uint64_t clingo_signature_t;

clingo_error_t clingo_signature_create(char const *name, uint32_t arity, bool positive, clingo_signature_t *ret);
char const *clingo_signature_name(clingo_signature_t sig);
uint32_t clingo_signature_arity(clingo_signature_t sig);
bool clingo_signature_is_positive(clingo_signature_t sig);
bool clingo_signature_is_negative(clingo_signature_t sig);
size_t clingo_signature_hash(clingo_signature_t sig);
bool clingo_signature_is_equal_to(clingo_signature_t a, clingo_signature_t b);
bool clingo_signature_is_less_than(clingo_signature_t a, clingo_signature_t b);

// {{{1 symbol

enum clingo_symbol_type {
    clingo_symbol_type_infimum  = 0,
    clingo_symbol_type_number   = 1,
    clingo_symbol_type_string   = 4,
    clingo_symbol_type_function = 5,
    clingo_symbol_type_supremum = 7
};
typedef int clingo_symbol_type_t;

typedef uint64_t clingo_symbol_t;

typedef struct clingo_symbolic_literal {
    clingo_symbol_t symbol;
    bool positive;
} clingo_symbolic_literal_t;

// construction

void clingo_symbol_create_num(int num, clingo_symbol_t *sym);
void clingo_symbol_create_supremum(clingo_symbol_t *sym);
void clingo_symbol_create_infimum(clingo_symbol_t *sym);
clingo_error_t clingo_symbol_create_string(char const *str, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_create_id(char const *id, bool positive, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_create_function(char const *name, clingo_symbol_t const *args, size_t n, bool positive, clingo_symbol_t *sym);

// inspection

clingo_error_t clingo_symbol_number(clingo_symbol_t sym, int *num);
clingo_error_t clingo_symbol_name(clingo_symbol_t sym, char const **name);
clingo_error_t clingo_symbol_string(clingo_symbol_t sym, char const **str);
clingo_error_t clingo_symbol_is_positive(clingo_symbol_t sym, bool *positive);
clingo_error_t clingo_symbol_is_negative(clingo_symbol_t sym, bool *negative);
clingo_error_t clingo_symbol_arguments(clingo_symbol_t sym, clingo_symbol_t const **args, size_t *n);
clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t sym);
clingo_error_t clingo_symbol_to_string_size(clingo_symbol_t sym, size_t *n);
clingo_error_t clingo_symbol_to_string(clingo_symbol_t sym, char *ret, size_t n);

// comparison

size_t clingo_symbol_hash(clingo_symbol_t sym);
bool clingo_symbol_is_equal_to(clingo_symbol_t a, clingo_symbol_t b);
bool clingo_symbol_is_less_than(clingo_symbol_t a, clingo_symbol_t b);

// {{{1 solve control

typedef struct clingo_solve_control clingo_solve_control_t;
clingo_error_t clingo_solve_control_thread_id(clingo_solve_control_t *ctl, clingo_id_t *ret);
clingo_error_t clingo_solve_control_add_clause(clingo_solve_control_t *ctl, clingo_symbolic_literal_t const *clause, size_t n);

// {{{1 model

enum clingo_model_type {
    clingo_model_type_stable_model          = 0,
    clingo_model_type_brave_consequences    = 1,
    clingo_model_type_cautious_consequences = 2
};
typedef int clingo_model_type_t;

enum clingo_show_type {
    clingo_show_type_csp        = 1,
    clingo_show_type_shown      = 2,
    clingo_show_type_atoms      = 4,
    clingo_show_type_terms      = 8,
    clingo_show_type_extra      = 16,
    clingo_show_type_all        = 31,
    clingo_show_type_complement = 32
};
typedef unsigned clingo_show_type_bitset_t;

typedef struct clingo_model clingo_model_t;
clingo_error_t clingo_model_atoms_size(clingo_model_t *m, clingo_show_type_bitset_t show, size_t *n);
clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_bitset_t show, clingo_symbol_t *ret, size_t n);
clingo_error_t clingo_model_contains(clingo_model_t *m, clingo_symbol_t atom, bool *ret);
clingo_error_t clingo_model_context(clingo_model_t *m, clingo_solve_control_t **ret);
clingo_error_t clingo_model_number(clingo_model_t *m, uint64_t *n);
clingo_error_t clingo_model_optimality_proven(clingo_model_t *m, bool *ret);
clingo_error_t clingo_model_cost_size(clingo_model_t *m, size_t *n);
clingo_error_t clingo_model_cost(clingo_model_t *m, int64_t *ret, size_t n);
clingo_error_t clingo_model_type(clingo_model_t *m, clingo_model_type_t *ret);

// {{{1 solve result

enum clingo_solve_result {
    clingo_solve_result_satisfiable   = 1,
    clingo_solve_result_unsatisfiable = 2,
    clingo_solve_result_exhausted     = 4,
    clingo_solve_result_interrupted   = 8
};
typedef unsigned clingo_solve_result_bitset_t;

// {{{1 truth value

enum clingo_truth_value {
    clingo_truth_value_free  = 0,
    clingo_truth_value_true  = 1,
    clingo_truth_value_false = 2
};
typedef int clingo_truth_value_t;

// {{{1 solve iter

typedef struct clingo_solve_iteratively clingo_solve_iteratively_t;
clingo_error_t clingo_solve_iteratively_next(clingo_solve_iteratively_t *it, clingo_model_t **m);
clingo_error_t clingo_solve_iteratively_get(clingo_solve_iteratively_t *it, clingo_solve_result_bitset_t *ret);
clingo_error_t clingo_solve_iteratively_close(clingo_solve_iteratively_t *it);

// {{{1 solve async

typedef struct clingo_solve_async clingo_solve_async_t;
clingo_error_t clingo_solve_async_cancel(clingo_solve_async_t *async);
clingo_error_t clingo_solve_async_get(clingo_solve_async_t *async, clingo_solve_result_bitset_t *ret);
clingo_error_t clingo_solve_async_wait(clingo_solve_async_t *async, double timeout, bool *ret);

// {{{1 symbolic atoms

typedef uint64_t clingo_symbolic_atom_iterator_t;

typedef struct clingo_symbolic_atoms clingo_symbolic_atoms_t;
clingo_error_t clingo_symbolic_atoms_begin(clingo_symbolic_atoms_t *dom, clingo_signature_t const *sig, clingo_symbolic_atom_iterator_t *ret);
clingo_error_t clingo_symbolic_atoms_end(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t *ret);
clingo_error_t clingo_symbolic_atoms_find(clingo_symbolic_atoms_t *dom, clingo_symbol_t atom, clingo_symbolic_atom_iterator_t *ret);
clingo_error_t clingo_symbolic_atoms_iterator_is_equal_to(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t it, clingo_symbolic_atom_iterator_t jt, bool *ret);
clingo_error_t clingo_symbolic_atoms_signatures_size(clingo_symbolic_atoms_t *dom, size_t *n);
clingo_error_t clingo_symbolic_atoms_signatures(clingo_symbolic_atoms_t *dom, clingo_signature_t *ret, size_t n);
clingo_error_t clingo_symbolic_atoms_size(clingo_symbolic_atoms_t *dom, size_t *ret);
clingo_error_t clingo_symbolic_atoms_symbol(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, clingo_symbol_t *sym);
clingo_error_t clingo_symbolic_atoms_literal(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, clingo_literal_t *lit);
clingo_error_t clingo_symbolic_atoms_is_fact(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, bool *fact);
clingo_error_t clingo_symbolic_atoms_is_external(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, bool *external);
clingo_error_t clingo_symbolic_atoms_next(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, clingo_symbolic_atom_iterator_t *next);
clingo_error_t clingo_symbolic_atoms_is_valid(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iterator_t atm, bool *valid);

// {{{1 theory atoms

enum clingo_theory_term_type {
    clingo_theory_term_type_tuple,
    clingo_theory_term_type_list,
    clingo_theory_term_type_set,
    clingo_theory_term_type_function,
    clingo_theory_term_type_number,
    clingo_theory_term_type_symbol
};
typedef int clingo_theory_term_type_t;

typedef struct clingo_theory_atoms clingo_theory_atoms_t;
clingo_error_t clingo_theory_atoms_term_type(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_theory_term_type_t *ret);
clingo_error_t clingo_theory_atoms_term_number(clingo_theory_atoms_t *atoms, clingo_id_t value, int *num);
clingo_error_t clingo_theory_atoms_term_name(clingo_theory_atoms_t *atoms, clingo_id_t value, char const **ret);
clingo_error_t clingo_theory_atoms_term_arguments(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n);
clingo_error_t clingo_theory_atoms_element_tuple(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n);
clingo_error_t clingo_theory_atoms_element_condition(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_literal_t const **ret, size_t *n);
clingo_error_t clingo_theory_atoms_element_condition_literal(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_literal_t *ret);
clingo_error_t clingo_theory_atoms_atom_elements(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n);
clingo_error_t clingo_theory_atoms_atom_term(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t *ret);
clingo_error_t clingo_theory_atoms_atom_has_guard(clingo_theory_atoms_t *atoms, clingo_id_t value, bool *ret);
clingo_error_t clingo_theory_atoms_atom_literal(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_literal_t *ret);
clingo_error_t clingo_theory_atoms_atom_guard(clingo_theory_atoms_t *atoms, clingo_id_t value, char const **ret_op, clingo_id_t *ret_term);
clingo_error_t clingo_theory_atoms_size(clingo_theory_atoms_t *atoms, size_t *ret);
clingo_error_t clingo_theory_atoms_term_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t value, size_t *n);
clingo_error_t clingo_theory_atoms_term_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t n);
clingo_error_t clingo_theory_atoms_element_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t value, size_t *n);
clingo_error_t clingo_theory_atoms_element_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t n);
clingo_error_t clingo_theory_atoms_atom_to_string_size(clingo_theory_atoms_t *atoms, clingo_id_t value, size_t *n);
clingo_error_t clingo_theory_atoms_atom_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t n);

// {{{1 propagate init

typedef struct clingo_propagate_init clingo_propagate_init_t;
clingo_error_t clingo_propagate_init_map_literal(clingo_propagate_init_t *init, clingo_literal_t lit, clingo_literal_t *ret);
clingo_error_t clingo_propagate_init_add_watch(clingo_propagate_init_t *init, clingo_literal_t lit);
int clingo_propagate_init_number_of_threads(clingo_propagate_init_t *init);
clingo_error_t clingo_propagate_init_symbolic_atoms(clingo_propagate_init_t *init, clingo_symbolic_atoms_t **ret);
clingo_error_t clingo_propagate_init_theory_atoms(clingo_propagate_init_t *init, clingo_theory_atoms_t **ret);

// {{{1 assignment

typedef struct clingo_assignment clingo_assignment_t;
bool clingo_assignment_has_conflict(clingo_assignment_t *ass);
uint32_t clingo_assignment_decision_level(clingo_assignment_t *ass);
bool clingo_assignment_has_literal(clingo_assignment_t *ass, clingo_literal_t lit);
clingo_error_t clingo_assignment_truth_value(clingo_assignment_t *ass, clingo_literal_t lit, clingo_truth_value_t *ret);
clingo_error_t clingo_assignment_level(clingo_assignment_t *ass, clingo_literal_t lit, uint32_t *ret);
clingo_error_t clingo_assignment_decision(clingo_assignment_t *ass, uint32_t level, clingo_literal_t *ret);
clingo_error_t clingo_assignment_is_fixed(clingo_assignment_t *ass, clingo_literal_t lit, bool *ret);
clingo_error_t clingo_assignment_is_true(clingo_assignment_t *ass, clingo_literal_t lit, bool *ret);
clingo_error_t clingo_assignment_is_false(clingo_assignment_t *ass, clingo_literal_t lit, bool *ret);

// {{{1 propagate control

enum clingo_clause_type {
    clingo_clause_type_learnt          = 0,
    clingo_clause_type_static          = 1,
    clingo_clause_type_volatile        = 2,
    clingo_clause_type_volatile_static = 3
};
typedef int clingo_clause_type_t;

typedef struct clingo_propagate_control clingo_propagate_control_t;
clingo_id_t clingo_propagate_control_thread_id(clingo_propagate_control_t *ctl);
clingo_assignment_t *clingo_propagate_control_assignment(clingo_propagate_control_t *ctl);
clingo_error_t clingo_propagate_control_add_clause(clingo_propagate_control_t *ctl, clingo_literal_t const *clause, size_t n, clingo_clause_type_t prop, bool *ret);
clingo_error_t clingo_propagate_control_propagate(clingo_propagate_control_t *ctl, bool *ret);

// {{{1 propagator

typedef struct clingo_propagator {
    clingo_error_t (*init) (clingo_propagate_init_t *ctl, void *data);
    clingo_error_t (*propagate) (clingo_propagate_control_t *ctl, clingo_literal_t const *changes, size_t n, void *data);
    clingo_error_t (*undo) (clingo_propagate_control_t *ctl, clingo_literal_t const *changes, size_t n, void *data);
    clingo_error_t (*check) (clingo_propagate_control_t *ctl, void *data);
} clingo_propagator_t;

// {{{1 backend

enum clingo_heuristic_type {
    clingo_heuristic_type_level  = 0,
    clingo_heuristic_type_sign   = 1,
    clingo_heuristic_type_factor = 2,
    clingo_heuristic_type_init   = 3,
    clingo_heuristic_type_true   = 4,
    clingo_heuristic_type_false  = 5
};
typedef int clingo_heuristic_type_t;

enum clingo_external_type {
    clingo_external_type_free    = 0,
    clingo_external_type_true    = 1,
    clingo_external_type_false   = 2,
    clingo_external_type_release = 3,
};
typedef int clingo_external_type_t;

typedef struct clingo_weighted_literal {
    clingo_literal_t literal;
    clingo_weight_t weight;
} clingo_weighted_literal_t;

typedef struct clingo_backend clingo_backend_t;
clingo_error_t clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_literal_t const *body, size_t body_n);
clingo_error_t clingo_backend_weight_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_weight_t lower, clingo_weighted_literal_t const *body, size_t body_n);
clingo_error_t clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t prio, clingo_weighted_literal_t const* lits, size_t lits_n);
clingo_error_t clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms, size_t n);
clingo_error_t clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom, clingo_external_type_t v);
clingo_error_t clingo_backend_assume(clingo_backend_t *backend, clingo_literal_t const *literals, size_t n);
clingo_error_t clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_literal_t const *condition, size_t condition_n);
clingo_error_t clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v, clingo_literal_t const *condition, size_t condition_n);
clingo_error_t clingo_backend_add_atom(clingo_backend_t *backend, clingo_atom_t *ret);

// {{{1 configuration

enum clingo_configuration_type {
    clingo_configuration_type_value = 1,
    clingo_configuration_type_array = 2,
    clingo_configuration_type_map   = 4
};
typedef unsigned clingo_configuration_type_bitset_t;

typedef struct clingo_configuration clingo_configuration_t;
clingo_error_t clingo_configuration_array_at(clingo_configuration_t *conf, clingo_id_t key, size_t idx, clingo_id_t *ret);
clingo_error_t clingo_configuration_array_size(clingo_configuration_t *conf, clingo_id_t key, size_t *ret);
clingo_error_t clingo_configuration_map_at(clingo_configuration_t *conf, clingo_id_t key, char const *name, clingo_id_t* subkey);
clingo_error_t clingo_configuration_map_size(clingo_configuration_t *conf, clingo_id_t key, size_t* subkey);
clingo_error_t clingo_configuration_map_subkey_name(clingo_configuration_t *conf, clingo_id_t key, size_t index, char const **name);
clingo_error_t clingo_configuration_value_is_assigned(clingo_configuration_t *conf, clingo_id_t key, bool *ret);
clingo_error_t clingo_configuration_value_get_size(clingo_configuration_t *conf, clingo_id_t key, size_t *n);
clingo_error_t clingo_configuration_value_get(clingo_configuration_t *conf, clingo_id_t key, char *ret, size_t n);
clingo_error_t clingo_configuration_value_set(clingo_configuration_t *conf, clingo_id_t key, char const *val);
clingo_error_t clingo_configuration_root(clingo_configuration_t *conf, clingo_id_t *ret);
clingo_error_t clingo_configuration_type(clingo_configuration_t *conf, clingo_id_t key, clingo_configuration_type_bitset_t *ret);
clingo_error_t clingo_configuration_description(clingo_configuration_t *conf, clingo_id_t key, char const **ret);

// {{{1 statistics

enum clingo_statistics_type {
    clingo_statistics_type_value = 0,
    clingo_statistics_type_array = 1,
    clingo_statistics_type_map   = 2
};
typedef int clingo_statistics_type_t;

typedef struct clingo_statistic clingo_statistics_t;
clingo_error_t clingo_statistics_array_at(clingo_statistics_t *stats, clingo_id_t key, size_t index, clingo_id_t *ret);
clingo_error_t clingo_statistics_array_size(clingo_statistics_t *stats, clingo_id_t key, size_t *ret);
clingo_error_t clingo_statistics_map_at(clingo_statistics_t *stats, clingo_id_t key, char const *name, clingo_id_t *ret);
clingo_error_t clingo_statistics_map_size(clingo_statistics_t *stats, clingo_id_t key, size_t *n);
clingo_error_t clingo_statistics_map_subkey_name(clingo_statistics_t *stats, clingo_id_t key, size_t index, char const **name);
clingo_error_t clingo_statistics_root(clingo_statistics_t *stats, clingo_id_t *ret);
clingo_error_t clingo_statistics_type(clingo_statistics_t *stats, clingo_id_t key, clingo_statistics_type_t *ret);
clingo_error_t clingo_statistics_value_get(clingo_statistics_t *stats, clingo_id_t key, double *value);

// {{{1 ast

typedef struct clingo_location {
    char const *begin_file;
    char const *end_file;
    size_t begin_line;
    size_t end_line;
    size_t begin_column;
    size_t end_column;
} clingo_location_t;

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
typedef struct clingo_ast_interval clingo_ast_interval_t;
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
        clingo_ast_interval_t const *interval;
        clingo_ast_function_t const *function;
        clingo_ast_function_t const *external_function;
        clingo_ast_pool_t const *pool;
    };
} clingo_ast_term_t;

// unary operation

enum clingo_ast_unary_operator {
    clingo_ast_unary_operator_minus    = 0,
    clingo_ast_unary_operator_negate   = 1,
    clingo_ast_unary_operator_absolute = 2
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

// TODO: rename multiply -> product
typedef struct clingo_ast_csp_multiply_term {
    clingo_location_t location;
    clingo_ast_term_t coefficient;
    clingo_ast_term_t const *variable;
} clingo_ast_csp_multiply_term_t;

// TODO: rename add -> sum
typedef struct clingo_ast_csp_add_term {
    clingo_location_t location;
    clingo_ast_csp_multiply_term_t *terms;
    size_t size;
} clingo_ast_csp_add_term_t;

typedef struct clingo_ast_csp_guard {
    clingo_ast_comparison_operator_t comparison;
    clingo_ast_csp_add_term_t term;
} clingo_ast_csp_guard_t;

typedef struct clingo_ast_csp_literal {
    clingo_ast_csp_add_term_t term;
    clingo_ast_csp_guard_t const *guards;
    // NOTE: size must be at least one
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
    clingo_ast_literal_type_comparison = 2,
    clingo_ast_literal_type_csp        = 3
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
        clingo_ast_csp_literal_t const *csp;
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
    clingo_ast_aggregate_guard_t const *left_guard;
    clingo_ast_aggregate_guard_t const *right_guard;
} clingo_ast_aggregate_t;

// body aggregate

typedef struct clingo_ast_body_aggregate_element {
    clingo_ast_term_t *tuple;
    size_t tuple_size;
    clingo_ast_literal_t const *condition;
    size_t condition_size;
} clingo_ast_body_aggregate_element_t;

typedef struct clingo_ast_body_aggregate {
    clingo_ast_aggregate_function_t function;
    clingo_ast_body_aggregate_element_t const *elements;
    size_t size;
    clingo_ast_aggregate_guard_t const *left_guard;
    clingo_ast_aggregate_guard_t const *right_guard;
} clingo_ast_body_aggregate_t;

// head aggregate

typedef struct clingo_ast_head_aggregate_element {
    clingo_ast_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_conditional_literal_t conditional_literal;
} clingo_ast_head_aggregate_element_t;

typedef struct clingo_ast_head_aggregate {
    clingo_ast_aggregate_function_t function;
    clingo_ast_head_aggregate_element_t const *elements;
    size_t size;
    clingo_ast_aggregate_guard_t const *left_guard;
    clingo_ast_aggregate_guard_t const *right_guard;
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
    clingo_ast_literal_t const *condition;
    size_t condition_size;
} clingo_ast_disjoint_element_t;

typedef struct clingo_ast_disjoint {
    clingo_ast_disjoint_element_t const *elements;
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
typedef struct clingo_ast_theory_unparsed_term_array clingo_ast_theory_unparsed_term_array_t;

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
        clingo_ast_theory_unparsed_term_array_t const *unparsed_array;
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
    clingo_ast_theory_term_t term;
} clingo_ast_theory_unparsed_term_t;

struct clingo_ast_theory_unparsed_term_array {
    clingo_ast_theory_unparsed_term_t const *terms;
    size_t size;
};

typedef struct clingo_ast_theory_atom_element {
    clingo_ast_theory_term_t const *tuple;
    size_t tuple_size;
    clingo_ast_literal_t const *condition;
    size_t condition_size;
} clingo_ast_theory_atom_element_t;

typedef struct clingo_ast_theory_guard {
    char const *operator_name;
    clingo_ast_theory_term_t term;
} clingo_ast_theory_guard_t;

typedef struct clingo_ast_theory_atom {
    clingo_ast_term_t term;
    clingo_ast_theory_atom_element_t const *elements;
    size_t size;
    clingo_ast_theory_guard_t const *guard;
} clingo_ast_theory_atom_t;

// {{{2 head literals

enum clingo_ast_head_literal_type {
    clingo_ast_head_literal_type_literal        = 0,
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
        clingo_ast_literal_t const *literal;
        clingo_ast_disjunction_t const *disjunction;
        clingo_ast_aggregate_t const *aggregate;
        clingo_ast_head_aggregate_t const *head_aggregate;
        clingo_ast_theory_atom_t const *theory_atom;
    };
} clingo_ast_head_literal_t;

// {{{2 body literals

enum clingo_ast_body_literal_type {
    clingo_ast_body_literal_type_literal        = 0,
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
        clingo_ast_literal_t const *literal;
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
    char const *guard;
    char const *const *operators;
    size_t size;
} clingo_ast_theory_guard_definition_t;

enum clingo_ast_theory_atom_definition_type {
    clingo_ast_theory_atom_definition_type_head      = 0,
    clingo_ast_theory_atom_definition_type_body      = 1,
    clingo_ast_theory_atom_definition_type_any       = 2,
    clingo_ast_theory_atom_definition_type_directive = 3,
};
typedef int clingo_ast_theory_atom_definition_type_t;

typedef struct clingo_ast_theory_atom_definition {
    clingo_location_t location;
    clingo_ast_theory_atom_definition_type_t type;
    char const *name;
    unsigned arity;
    char const *elements;
    clingo_ast_theory_guard_definition_t const *guard;
} clingo_ast_theory_atom_definition_t;

typedef struct clingo_ast_theory_definition {
    char const *name;
    clingo_ast_theory_term_definition_t const *terms;
    size_t terms_size;
    clingo_ast_theory_atom_definition_t const *atoms;
    size_t atoms_size;
} clingo_ast_theory_definition_t;

// {{{2 statements

// rule

typedef struct clingo_ast_rule {
    clingo_ast_head_literal_t head;
    clingo_ast_body_literal_t const *body;
    size_t size;
} clingo_ast_rule_t;

// definition

typedef struct clingo_ast_definition {
    char const *name;
    clingo_ast_term_t value;
    bool is_default;
} clingo_ast_definition_t;

// show

typedef struct clingo_ast_show_signature {
    clingo_signature_t signature;
    bool csp;
} clingo_ast_show_signature_t;

typedef struct clingo_ast_show_term {
    clingo_ast_term_t term;
    clingo_ast_body_literal_t const *body;
    size_t size;
    bool csp;
} clingo_ast_show_term_t;

// minimize

typedef struct clingo_ast_minimize {
    clingo_ast_term_t weight;
    clingo_ast_term_t priority;
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
    clingo_ast_statement_type_const             = 1,
    clingo_ast_statement_type_show_signature    = 2,
    clingo_ast_statement_type_show_term         = 3,
    clingo_ast_statement_type_minimize          = 4,
    clingo_ast_statement_type_script            = 5,
    clingo_ast_statement_type_program           = 6,
    clingo_ast_statement_type_external          = 7,
    clingo_ast_statement_type_edge              = 8,
    clingo_ast_statement_type_heuristic         = 9,
    clingo_ast_statement_type_project           = 10,
    clingo_ast_statement_type_project_signatrue = 11,
    clingo_ast_statement_type_theory_definition = 12
};
typedef int clingo_ast_statement_type_t;

typedef struct clingo_ast_statement {
    clingo_location_t location;
    clingo_ast_statement_type_t type;
    union {
        clingo_ast_rule_t const *rule;
        clingo_ast_definition_t const *definition;
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
} clingo_ast_statement_t;

// {{{1 global functions

typedef clingo_error_t clingo_ast_callback_t (clingo_ast_statement_t const *, void *);
clingo_error_t clingo_parse_term(char const *str, clingo_logger_t *logger, void *data, unsigned message_limit, clingo_symbol_t *ret);
clingo_error_t clingo_parse_program(char const *program, clingo_ast_callback_t *cb, void *cb_data, clingo_logger_t *logger, void *logger_data, unsigned message_limit);
clingo_error_t clingo_add_string(char const *str, char const **ret);
void clingo_version(int *major, int *minor, int *revision);

// {{{1 program builder

typedef struct clingo_program_builder clingo_program_builder_t;
clingo_error_t clingo_program_builder_begin(clingo_program_builder_t *bld);
clingo_error_t clingo_program_builder_add(clingo_program_builder_t *bld, clingo_ast_statement_t const *stm);
clingo_error_t clingo_program_builder_end(clingo_program_builder_t *bld);

// {{{1 control

typedef struct clingo_part {
    char const *name;
    clingo_symbol_t const *params;
    size_t size;
} clingo_part_t;

typedef clingo_error_t clingo_model_callback_t (clingo_model_t*, void *, bool *);
typedef clingo_error_t clingo_finish_callback_t (clingo_solve_result_bitset_t res, void *);
typedef clingo_error_t clingo_symbol_callback_t (clingo_symbol_t const *, size_t, void *);
typedef clingo_error_t clingo_ground_callback_t (clingo_location_t, char const *, clingo_symbol_t const *, size_t, void *, clingo_symbol_callback_t *, void *);
typedef struct clingo_control clingo_control_t;
clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const * const * params, size_t n, char const *part);
clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_symbol_t atom, clingo_truth_value_t value);
clingo_error_t clingo_control_backend(clingo_control_t *ctl, clingo_backend_t **ret);
clingo_error_t clingo_control_cleanup(clingo_control_t *ctl);
clingo_error_t clingo_control_configuration(clingo_control_t *ctl, clingo_configuration_t **conf);
clingo_error_t clingo_control_get_const(clingo_control_t *ctl, char const *name, clingo_symbol_t *ret);
clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_t const *params, size_t n, clingo_ground_callback_t *cb, void *data);
clingo_error_t clingo_control_has_const(clingo_control_t *ctl, char const *name, bool *ret);
clingo_error_t clingo_control_load(clingo_control_t *ctl, char const *file);
clingo_error_t clingo_control_new(char const *const * args, size_t n, clingo_logger_t *logger, void *data, unsigned message_limit, clingo_control_t **ctl);
clingo_error_t clingo_control_register_propagator(clingo_control_t *ctl, clingo_propagator_t propagator, void *data, bool sequential);
clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_symbol_t atom);
clingo_error_t clingo_control_solve_async(clingo_control_t *ctl, clingo_model_callback_t *mh, void *mh_data, clingo_finish_callback_t *fh, void *fh_data, clingo_symbolic_literal_t const * assumptions, size_t n, clingo_solve_async_t **ret);
clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_model_callback_t *mh, void *data, clingo_symbolic_literal_t const * assumptions, size_t n, clingo_solve_result_bitset_t *ret);
clingo_error_t clingo_control_solve_iteratively(clingo_control_t *ctl, clingo_symbolic_literal_t const *assumptions, size_t n, clingo_solve_iteratively_t **it);
clingo_error_t clingo_control_statistics(clingo_control_t *ctl, clingo_statistics_t **stats);
clingo_error_t clingo_control_symbolic_atoms(clingo_control_t *ctl, clingo_symbolic_atoms_t **ret);
clingo_error_t clingo_control_theory_atoms(clingo_control_t *ctl, clingo_theory_atoms_t **ret);
clingo_error_t clingo_control_use_enum_assumption(clingo_control_t *ctl, bool value);
clingo_error_t clingo_control_begin_add_ast(clingo_control_t *ctl);
clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_ast_statement_t const *stm);
clingo_error_t clingo_control_end_add_ast(clingo_control_t *ctl);
clingo_error_t clingo_control_program_builder(clingo_control_t *ctl, clingo_program_builder_t **ret);
void clingo_control_interrupt(clingo_control_t *ctl);
void clingo_control_free(clingo_control_t *ctl);

// }}}1

#ifdef __cplusplus
}
#endif

#endif
