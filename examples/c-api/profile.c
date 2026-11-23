#include <clingo/control.h>
#include <clingo/profile.h>

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

//! Print an internal node of the profile tree.
static bool on_internal(size_t depth, char const *key, size_t key_size, bool nested, void *data) {
    (void)nested;
    (void)data;
    printf("%*s%.*s\n", (int)(2 * depth), "", (int)key_size, key);
    return true;
}

//! Print a leaf node of the profile tree.
//!
//! The timings are not printed because they differ from run to run.
static bool on_leaf(size_t depth, clingo_profile_data_t *values, clingo_profile_type_t type, void *data) {
    (void)data;
    printf("%*s%s: matches=%llu instances=%llu\n", (int)(2 * depth), "",
           type == clingo_profile_type_step ? "step" : "accu", (unsigned long long)values->matches,
           (unsigned long long)values->instances);
    return true;
}

//! Simple example showing how to inspect profiling data gathered during grounding.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;

    // profiling has to be enabled via the --profile option
    char const *arg = "--profile";
    clingo_string_t args[] = {{arg, strlen(arg)}};
    char const *prg = "p(1..3). q(X) :- p(X).";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};
    clingo_profile_visitor_t visitor = {&on_internal, &on_leaf};

    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, args, 1, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));

    // visit the profile tree
    CHECK(clingo_control_profile(ctl, &visitor, NULL));

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
