#ifndef GRINGO_CLINGO_INCLUDED
#define GRINGO_CLINGO_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

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

#ifdef __cplusplus
}
#endif

#endif
