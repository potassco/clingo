#include <cclingo.h>
#include <stdio.h>
#include <stdlib.h>

int report_error(clingo_error_t err) {
    if (err != clingo_error_success) {
        printf("an error occurred: %s\n", clingo_error_str(err));
        return 1;
    }
    return 0;
}

int on_model(clingo_model_t *model, void *data) {
    (void)data;
    // NOTE: should a mechanism to report errors be added?
    clingo_value_span_t atoms;
    if (report_error(clingo_model_atoms(model, clingo_show_type_atoms | clingo_show_type_csp, &atoms))) {
        return 0;
    }
    printf("Model:");
    clingo_value_t const *it, *ie;
    for (it = atoms.data, ie = it + atoms.size; it != ie; ++it) {
        char *str;
        if (report_error(clingo_value_to_string(*it, &str))) {
            free((void*)atoms.data);
            return 0;
        }
        printf(" %s", str);
        free(str);
    }
    printf("\n");
    return 1;
}

int solve(clingo_control_t *ctl) {
    clingo_solve_result_t ret;
    if (report_error(clingo_control_solve(ctl, 0, &on_model, 0, &ret))) { return 0; }
    printf("the solve result is: %d\n", ret);
    return 1;
}
int solve_iter(clingo_control_t *ctl) {
    clingo_solve_iter_t *it;
    if (report_error(clingo_control_solve_iter(ctl, 0, &it))) { return 0; }
    for (;;) {
        clingo_model_t *m;
        if (report_error(clingo_solve_iter_next(it, &m))) { return 0; }
        if (!m) { break; }
        on_model(m, 0);
    }
    clingo_solve_result_t ret;
    if (report_error(clingo_solve_iter_get(it, &ret))) { return 0; }
    printf("the solve result is: %d\n", ret);
    if (report_error(clingo_solve_iter_close(it))) { return 0; }
    return 1;
}

int run(clingo_control_t *ctl) {
    char const *base_params[] = { 0 };
    if (report_error(clingo_control_add(ctl, "base", base_params, "a :- not b. b :- not a."))) { return 0; }
    clingo_value_t empty[] = {};
    clingo_part_t part_vec[] = { {"base", { empty, 0 } } };
    clingo_part_span_t part_span = { part_vec, 1 };
    if (report_error(clingo_control_ground(ctl, part_span, 0, 0))) { return 0; }
    if (!solve(ctl)) { return 0; }
    if (!solve_iter(ctl)) { return 0; }
    return 1;
}

int main(int argc, char const **argv) {
    clingo_module_t *mod;
    if (report_error(clingo_module_new(&mod))) {
        return 1;
    }
    clingo_control_t *ctl;
    if (report_error(clingo_control_new(mod, argc, argv, &ctl))) {
        clingo_module_free(mod);
        return 1;
    }
    run(ctl);

    clingo_control_free(ctl);
    clingo_module_free(mod);
}

