#include <clingo/control.h>
#include <clingo/model.h>
#include <clingo/solve.h>

#include <stdint.h>
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

//! Data passed to the model callback.
typedef struct {
    clingo_string_builder_t *bld;
    clingo_symbol_t atom_a;
} model_data_t;

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

//! Print the symbols of the given type in the model.
static bool print_model(clingo_model_t const *model, char const *label, clingo_show_type_bitset_t show,
                        clingo_string_builder_t *bld) {
    bool status = true;
    printf("  %s:", label);
    CHECK(clingo_model_symbols(model, show, &print_symbols, bld));
    printf("\n");
cleanup:
    return status;
}

//! Inspect the given model.
static bool on_model(clingo_model_t *model, void *data, bool *goon) {
    bool status = true;
    model_data_t *md = (model_data_t *)data;
    clingo_model_type_t type = clingo_model_type_stable_model;
    uint64_t number = 0;
    bool contains = false;

    *goon = true;

    // print the number and type of the model
    CHECK(clingo_model_number(model, &number));
    CHECK(clingo_model_type(model, &type));
    printf("Model %llu (%s):\n", (unsigned long long)number,
           type == clingo_model_type_stable_model ? "stable model" : "consequences");

    // print the symbols of the model selecting different types
    CHECK(print_model(model, "shown", clingo_show_type_shown, md->bld));
    CHECK(print_model(model, "atoms", clingo_show_type_atoms, md->bld));
    CHECK(print_model(model, "terms", clingo_show_type_terms, md->bld));

    // check if atom a is contained in the model
    CHECK(clingo_model_contains(model, md->atom_a, &contains));
    printf("  contains a: %s\n", contains ? "yes" : "no");

cleanup:
    return status;
}

//! Simple example showing how to inspect models.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_solve_handle_t *hnd = NULL;
    model_data_t md = {NULL, 0};

    char const *arg = "0";
    clingo_string_t args[] = {{arg, strlen(arg)}};
    char const *prg = "1 {a; b} 1. #show c : b. #show a/0.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;

    CHECK(clingo_string_builder_new(&md.bld));
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_symbol_create_id(lib, "a", 1, true, &md.atom_a));
    CHECK(clingo_control_new(lib, args, 1, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));
    CHECK(clingo_control_solve(ctl, clingo_solve_mode_default, NULL, 0, &seh, &md, &hnd));
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
    clingo_symbol_release(md.atom_a);
    clingo_lib_release(lib);
    clingo_string_builder_free(md.bld);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
