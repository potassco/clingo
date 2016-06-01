#include <clingo.h>
#include <stdio.h>
#include <stdlib.h>

#define E(e, msg) \
    ret = (e); \
    if (ret != 0) { \
        fprintf(stderr, "%s\n", (msg)); \
        goto cleanup; \
    }
#define A(ptr, type, old_n, new_n, msg) \
    if (old_n < new_n) { \
        type *ptr ## _new = (type*)realloc(ptr, new_n * sizeof(type)); \
        E(ptr ## _new ? clingo_error_success : clingo_error_bad_alloc, (msg)); \
        ptr = ptr ## _new; \
        old_n = new_n; \
    }

void logger(clingo_message_code_t code, char const *message, void *data) {
    (void)code;
    (void)data;
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
}

int main(int argc, char const **argv) {
    clingo_error_t ret;
    clingo_module_t *mod = 0;
    clingo_control_t *ctl = 0;
    clingo_solve_iter_t *it = 0;
    clingo_part_t parts[] = {{ "base", 0, 0 }};
    clingo_symbol_t *atoms = 0;
    size_t n;
    size_t atoms_n = 0;
    char *str = 0;
    size_t str_n = 0;
    E(clingo_module_new(&mod), "error initializing module");
    E(clingo_control_new(mod, argv, argc, &logger, 0, 20, &ctl), "error initializing control");
    E(clingo_control_add(ctl, "base", 0, 0, "a :- not b. b :- not a."), "error adding program");
    E(clingo_control_ground(ctl, parts, 1, 0, 0), "error while grounding");
    E(clingo_control_solve_iter(ctl, 0, 0, &it), "error while solving");
    for (;;) {
        clingo_model_t *m;
        E(clingo_solve_iter_next(it, &m), "error grounding program");
        if (!m) { break; }
        E(clingo_model_atoms(m, clingo_show_type_atoms | clingo_show_type_csp, 0, &n), "error obtaining number of atoms");
        A(atoms, clingo_symbol_t, atoms_n, n, "failed to allocate memory for atoms");
        E(clingo_model_atoms(m, clingo_show_type_atoms | clingo_show_type_csp, atoms, &n), "error obtaining atoms");
        printf("Model:");
        for (clingo_symbol_t const *it = atoms, *ie = it + atoms_n; it != ie; ++it) {
            E(clingo_symbol_to_string(*it, 0, &n), "error obtaining symbol's string length");
            A(str, char, str_n, n, "failed to allocate memory for symbol's string");
            E(clingo_symbol_to_string(*it, str, &n), "error obtaining symbol's string\n");
            printf(" %s", str);
        }
        printf("\n");
    }
cleanup:
    if (str)   { free(str); }
    if (atoms) { free(atoms); }
    if (it)    { clingo_solve_iter_close(it); }
    if (ctl)   { clingo_control_free(ctl); }
    if (mod)   { clingo_module_free(mod); }
    return ret;
}

