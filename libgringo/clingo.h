#ifndef GRINGO_CLINGO_H
#define GRINGO_CLINGO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// {{{1 error

enum clingo_error {
    clingo_error_success   = 0,
    clingo_error_runtime   = 1,
    clingo_error_logic     = 2,
    clingo_error_bad_alloc = 3,
    clingo_error_unknown   = 4
};
// An application can return negative error codes from callbacks
// to indicate errors not related to clingo.
// Such error codes are passed through to the calling function.
typedef int clingo_error_t;
char const *clingo_error_str(clingo_error_t err);

// {{{1 symbol
// {{{2 types

enum clingo_symbol_type {
    clingo_symbol_type_inf = 0,
    clingo_symbol_type_num = 1,
    clingo_symbol_type_str = 4,
    clingo_symbol_type_fun = 5,
    clingo_symbol_type_sup = 7
};
typedef int clingo_symbol_type_t;
typedef struct clingo_symbol {
    uint64_t rep;
} clingo_symbol_t;
typedef struct clingo_symbol_span {
    clingo_symbol_t const *first;
    size_t size;
} clingo_symbol_span_t;
typedef clingo_error_t clingo_string_callback (char const *, void *);

// {{{2 construction

void clingo_symbol_new_num(int num, clingo_symbol_t *sym);
void clingo_symbol_new_sup(clingo_symbol_t *sym);
void clingo_symbol_new_inf(clingo_symbol_t *sym);
clingo_error_t clingo_symbol_new_str(char const *str, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_new_id(char const *id, bool sign, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_new_fun(char const *name, clingo_symbol_span_t args, bool sign, clingo_symbol_t *sym);

// {{{2 inspection

clingo_error_t clingo_symbol_num(clingo_symbol_t sym, int *num);
clingo_error_t clingo_symbol_name(clingo_symbol_t sym, char const **name);
clingo_error_t clingo_symbol_string(clingo_symbol_t sym, char const **str);
clingo_error_t clingo_symbol_sign(clingo_symbol_t sym, bool *sign);
clingo_error_t clingo_symbol_args(clingo_symbol_t sym, clingo_symbol_span_t *args);
clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t sym);
clingo_error_t clingo_symbol_to_string(clingo_symbol_t sym, clingo_string_callback *cb, void *data);

// {{{2 comparison

size_t clingo_symbol_hash(clingo_symbol_t sym);
bool clingo_symbol_eq(clingo_symbol_t a, clingo_symbol_t b);
bool clingo_symbol_lt(clingo_symbol_t a, clingo_symbol_t b);

// {{{1 symbolic literals

typedef struct clingo_symbolic_literal {
    clingo_symbol_t atom;
    bool sign;
} clingo_symbolic_literal_t;

typedef struct clingo_symbolic_literal_span {
    clingo_symbolic_literal_t const *first;
    size_t size;
} clingo_symbolic_literal_span_t;

// {{{1 part

typedef struct clingo_part {
    char const *name;
    clingo_symbol_span_t params;
} clingo_part_t;
typedef struct clingo_part_span {
    clingo_part_t const *first;
    size_t size;
} clingo_part_span_t;

// {{{1 module (there should only ever be one module)

typedef struct clingo_module clingo_module_t;

clingo_error_t clingo_module_new(clingo_module_t **mod);
void clingo_module_free(clingo_module_t *mod);

// {{{1 model

enum clingo_show_type {
    clingo_show_type_csp   = 1,
    clingo_show_type_shown = 2,
    clingo_show_type_atoms = 4,
    clingo_show_type_terms = 8,
    clingo_show_type_comp  = 16,
    clingo_show_type_all   = 15
};
typedef int clingo_show_type_t;
typedef struct clingo_model clingo_model_t;
bool clingo_model_contains(clingo_model_t *m, clingo_symbol_t atom);
clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_t show, clingo_symbol_span_t *ret);

// {{{1 solve result

enum clingo_solve_result {
    clingo_solve_result_sat         = 1,
    clingo_solve_result_unsat       = 2,
    clingo_solve_result_exhausted   = 4,
    clingo_solve_result_interrupted = 8
};
typedef unsigned clingo_solve_result_t;

// {{{1 solve iter

typedef struct clingo_solve_iter clingo_solve_iter_t;
clingo_error_t clingo_solve_iter_next(clingo_solve_iter_t *it, clingo_model_t **m);
clingo_error_t clingo_solve_iter_get(clingo_solve_iter_t *it, clingo_solve_result_t *ret);
clingo_error_t clingo_solve_iter_close(clingo_solve_iter_t *it);

// {{{1 truth value

enum clingo_truth_value {
    clingo_truth_value_free = 0,
    clingo_truth_value_true = 1,
    clingo_truth_value_false = 2
};
typedef int clingo_truth_value_t;

// {{{1 ast

typedef struct clingo_location {
    char const *begin_file;
    char const *end_file;
    size_t begin_line;
    size_t end_line;
    size_t begin_column;
    size_t end_column;
} clingo_location_t;
typedef struct clingo_ast clingo_ast_t;
typedef struct clingo_ast_span {
    clingo_ast_t const *first;
    size_t size;
} clingo_ast_span_t;
struct clingo_ast {
    clingo_location_t location;
    clingo_symbol_t value;
    clingo_ast_span_t children;
};
typedef clingo_error_t clingo_ast_callback_t (clingo_ast_t const *, void *);
typedef clingo_error_t clingo_add_ast_callback_t (void *, clingo_ast_callback_t *, void *);

// {{{1 control

typedef clingo_error_t clingo_model_handler_t (clingo_model_t*, void *, bool *);
typedef clingo_error_t clingo_symbol_span_callback_t (clingo_symbol_span_t, void *);
typedef clingo_error_t clingo_ground_callback_t (char const *, clingo_symbol_span_t, void *, clingo_symbol_span_callback_t *, void *);
typedef struct clingo_control clingo_control_t;
clingo_error_t clingo_control_new(clingo_module_t *mod, int argc, char const **argv, clingo_control_t **);
void clingo_control_free(clingo_control_t *ctl);
clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const **params, char const *part);
clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_span_t vec, clingo_ground_callback_t *cb, void *data);
clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_symbolic_literal_span_t assumptions, clingo_model_handler_t *mh, void *data, clingo_solve_result_t *ret);
clingo_error_t clingo_control_solve_iter(clingo_control_t *ctl, clingo_symbolic_literal_span_t assumptions, clingo_solve_iter_t **it);
clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_symbol_t atom, clingo_truth_value_t value);
clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_symbol_t atom);
clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_ast_callback_t *cb, void *data);
clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data);

// }}}1

#ifdef __cplusplus
}
#endif

#endif
