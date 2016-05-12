#ifndef CCLINGO_CCLINGO_INCLUDED
#define CCLINGO_CCLINGO_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// TODO: clingo uses both *int32_t and *int64_t
// this mapping should use these two types too

typedef int bool_t;
void clingo_free(void *p);

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

// {{{1 value
enum clingo_value_type {
    clingo_value_type_inf = 0,
    clingo_value_type_num = 1,
    clingo_value_type_id  = 2,
    clingo_value_type_str = 3,
    clingo_value_type_fun = 4,
    clingo_value_type_sup = 6
};
typedef int clingo_value_type_t;
typedef struct clingo_value {
    unsigned a;
    unsigned b;
} clingo_value_t;
typedef struct clingo_value_span {
    clingo_value_t const *data;
    size_t size;
} clingo_value_span_t;
void clingo_value_new_num(int num, clingo_value_t *val);
void clingo_value_new_sup(clingo_value_t *val);
void clingo_value_new_inf(clingo_value_t *val);
clingo_error_t clingo_value_new_str(char const *str, clingo_value_t *val);
clingo_error_t clingo_value_new_id(char const *id, bool_t sign, clingo_value_t *val);
clingo_error_t clingo_value_new_fun(char const *name, clingo_value_span_t args, bool_t sign, clingo_value_t *val);
int clingo_value_num(clingo_value_t val);
char const *clingo_value_name(clingo_value_t val);
char const *clingo_value_str(clingo_value_t val);
bool_t clingo_value_sign(clingo_value_t val);
// Note: this array should not be stored for later use
//       because it might be invalidated when calling clingo functions
//       it is safe to use functions to inspect values though
clingo_value_span_t clingo_value_args(clingo_value_t val);
clingo_value_type_t clingo_value_type(clingo_value_t val);
bool_t clingo_value_eq(clingo_value_t a, clingo_value_t b);
bool_t clingo_value_lt(clingo_value_t a, clingo_value_t b);
// Note: str is allocated using malloc and has to be freed by the user
clingo_error_t clingo_value_to_string(clingo_value_t val, char **str);

// {{{1 symbolic literals
typedef struct clingo_symbolic_literal {
    clingo_value_t atom;
    bool_t sign;
} clingo_symbolic_literal_t;
typedef struct clingo_symbolic_literal_span {
    clingo_symbolic_literal_t const *data;
    size_t size;
} clingo_symbolic_literal_span_t;

// {{{1 part
typedef struct clingo_part {
    char const *name;
    clingo_value_span_t params;
} clingo_part_t;
typedef struct clingo_part_span {
    clingo_part_t const *data;
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
    clingo_show_type_all   = 31
};
typedef int clingo_show_type_t;
typedef struct clingo_model clingo_model_t;
bool_t clingo_model_contains(clingo_model_t *m, clingo_value_t atom);
clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_t show, clingo_value_span_t *ret);

// {{{1 solve result
enum clingo_solve_result {
    clingo_solve_result_unknown = 0,
    clingo_solve_result_sat     = 1,
    clingo_solve_result_unsat   = 2
};
typedef int clingo_solve_result_t;

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
    clingo_ast_t const *data;
    size_t size;
} clingo_ast_span_t;
struct clingo_ast {
    clingo_location_t location;
    clingo_value_t value;
    clingo_ast_span_t children;
};
typedef clingo_error_t clingo_parse_callback_t (clingo_ast_t const *, void *);
typedef clingo_error_t clingo_add_ast_callback_t (clingo_ast_t const **, void *);

// {{{1 control
// TODO: consider adding bool_t as parameter and return an error code instead
typedef bool_t clingo_model_handler_t (clingo_model_t*, void *);
typedef clingo_error_t clingo_ground_callback_t (char const *, clingo_value_span_t, void *, clingo_value_span_t *);
typedef struct clingo_control clingo_control_t;
clingo_error_t clingo_control_new(clingo_module_t *mod, int argc, char const **argv, clingo_control_t **);
void clingo_control_free(clingo_control_t *ctl);
clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const **params, char const *part);
clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_span_t vec, clingo_ground_callback_t *cb, void *data);
clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_symbolic_literal_span_t *assumptions, clingo_model_handler_t *mh, void *data, clingo_solve_result_t *ret);
clingo_error_t clingo_control_solve_iter(clingo_control_t *ctl, clingo_symbolic_literal_span_t *assumptions, clingo_solve_iter_t **it);
clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_value_t atom, clingo_truth_value_t value);
clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_value_t atom);
clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_parse_callback_t *cb, void *data);
clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data);

// }}}1

#ifdef __cplusplus
}
#endif

#endif //CCLINGO_CCLINGO_INCLUDED
