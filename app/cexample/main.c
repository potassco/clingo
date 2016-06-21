#include <clingo.h>
#include <stdio.h>
#include <stdlib.h>

#define EM(e, msg) \
    ret = (e); \
    if (ret != 0) { \
        fprintf(stderr, "example failed with: %s\n", (msg) ? (msg) : "unknown error"); \
        goto cleanup; \
    }
#define E(e) EM((e), clingo_error_message())
#define A(ptr, type, old_n, new_n, msg) \
    if (old_n < new_n) { \
        type *ptr ## _new = (type*)realloc(ptr, new_n * sizeof(type)); \
        EM(ptr ## _new ? clingo_error_success : clingo_error_bad_alloc, (msg)); \
        ptr = ptr ## _new; \
        old_n = new_n; \
    }

void logger(clingo_warning_t code, char const *message, void *data) {
    (void)code;
    (void)data;
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
}

int main(int argc, char const **argv) {
    clingo_error_t ret;
    clingo_control_t *ctl = NULL;
    clingo_solve_iterator_t *solve_it = NULL;
    clingo_part_t parts[] = {{ "base", NULL, 0 }};
    clingo_symbol_t *atoms = NULL;
    size_t n;
    size_t atoms_n = 0;
    char *str = NULL;
    size_t str_n = 0;
    E(clingo_control_new(argv+1, argc-1, &logger, NULL, 20, &ctl));
    E(clingo_control_add(ctl, "base", NULL, 0, "a :- not b. b :- not a."));
    E(clingo_control_ground(ctl, parts, 1, NULL, NULL));
    E(clingo_control_solve_iteratively(ctl, NULL, 0, &solve_it));
    for (;;) {
        clingo_model_t *m;
        clingo_symbol_t const *atoms_it, *atoms_ie;
        E(clingo_solve_iterator_next(solve_it, &m));
        if (!m) { break; }
        E(clingo_model_atoms_size(m, clingo_show_type_atoms | clingo_show_type_csp, &n));
        A(atoms, clingo_symbol_t, atoms_n, n, "failed to allocate memory for atoms");
        E(clingo_model_atoms(m, clingo_show_type_atoms | clingo_show_type_csp, atoms, n));
        printf("Model:");
        for (atoms_it = atoms, atoms_ie = atoms + n; atoms_it != atoms_ie; ++atoms_it) {
            E(clingo_symbol_to_string_size(*atoms_it, &n));
            A(str, char, str_n, n, "failed to allocate memory for symbol's string");
            E(clingo_symbol_to_string(*atoms_it, str, n));
            printf(" %s", str);
        }
        printf("\n");
    }
cleanup:
    if (str)      { free(str); }
    if (atoms)    { free(atoms); }
    if (solve_it) { clingo_solve_iterator_close(solve_it); }
    if (ctl)      { clingo_control_free(ctl); }
    return ret;
}

