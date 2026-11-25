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
static bool print_model(char const *prefix, clingo_model_t const *model, clingo_string_builder_t *bld) {
    bool status = true;
    printf("%s:", prefix);
    CHECK(clingo_model_symbols(model, clingo_show_type_shown, &print_symbols, bld));
    printf("\n");
cleanup:
    return status;
}

//! Print the model passed to the solve event handler.
static bool on_model(clingo_model_t *model, void *data, bool *goon) {
    *goon = true;
    return print_model("async model", model, (clingo_string_builder_t *)data);
}

//! Print the given solve result.
static void print_result(char const *prefix, clingo_solve_result_bitset_t res) {
    char const *str = "UNKNOWN";
    if ((res & clingo_solve_result_satisfiable) != 0) {
        str = "SAT";
    } else if ((res & clingo_solve_result_unsatisfiable) != 0) {
        str = "UNSAT";
    }
    printf("%s: %s\n", prefix, str);
}

//! Iterate over the models of the grounded program.
static bool solve_yield(clingo_control_t *ctl, clingo_string_builder_t *bld) {
    bool status = true;
    clingo_solve_handle_t *hnd = NULL;
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;

    CHECK(clingo_control_solve(ctl, clingo_solve_mode_yield, NULL, 0, NULL, NULL, &hnd));
    for (;;) {
        clingo_model_t *model = NULL;
        // resume the search and get the next model
        CHECK(clingo_solve_handle_resume(hnd));
        CHECK(clingo_solve_handle_model(hnd, &model));
        // the model is NULL if there are no more models
        if (model == NULL) {
            break;
        }
        CHECK(print_model("yield model", model, bld));
    }
    CHECK(clingo_solve_handle_get(hnd, &res));
    print_result("yield result", res);

cleanup:
    clingo_solve_handle_close(hnd);
    return status;
}

//! Solve the grounded program in the background.
static bool solve_async(clingo_control_t *ctl, clingo_string_builder_t *bld) {
    bool status = true;
    clingo_solve_handle_t *hnd = NULL;
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;
    bool finished = false;
    double const timeout = 0.01;

    CHECK(clingo_control_solve(ctl, clingo_solve_mode_async, NULL, 0, &seh, bld, &hnd));
    // other work could be done here while polling the search
    while (!finished) {
        CHECK(clingo_solve_handle_wait(hnd, timeout, &finished));
    }
    CHECK(clingo_solve_handle_get(hnd, &res));
    print_result("async result", res);

cleanup:
    clingo_solve_handle_close(hnd);
    return status;
}

//! Simple example showing how to solve iteratively and asynchronously.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_string_builder_t *bld = NULL;

    char const *arg = "0";
    clingo_string_t args[] = {{arg, strlen(arg)}};
    char const *prg = "1 {a; b} 1.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};

    // models are printed from a background thread, so the library has to be shared
    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_shared | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL, 20,
                         &lib));
    CHECK(clingo_control_new(lib, args, 1, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));

    CHECK(solve_yield(ctl, bld));
    CHECK(solve_async(ctl, bld));

cleanup:
    if (!status) {
        clingo_string_t err_msg = {NULL, 0};
        clingo_result_t code = 0;
        clingo_get_error(&code, &err_msg);
        fprintf(stderr, "Error: %.*s\n", (int)err_msg.size, err_msg.data);
    }

    clingo_control_release(ctl);
    clingo_lib_release(lib);
    clingo_string_builder_free(bld);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
