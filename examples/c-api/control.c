#include <clingo.h>

// NOLINTNEXTLINE(STDC_FORMAT_MACROS)
#define STDC_FORMAT_MACROS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GC(f) __attribute__((cleanup(f)))

void free_lib(clingo_lib_t **lib) {
    clingo_lib_release(*lib);
}
void free_ctl(clingo_control_t **ctl) {
    clingo_control_release(*ctl);
}
void handle_error(bool ret) {
    if (!ret) {
        exit(1);
    }
}

bool print_symbols(clingo_symbol_t const *symbols, size_t size, void *data) {
    (void)data;
    clingo_string_t str;
    bool res = true;
    clingo_string_builder_t *bld = NULL;
    res = clingo_string_builder_new(&bld);
    if (!res) {
        goto out;
    }
    for (size_t i = 0; i != size; ++i) {
        clingo_string_builder_clear(bld);
        res = clingo_symbol_to_string(symbols[i], bld);
        if (!res) {
            goto out;
        }
        res = clingo_string_builder_string(bld, &str);
        if (!res) {
            goto out;
        }
        printf(" %.*s", (int)str.size, str.data);
    }

out:
    clingo_string_builder_free(bld);
    return res;
}

bool on_model(clingo_model_t *mdl, void *data, bool *goon) {
    (void)data;
    printf("Answer:");
    bool res = clingo_model_symbols(mdl, clingo_show_type_shown, print_symbols, NULL);
    if (!res) {
        return res;
    }
    printf("\n");
    *goon = true;
    return true;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    clingo_lib_t *lib GC(free_lib) = NULL;
    handle_error(clingo_lib_new(0, clingo_log_level_info, NULL, NULL, 0, &lib));

    clingo_control_t *ctl GC(free_ctl) = NULL;
    handle_error(clingo_control_new(lib, NULL, 0, &ctl));

    char const *prg = "1 {a; b} 1.";
    handle_error(clingo_control_parse_string(ctl, prg, strlen(prg)));

    clingo_part_t const parts[] = {{"base", 4, NULL, 0}};
    handle_error(clingo_control_ground(ctl, parts, 1, NULL, NULL));

    clingo_solve_event_handler_t const seh = {on_model, NULL, NULL, NULL, NULL};

    clingo_solve_handle_t *hnd = NULL;
    handle_error(clingo_control_solve(ctl, clingo_solve_mode_async, NULL, 0, &seh, NULL, &hnd));

    clingo_solve_result_bitset_t res = 0;
    clingo_solve_handle_get(hnd, &res);

    printf("solving finished with result: %d\n", res);

    return 0;
}
