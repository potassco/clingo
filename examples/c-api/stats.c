#include <clingo/control.h>
#include <clingo/solve.h>
#include <clingo/stats.h>

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

//! Add user statistics after solving.
//!
//! Step statistics are reset for each solve call while accumulated
//! statistics are kept and can be updated.
static bool on_stats(clingo_stats_t *stats, void *data) {
    bool status = true;
    uint64_t root = 0;
    uint64_t step = 0;
    uint64_t accu = 0;
    uint64_t key = 0;
    bool found = false;
    double value = 0;
    (void)data;

    CHECK(clingo_stats_root(stats, &root));

    // set a step statistic
    CHECK(clingo_stats_map_add_subkey(stats, root, clingo_user_stats_step.data, clingo_user_stats_step.size,
                                      clingo_stats_type_map, &step));
    CHECK(clingo_stats_map_add_subkey(stats, step, "example", strlen("example"), clingo_stats_type_value, &key));
    CHECK(clingo_stats_value_set(stats, key, 21));

    // increment an accumulated statistic
    CHECK(clingo_stats_map_add_subkey(stats, root, clingo_user_stats_accu.data, clingo_user_stats_accu.size,
                                      clingo_stats_type_map, &accu));
    CHECK(clingo_stats_map_has_subkey(stats, accu, "example", strlen("example"), &key, &found));
    if (found) {
        CHECK(clingo_stats_value_get(stats, key, &value));
    } else {
        CHECK(clingo_stats_map_add_subkey(stats, accu, "example", strlen("example"), clingo_stats_type_value, &key));
    }
    CHECK(clingo_stats_value_set(stats, key, value + 21));

cleanup:
    return status;
}

//! Simple example showing how to add and inspect statistics.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_solve_handle_t *hnd = NULL;
    clingo_string_builder_t *bld = NULL;
    clingo_stats_t const *stats = NULL;
    uint64_t root = 0;
    uint64_t key = 0;
    double value = 0;
    clingo_string_t str = {NULL, 0};

    // without option --stats only basic statistics are gathered
    char const *arg_models = "0";
    char const *arg_stats = "--stats";
    clingo_string_t args[] = {{arg_models, strlen(arg_models)}, {arg_stats, strlen(arg_stats)}};
    char const *prg = "1 {a; b} 1.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    clingo_solve_event_handler_t seh = {&on_model, NULL, &on_stats, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;

    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, args, 2, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));
    CHECK(clingo_control_solve(ctl, clingo_solve_mode_default, NULL, 0, &seh, bld, &hnd));
    CHECK(clingo_solve_handle_get(hnd, &res));
    clingo_solve_handle_close(hnd);
    hnd = NULL;

    // look up a single value (keys can be separated by periods)
    CHECK(clingo_control_stats(ctl, &stats));
    CHECK(clingo_stats_root(stats, &root));
    CHECK(clingo_stats_map_at(stats, root, "summary.models.enumerated", strlen("summary.models.enumerated"), &key));
    CHECK(clingo_stats_value_get(stats, key, &value));
    printf("enumerated models: %g\n", value);

    // print all statistics
    clingo_string_builder_clear(bld);
    CHECK(clingo_stats_to_string(stats, root, bld));
    CHECK(clingo_string_builder_string(bld, &str));
    printf("%.*s\n", (int)str.size, str.data);

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
