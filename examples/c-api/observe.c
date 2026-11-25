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

//! Print an array of literals.
static void print_literals(clingo_literal_t const *literals, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        printf("%s%d", i > 0 ? "," : "", literals[i]);
    }
}

//! Print a rule.
static bool on_rule(bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body,
                    size_t body_size, void *data) {
    (void)data;
    printf("rule: %s", choice ? "{" : "");
    for (size_t i = 0; i < head_size; ++i) {
        printf("%s%u", i > 0 ? ";" : "", head[i]);
    }
    printf("%s :- ", choice ? "}" : "");
    print_literals(body, body_size);
    printf(".\n");
    return true;
}

//! Print a minimize constraint.
static bool on_minimize(clingo_weight_t priority, clingo_weighted_literal_t const *literals, size_t size, void *data) {
    (void)data;
    printf("minimize: priority=%d", priority);
    for (size_t i = 0; i < size; ++i) {
        printf(" %d*%d", literals[i].weight, literals[i].literal);
    }
    printf("\n");
    return true;
}

//! Print an external statement.
static bool on_external(clingo_atom_t atom, clingo_external_type_t type, void *data) {
    static char const *const names[] = {"free", "true", "false", "release"};
    (void)data;
    printf("external: %u %s\n", atom, names[type]);
    return true;
}

//! Simple example showing how to inspect the ground program.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;

    char const *prg = "{ a }. b :- a. #minimize { 1 : b }. #external c.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    // callbacks that are not needed can be set to NULL
    clingo_observer_t observer = {.rule = &on_rule, .minimize = &on_minimize, .external = &on_external};
    clingo_string_t aspif = {NULL, 0};

    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, NULL, 0, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));

    // inspect the ground program without preprocessing
    CHECK(clingo_control_observe(ctl, &observer, NULL, false));

    // write the ground program in aspif format to the output buffer
    CHECK(clingo_control_write_aspif(ctl, NULL, 0, clingo_write_aspif_mode_preamble));
    CHECK(clingo_control_buffer(ctl, &aspif));
    printf("aspif:\n%.*s", (int)aspif.size, aspif.data);

cleanup:
    if (!status) {
        clingo_string_t err_msg = {NULL, 0};
        clingo_result_t code = 0;
        clingo_get_error(&code, &err_msg);
        fprintf(stderr, "Error: %.*s\n", (int)err_msg.size, err_msg.data);
    }

    clingo_control_release(ctl);
    clingo_lib_release(lib);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
