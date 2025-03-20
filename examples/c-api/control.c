#include <clingo.h>

// NOLINTNEXTLINE(STDC_FORMAT_MACROS)
#define STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>

#define GC(f) __attribute__((cleanup(f)))

void free_lib(clingo_lib_t **lib) {
    clingo_lib_free(*lib, true);
}
void free_ctl(clingo_control_t **ctl) {
    clingo_control_free(*ctl);
}
void handle_error(clingo_result_t ret) {
    if (ret != clingo_result_success) {
        exit(1);
    }
}

clingo_result_t print_symbols(clingo_symbol_t const *symbols, size_t size, void *data) {
    (void)data;
    char const *str = NULL;
    clingo_result_t res = clingo_result_success;
    clingo_string_builder_t *bld = NULL;
    res = clingo_string_builder_new(&bld);
    if (res != clingo_result_success) {
        goto out;
    }
    for (size_t i = 0; i != size; ++i) {
        clingo_string_builder_clear(bld);
        res = clingo_symbol_to_string(symbols[i], bld);
        if (res != clingo_result_success) {
            goto out;
        }
        res = clingo_string_builder_string(bld, &str, NULL);
        if (res != clingo_result_success) {
            goto out;
        }
        printf(" %s", str);
    }

out:
    clingo_string_builder_free(bld);
    return res;
}

clingo_result_t on_model(clingo_solve_event_type_t type, void *event, void *data, bool *goon) {
    (void)data;
    if (type == clingo_solve_event_type_model) {
        printf("Answer:");
        clingo_model_t *mdl = (clingo_model_t *)(event);
        clingo_result_t res = clingo_model_symbols(mdl, clingo_show_type_shown, print_symbols, NULL);
        if (res != clingo_result_success) {
            return res;
        }
        printf("\n");
    }
    *goon = true;
    return clingo_result_success;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    clingo_lib_t *lib GC(free_lib) = NULL;
    handle_error(clingo_lib_new(0, clingo_log_level_info, NULL, NULL, 0, &lib));

    clingo_control_t *ctl GC(free_ctl) = NULL;
    handle_error(clingo_control_new(lib, NULL, 0, &ctl));

    handle_error(clingo_control_parse_string(ctl, "1 {a; b} 1."));

    clingo_part_t parts[] = {{"base", NULL, 0}};
    handle_error(clingo_control_ground(ctl, parts, 1, NULL, NULL));

    clingo_solve_handle_t *hnd = NULL;
    handle_error(clingo_control_solve(ctl, clingo_solve_mode_async, NULL, 0, on_model, NULL, &hnd));

    clingo_solve_result_bitset_t res = 0;
    clingo_solve_handle_get(hnd, &res);

    printf("solving finished with result: %d\n", res);

    return 0;
}
