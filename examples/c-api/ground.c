#include <clingo/control.h>
#include <clingo/ground.h>
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
#define ERROR(cond, type, msg)                                                                                         \
    do {                                                                                                               \
        if (cond) {                                                                                                    \
            clingo_set_error(clingo_result_##type, msg, strlen(msg));                                                  \
            status = false;                                                                                            \
            goto cleanup;                                                                                              \
        }                                                                                                              \
    } while (0)

//! Check if the given name equals the expected one.
static bool is_name(char const *name, size_t size, char const *expected) {
    return (size == strlen(expected) && strncmp(name, expected, size) == 0) != 0;
}

//! Report which external functions are implemented.
static bool callable(char const *name, size_t name_size, size_t arguments_size, void *data, bool *result) {
    (void)data;
    *result = (((is_name(name, name_size, "inc") && arguments_size == 1) ||
                (is_name(name, name_size, "seq") && arguments_size == 2)) != 0);
    return true;
}

//! Implement the external functions @inc(X) and @seq(X,Y).
static bool call(clingo_lib_t *lib, clingo_location_t const *location, char const *name, size_t name_size,
                 clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                 clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) {
    bool status = true;
    (void)lib;
    (void)location;
    (void)data;

    if (is_name(name, name_size, "inc") && arguments_size == 1) {
        // return the successor of the given number
        int32_t num = 0;
        ERROR(clingo_symbol_type(arguments[0]) != clingo_symbol_type_number, runtime, "inc expects a number");
        CHECK(clingo_symbol_number(arguments[0], &num));
        ERROR(num == INT32_MAX, runtime, "inc overflows");
        clingo_symbol_t sym = clingo_symbol_create_number(num + 1);
        CHECK(symbol_callback(&sym, 1, symbol_callback_data));
    } else if (is_name(name, name_size, "seq") && arguments_size == 2) {
        // return both arguments
        CHECK(symbol_callback(arguments, arguments_size, symbol_callback_data));
    } else {
        ERROR(true, runtime, "unknown external function");
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

//! Simple example showing how to ground in the background using external functions.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_ground_handle_t *ground_hnd = NULL;
    clingo_solve_handle_t *solve_hnd = NULL;
    clingo_string_builder_t *bld = NULL;
    bool finished = false;

    char const *prg = "p(@inc(10)). q(@seq(1,2)).";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    clingo_ground_event_handler_t geh = {&callable, &call, NULL, NULL};
    clingo_ground_result_t ground_res = clingo_ground_result_ok;
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};
    clingo_solve_result_bitset_t solve_res = clingo_solve_result_empty;

    // symbols are created in a background thread, so the library has to be shared
    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_shared | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL, 20,
                         &lib));
    CHECK(clingo_control_new(lib, NULL, 0, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));

    // start grounding in the background
    CHECK(clingo_control_start_ground(ctl, &part, 1, &geh, NULL, &ground_hnd));
    // poll whether grounding has finished (other work could be done here)
    CHECK(clingo_ground_handle_wait(ground_hnd, 0, &finished));
    printf("grounding %s\n", finished ? "finished" : "running");
    // wait for grounding to finish and get the result
    CHECK(clingo_ground_handle_get(ground_hnd, &ground_res));
    printf("grounding result: %s\n", ground_res == clingo_ground_result_ok ? "ok" : "not ok");
    // the handle must be closed before solving
    clingo_ground_handle_close(ground_hnd);
    ground_hnd = NULL;

    // solve and print the models
    CHECK(clingo_control_solve(ctl, clingo_solve_mode_default, NULL, 0, &seh, bld, &solve_hnd));
    CHECK(clingo_solve_handle_get(solve_hnd, &solve_res));

cleanup:
    if (!status) {
        clingo_string_t err_msg = {NULL, 0};
        clingo_result_t code = 0;
        clingo_get_error(&code, &err_msg);
        fprintf(stderr, "Error: %.*s\n", (int)err_msg.size, err_msg.data);
    }

    clingo_ground_handle_close(ground_hnd);
    clingo_solve_handle_close(solve_hnd);
    clingo_control_release(ctl);
    clingo_lib_release(lib);
    clingo_string_builder_free(bld);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
