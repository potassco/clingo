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

//! Simple example showing how to inspect and modify the solver configuration.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_solve_handle_t *hnd = NULL;
    clingo_string_builder_t *bld = NULL;
    clingo_config_t *cfg = NULL;
    clingo_id_t root = 0;
    clingo_id_t solve = 0;
    clingo_id_t models = 0;
    bool found = false;
    clingo_string_t str = {NULL, 0};
    bool has_value = false;

    char const *prg = "1 {a; b} 1.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;

    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, NULL, 0, &ctl));

    // get the root key of the configuration
    CHECK(clingo_control_config(ctl, &cfg));
    CHECK(clingo_config_root(cfg, &root));

    // print the solve options
    CHECK(clingo_config_map_at(cfg, root, "solve", strlen("solve"), &solve, &found));
    ERROR(!found, logic, "configuration entry solve not found");
    clingo_string_builder_clear(bld);
    CHECK(clingo_config_to_string(cfg, solve, bld));
    CHECK(clingo_string_builder_string(bld, &str));
    printf("solve options:\n%.*s\n", (int)str.size, str.data);

    // look up the number of models to compute (keys can be separated by periods)
    CHECK(clingo_config_map_at(cfg, root, "solve.models", strlen("solve.models"), &models, &found));
    ERROR(!found, logic, "configuration entry solve.models not found");
    CHECK(clingo_config_description(cfg, models, &str));
    printf("description: %.*s\n", (int)str.size, str.data);
    CHECK(clingo_config_value_get(cfg, models, &str, &has_value));
    if (has_value) {
        printf("value: %.*s\n", (int)str.size, str.data);
    }

    // enumerate all models
    CHECK(clingo_config_value_set(cfg, models, "0", 1));

    // ground and solve
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
