#include <clingo.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(call)                                                                                                    \
    do {                                                                                                               \
        if (!(call)) {                                                                                                 \
            status = false;                                                                                            \
            goto cleanup;                                                                                              \
        }                                                                                                              \
    } while (0)
#define ERROR(cond, type, msg)                                                                                         \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            clingo_set_error(clingo_result_##type, msg, strlen(msg));                                                  \
            status = false;                                                                                            \
            goto cleanup;                                                                                              \
        }                                                                                                              \
    } while (0)

//! The state of the propagator.
typedef struct {
    clingo_literal_t a; //!< solver literal of atom a
    clingo_literal_t b; //!< solver literal of atom b
} aiffb_t;

//! Get the solver literal of the given propositional atom and watch it.
static bool watch_atom(clingo_propagate_init_t *init, char const *name, clingo_literal_t *solver_lit) {
    bool status = true;
    clingo_lib_t *lib = NULL;
    clingo_base_t const *base = NULL;
    clingo_propagate_control_t *ctl = NULL;
    clingo_atom_base_t const *atoms = NULL;
    clingo_signature_t sig = {name, strlen(name), 0, true};
    clingo_symbol_t sym = 0;
    clingo_literal_t program_lit = 0;
    size_t index = 0;
    size_t size = 0;
    bool found = false;

    // look up the program literal of the atom
    CHECK(clingo_propagate_init_library(init, &lib));
    CHECK(clingo_propagate_init_base(init, &base));
    CHECK(clingo_base_atoms_find(base, &sig, &atoms, &found));
    ERROR(!found, logic, "atom not found in base");
    CHECK(clingo_symbol_create_id(lib, name, strlen(name), true, &sym));
    CHECK(clingo_atom_base_find(atoms, sym, &index));
    CHECK(clingo_atom_base_size(atoms, &size));
    ERROR(index >= size, logic, "atom not found in base");
    CHECK(clingo_atom_base_literal(atoms, index, &program_lit));

    // map it to a solver literal and add a watch
    CHECK(clingo_propagate_init_solver_literal(init, program_lit, solver_lit));
    CHECK(clingo_propagate_init_control(init, &ctl));
    CHECK(clingo_propagate_control_add_watch(ctl, *solver_lit));

cleanup:
    clingo_symbol_release(sym);
    return status;
}

//! Initialize the propagator watching atoms a and b.
static bool aiffb_init(clingo_assignment_t const *assignment, clingo_propagate_init_t *init, void *data) {
    bool status = true;
    aiffb_t *self = (aiffb_t *)data;
    (void)assignment;

    CHECK(watch_atom(init, "a", &self->a));
    CHECK(watch_atom(init, "b", &self->b));

cleanup:
    return status;
}

//! Propagate that a implies b and b implies a.
static bool aiffb_propagate(clingo_assignment_t const *assignment, clingo_propagate_control_t *control,
                            clingo_literal_t const *changes, size_t size, void *data) {
    bool status = true;
    aiffb_t *self = (aiffb_t *)data;
    (void)assignment;

    for (size_t i = 0; i < size; ++i) {
        clingo_literal_t clause[2] = {0, 0};
        bool result = false;
        if (changes[i] == self->a) {
            // if a is true, then b has to be true
            clause[0] = -self->a;
            clause[1] = self->b;
        } else if (changes[i] == self->b) {
            // if b is true, then a has to be true
            clause[0] = -self->b;
            clause[1] = self->a;
        } else {
            continue;
        }
        CHECK(clingo_propagate_control_add_clause(control, clause, 2, clingo_clause_type_learnt, &result));
        // stop propagation if the clause caused a conflict
        if (!result) {
            break;
        }
    }

cleanup:
    return status;
}

//! Print the given symbols.
static bool print_symbols(clingo_symbol_t const *symbols, size_t size, void *data) {
    bool status = true;
    clingo_string_builder_t *bld = (clingo_string_builder_t *)data;
    for (size_t i = 0; i < size; ++i) {
        clingo_string_t str = {NULL, 0};
        clingo_string_builder_clear(bld);
        CHECK(clingo_symbol_to_string(symbols[i], bld));
        CHECK(clingo_string_builder_string(bld, &str));
        printf(" %.*s", (int)str.size, str.data);
    }
cleanup:
    return status;
}

//! Print the given model.
static bool on_model(clingo_model_t *model, void *data, bool *goon) {
    bool status = true;
    *goon = true;
    printf("Model:");
    CHECK(clingo_model_symbols(model, clingo_show_type_shown, &print_symbols, data));
    printf("\n");
cleanup:
    return status;
}

//! Simple example showing how to implement a propagator enforcing that a holds if and only if b holds.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_solve_handle_t *hnd = NULL;
    clingo_string_builder_t *bld = NULL;

    char const *arg = "0";
    clingo_string_t args[] = {{arg, strlen(arg)}};
    char const *prg = "1 {a; b}.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    // the propagator state lives on the stack, so no free callback is needed
    aiffb_t aiffb = {0, 0};
    clingo_propagator_t prop = {&aiffb_init, NULL, &aiffb_propagate, NULL, NULL, NULL, NULL};
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;

    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, args, 1, &ctl));
    CHECK(clingo_control_register_propagator(ctl, &prop, &aiffb));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));
    CHECK(clingo_control_solve(ctl, clingo_solve_mode_default, NULL, 0, &seh, bld, &hnd));
    CHECK(clingo_solve_handle_get(hnd, &res));

cleanup:
    if (!status) {
        clingo_string_t err_msg = {NULL, 0};
        clingo_result_t code = 0;
        clingo_get_error(&code, &err_msg);
        fprintf(stderr, "Error: %.*s\n", (int)err_msg.size, err_msg.data);
    }

    clingo_solve_handle_close(hnd);
    clingo_control_release(ctl);
    clingo_lib_release(lib);
    clingo_string_builder_free(bld);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
