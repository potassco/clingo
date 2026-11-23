#include <clingo/control.h>
#include <clingo/solve.h>
#include <clingo/stats.h>
#include <clingo/theory.h>

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

// {{{1 theory

//! The state of the theory.
typedef struct {
    uint64_t models; //!< the number of models found
} counter_t;

//! Get the name and version of the theory.
static bool counter_info(void *self, clingo_string_t *name, int *major, int *minor, int *revision,
                         clingo_string_t *hash) {
    (void)self;
    static char const theory_name[] = "counter";
    // all output parameters are optional
    if (name != NULL) {
        name->data = theory_name;
        name->size = sizeof(theory_name) - 1;
    }
    if (major != NULL) {
        *major = 1;
    }
    if (minor != NULL) {
        *minor = 0;
    }
    if (revision != NULL) {
        *revision = 0;
    }
    if (hash != NULL) {
        hash->data = NULL;
        hash->size = 0;
    }
    return true;
}

//! Free the state of the theory.
static void counter_destroy(void *self) {
    free(self);
}

//! Register the theory with the control object.
static bool counter_register(void *self, clingo_control_t *control) {
    (void)self;
    (void)control;
    // a theory might register propagators or add theory definitions here
    return true;
}

//! Prepare the theory before solving.
static bool counter_prepare(void *self, clingo_control_t *control) {
    (void)control;
    ((counter_t *)self)->models = 0;
    return true;
}

//! Count the models found.
static bool counter_on_model(void *self, clingo_model_t *model) {
    (void)model;
    ((counter_t *)self)->models += 1;
    return true;
}

//! Add the number of models found to the user statistics.
static bool counter_on_stats(void *self, clingo_stats_t *stats) {
    bool status = true;
    counter_t *counter = (counter_t *)self;
    uint64_t root = 0;
    uint64_t step = 0;
    uint64_t key = 0;

    CHECK(clingo_stats_root(stats, &root));
    CHECK(clingo_stats_map_add_subkey(stats, root, clingo_user_stats_step.data, clingo_user_stats_step.size,
                                      clingo_stats_type_map, &step));
    CHECK(clingo_stats_map_add_subkey(stats, step, "counter", strlen("counter"), clingo_stats_type_value, &key));
    CHECK(clingo_stats_value_set(stats, key, (double)counter->models));

cleanup:
    return status;
}

//! Create the theory.
//!
//! Only some of the callbacks are implemented; the remaining ones are set to
//! NULL indicating that they are not supported.
static bool counter_create(clingo_theory_t *theory) {
    bool status = true;
    counter_t *counter = (counter_t *)malloc(sizeof(counter_t));
    ERROR(counter == NULL, bad_alloc, "failed to allocate theory");
    counter->models = 0;

    memset(theory, 0, sizeof(*theory));
    theory->self = counter;
    theory->info = &counter_info;
    theory->destroy = &counter_destroy;
    theory->register_theory = &counter_register;
    theory->prepare = &counter_prepare;
    theory->on_model = &counter_on_model;
    theory->on_stats = &counter_on_stats;

cleanup:
    return status;
}

// {{{1 application

//! Forward models to the theory.
static bool on_model(clingo_model_t *model, void *data, bool *goon) {
    clingo_theory_t const *theory = (clingo_theory_t const *)data;
    *goon = true;
    return theory->on_model(theory->self, model);
}

//! Forward statistics to the theory.
static bool on_stats(clingo_stats_t *stats, void *data) {
    clingo_theory_t const *theory = (clingo_theory_t const *)data;
    return theory->on_stats(theory->self, stats);
}

//! Simple example showing how to implement a theory and how to drive it.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_solve_handle_t *hnd = NULL;
    clingo_theory_t theory;
    clingo_string_t name = {NULL, 0};
    int major = 0;
    int minor = 0;
    int revision = 0;
    clingo_stats_t const *stats = NULL;
    uint64_t root = 0;
    uint64_t key = 0;
    double value = 0;

    // user statistics are only reported with option --stats
    char const *arg_models = "0";
    char const *arg_stats = "--stats";
    clingo_string_t args[] = {{arg_models, strlen(arg_models)}, {arg_stats, strlen(arg_stats)}};
    char const *prg = "1 {a; b; c}.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    clingo_solve_event_handler_t seh = {&on_model, NULL, &on_stats, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;

    memset(&theory, 0, sizeof(theory));

    // create the theory and print its name and version
    CHECK(counter_create(&theory));
    CHECK(theory.info(theory.self, &name, &major, &minor, &revision, NULL));
    printf("theory %.*s version %d.%d.%d\n", (int)name.size, name.data, major, minor, revision);

    // register the theory, ground, and prepare the theory for solving
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, args, 2, &ctl));
    CHECK(theory.register_theory(theory.self, ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));
    CHECK(theory.prepare(theory.self, ctl));

    // solve passing models and statistics to the theory
    CHECK(clingo_control_solve(ctl, clingo_solve_mode_default, NULL, 0, &seh, &theory, &hnd));
    CHECK(clingo_solve_handle_get(hnd, &res));
    clingo_solve_handle_close(hnd);
    hnd = NULL;

    // print the statistics added by the theory
    CHECK(clingo_control_stats(ctl, &stats));
    CHECK(clingo_stats_root(stats, &root));
    CHECK(clingo_stats_map_at(stats, root, clingo_user_stats_step.data, clingo_user_stats_step.size, &key));
    CHECK(clingo_stats_map_at(stats, key, "counter", strlen("counter"), &key));
    CHECK(clingo_stats_value_get(stats, key, &value));
    printf("models counted by theory: %g\n", value);

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
    if (theory.destroy != NULL) {
        theory.destroy(theory.self);
    }

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
