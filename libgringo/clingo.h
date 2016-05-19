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

// }}}1

#ifdef __cplusplus
}
#endif

#endif
