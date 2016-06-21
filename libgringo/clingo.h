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
    clingo_warning_atom_undefined      = 1, //< Undefined atom in program.
    clingo_warning_file_included       = 2, //< Same file included multiple times.
    clingo_warning_variable_unbounded  = 3, //< CSP Domain undefined.
    clingo_warning_global_variable     = 4, //< Global variable in tuple of aggregate element.
    clingo_warning_other               = 5, //< Other kinds of warnings
};
typedef int clingo_warning_t;

char const *clingo_warning_string(clingo_warning_t code);
typedef void clingo_logger_t(clingo_warning_t, char const *, void *);

// {{{1 signature

typedef uint64_t clingo_signature_t;

clingo_error_t clingo_signature_create(char const *name, uint32_t arity, bool sign, clingo_signature_t *ret);
char const *clingo_signature_name(clingo_signature_t sig);
uint32_t clingo_signature_arity(clingo_signature_t sig);
bool clingo_signature_sign(clingo_signature_t sig);
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
    clingo_symbol_t atom;
    bool sign;
} clingo_symbolic_literal_t;

// construction

void clingo_symbol_create_num(int num, clingo_symbol_t *sym);
void clingo_symbol_create_supremum(clingo_symbol_t *sym);
void clingo_symbol_create_infimum(clingo_symbol_t *sym);
clingo_error_t clingo_symbol_create_string(char const *str, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_create_id(char const *id, bool sign, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_create_function(char const *name, clingo_symbol_t const *args, size_t n, bool sign, clingo_symbol_t *sym);

// inspection

clingo_error_t clingo_symbol_number(clingo_symbol_t sym, int *num);
clingo_error_t clingo_symbol_name(clingo_symbol_t sym, char const **name);
clingo_error_t clingo_symbol_string(clingo_symbol_t sym, char const **str);
clingo_error_t clingo_symbol_sign(clingo_symbol_t sym, bool *sign);
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
    clingo_show_type_all        = 15,
    clingo_show_type_complement = 16
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

typedef struct clingo_solve_iterator clingo_solve_iterator_t;
clingo_error_t clingo_solve_iterator_next(clingo_solve_iterator_t *it, clingo_model_t **m);
clingo_error_t clingo_solve_iterator_get(clingo_solve_iterator_t *it, clingo_solve_result_bitset_t *ret);
clingo_error_t clingo_solve_iterator_close(clingo_solve_iterator_t *it);

// {{{1 solve async

typedef struct clingo_solve_async clingo_solve_async_t;
clingo_error_t clingo_solve_async_cancel(clingo_solve_async_t *async);
clingo_error_t clingo_solve_async_get(clingo_solve_async_t *async, clingo_solve_result_bitset_t *ret);
clingo_error_t clingo_solve_async_wait(clingo_solve_async_t *async, double timeout, bool *ret);

// {{{1 ast

// TODO:
//   think about a visitor:
//     clingo_error_t clingo_ast_callback(clingo_location_t *loc, clingo_symbol_t value, size_t number_of_children, void *data)
//     - should location be optional?
//       if not, the location could simply be added to the arguments
//     - allows for building an AST using a stack
//     - adding an ast to the program could work the same way
//       the user would have to call the clingo_ast_callback
//       this could be done in a callback again
//       or alternatively with an object that has to be created/freed
typedef struct clingo_location {
    char const *begin_file;
    char const *end_file;
    size_t begin_line;
    size_t end_line;
    size_t begin_column;
    size_t end_column;
} clingo_location_t;
typedef struct clingo_ast clingo_ast_t;
struct clingo_ast {
    clingo_location_t location;
    clingo_symbol_t value;
    clingo_ast_t const *children;
    size_t n;
};
typedef clingo_error_t clingo_ast_callback_t (clingo_ast_t const *, void *);
typedef clingo_error_t clingo_add_ast_callback_t (void *, clingo_ast_callback_t *, void *);

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

// {{{1 global functions

clingo_error_t clingo_parse_term(char const *str, clingo_logger_t *logger, void *data, unsigned message_limit, clingo_symbol_t *ret);
void clingo_version(int *major, int *minor, int *revision);

// {{{1 control

typedef struct clingo_part {
    char const *name;
    clingo_symbol_t const *params;
    size_t n;
} clingo_part_t;

typedef clingo_error_t clingo_model_callback_t (clingo_model_t*, void *, bool *);
typedef clingo_error_t clingo_finish_callback_t (clingo_solve_result_bitset_t res, void *);
typedef clingo_error_t clingo_symbol_callback_t (clingo_symbol_t const *, size_t, void *);
typedef clingo_error_t clingo_ground_callback_t (clingo_location_t, char const *, clingo_symbol_t const *, size_t, void *, clingo_symbol_callback_t *, void *);
typedef struct clingo_control clingo_control_t;
clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data);
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
clingo_error_t clingo_control_solve_iteratively(clingo_control_t *ctl, clingo_symbolic_literal_t const *assumptions, size_t n, clingo_solve_iterator_t **it);
clingo_error_t clingo_control_statistics(clingo_control_t *ctl, clingo_statistics_t **stats);
clingo_error_t clingo_control_symbolic_atoms(clingo_control_t *ctl, clingo_symbolic_atoms_t **ret);
clingo_error_t clingo_control_theory_atoms(clingo_control_t *ctl, clingo_theory_atoms_t **ret);
clingo_error_t clingo_control_use_enum_assumption(clingo_control_t *ctl, bool value);
void clingo_control_interrupt(clingo_control_t *ctl);
void clingo_control_free(clingo_control_t *ctl);

// TODO: should not depend on control
clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_ast_callback_t *cb, void *data);

// }}}1

#ifdef __cplusplus
}
#endif

#endif
