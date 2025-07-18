#include <clingo.h>
#include <clingo/backend.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(label, call)                                                                                             \
    do {                                                                                                               \
        if (!(call)) {                                                                                                 \
            status = false;                                                                                            \
            goto label;                                                                                                \
        }                                                                                                              \
    } while (0)
#define ERROR(label, cond, type, msg)                                                                                  \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            clingo_set_error(clingo_result_##type, msg, strlen(msg));                                                  \
            status = false;                                                                                            \
            goto label;                                                                                                \
        }                                                                                                              \
    } while (0)

//! Print the given symbols.
bool print_symbols(clingo_symbol_t const *symbols, size_t size, void *data) {
    bool status = true;
    clingo_string_builder_t *bld = (clingo_string_builder_t *)data;
    for (size_t i = 0; i < size; ++i) {
        clingo_string_builder_clear(bld);
        CHECK(out, clingo_symbol_to_string(symbols[i], bld));
        clingo_string_t str = {NULL, 0};
        CHECK(out, clingo_string_builder_string(bld, &str));
        printf(" %.*s", (int)str.size, str.data);
    }
out:
    return status;
}

//! Print the given model.
bool on_model(clingo_model_t *model, void *data, bool *goon) {
    bool status = true;
    *goon = true;
    printf("Answer:");
    CHECK(out, clingo_model_symbols(model, clingo_show_type_shown, &print_symbols, data));
    printf("\n");

out:
    return status;
}

//! Solve a previously grounded program and print the models.
bool solve(clingo_control_t *ctl, clingo_solve_result_bitset_t *result) {
    bool status = true;

    clingo_solve_handle_t *handle = NULL;
    clingo_string_builder_t *bld = NULL;
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};

    CHECK(out, clingo_string_builder_new(&bld));
    CHECK(out, clingo_control_solve(ctl, clingo_solve_mode_default, NULL, 0, &seh, bld, &handle));
    CHECK(out, clingo_solve_handle_get(handle, result));

out:
    clingo_string_builder_free(bld);
    clingo_solve_handle_close(handle);

    return status;
}

//! Initialize the library, control object, and ground a simple program.
bool init(int argc, char const **argv, clingo_lib_t **lib, clingo_control_t **ctl, char const *prg) {
    bool status = true;

    clingo_lib_flags_t flags = clingo_lib_flags_fast_release | clingo_lib_flags_slotted;
    clingo_log_level_t level = clingo_log_level_info;
    size_t const limit = 20;

    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};

    clingo_string_t *clingo_argv = malloc((argc - 1) * sizeof(clingo_string_t));

    ERROR(out, clingo_argv == NULL, bad_alloc, "failed to allocate memory for arguments");

    for (int i = 1; i < argc; ++i) {
        clingo_argv[i - 1].data = argv[i];
        clingo_argv[i - 1].size = strlen(argv[i]);
    }

    CHECK(out, clingo_lib_new(flags, level, NULL, NULL, limit, lib));
    CHECK(out, clingo_control_new(*lib, clingo_argv, argc - 1, ctl));
    CHECK(out, clingo_control_parse_string(*ctl, prg, strlen(prg)));
    CHECK(out, clingo_control_ground(*ctl, &part, 1, NULL, NULL));

out:
    free(clingo_argv);

    return status;
}

//! Simple example showing how to use the backend to add rules to a grounded program.
int main(int argc, char const **argv) {
    bool status = true;

    clingo_solve_result_bitset_t solve_ret = clingo_solve_result_empty;
    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_base_t const *base = NULL;
    clingo_backend_t *backend = NULL;
    clingo_atom_t atom_ids[] = {0, 0, 0, 0};
    char const *atom_strings[] = {"a", "b", "c"};
    clingo_literal_t body[] = {0, 0};

    CHECK(out, init(argc, argv, &lib, &ctl, "{a; b; c}."));

    // get the container for symbolic atoms
    CHECK(out, clingo_control_base(ctl, &base));
    // get the ids of atoms a, b, and c
    for (size_t i = 0, n = (sizeof(atom_strings) / sizeof(*atom_strings)); i != n; ++i) {
        char const *name = atom_strings[i];
        clingo_symbol_t sym = 0;
        clingo_literal_t lit = 0;
        clingo_atom_base_t const *atom_base = NULL;
        clingo_signature_t sig = {name, strlen(name), 0, true};
        size_t index = 0;
        size_t size = 0;
        bool found = false;

        // lookup the atom
        CHECK(loop_out, clingo_symbol_create_id(lib, name, strlen(name), true, &sym));
        CHECK(loop_out, clingo_base_atoms_find(base, &sig, &atom_base, &found));
        ERROR(loop_out, !found, logic, "atom not found in base");
        CHECK(loop_out, clingo_atom_base_find(atom_base, sym, &index));
        CHECK(loop_out, clingo_atom_base_size(atom_base, &size));
        ERROR(loop_out, index >= size, logic, "index out of range");

        CHECK(loop_out, clingo_atom_base_literal(atom_base, index, &lit));
        atom_ids[i] = lit;

    loop_out:
        clingo_symbol_release(sym);
    }

    // prepare the backend for adding rules
    CHECK(out, clingo_control_backend(ctl, &backend));
    // add an additional atom (referred to as d below)
    CHECK(out, clingo_backend_add_atom(backend, NULL, atom_ids + 3));
    // add rule: d :- a, b.
    body[0] = (clingo_literal_t)atom_ids[0];
    body[1] = (clingo_literal_t)atom_ids[1];
    CHECK(out, clingo_backend_rule(backend, false, atom_ids + 3, 1, body, 2));
    // add rule: :- not d, c.
    body[0] = -(clingo_literal_t)atom_ids[3];
    body[1] = (clingo_literal_t)atom_ids[2];
    CHECK(out, clingo_backend_rule(backend, false, NULL, 0, body, 2));
    // finalize the backend
    CHECK(out, clingo_backend_close(backend));

    CHECK(out, solve(ctl, &solve_ret));

out:
    if (!status) {
        clingo_string_t err_msg = {NULL, 0};
        clingo_result_t code = 0;
        clingo_get_error(&code, &err_msg);
        fprintf(stderr, "Error: %.*s\n", (int)err_msg.size, err_msg.data);
    }

    clingo_control_release(ctl);
    clingo_lib_release(lib);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
