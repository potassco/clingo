#include <clingo/control.h>
#include <clingo/solve.h>

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

//! Simple example showing how to ground and solve a logic program.
//!
//! The command line arguments are passed on to the control object.
int main(int argc, char const **argv) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_solve_handle_t *hnd = NULL;
    clingo_string_builder_t *bld = NULL;
    clingo_string_t *args = NULL;
    size_t const nargs = argc > 1 ? (size_t)argc - 1 : 0;

    char const *prg = "1 {a; b} 1.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;

    // convert the command line arguments
    args = (clingo_string_t *)malloc((nargs + 1) * sizeof(clingo_string_t));
    ERROR(args == NULL, bad_alloc, "failed to allocate memory for arguments");
    for (size_t i = 0; i < nargs; ++i) {
        args[i].data = argv[i + 1];
        args[i].size = strlen(argv[i + 1]);
    }

    // create a library and a control object
    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, args, nargs, &ctl));

    // parse and ground the program
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));

    // solve printing models with the event handler
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
    free(args);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
