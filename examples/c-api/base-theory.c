#include <clingo/base.h>
#include <clingo/control.h>

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

//! Print the string stored in the given builder.
static bool print_builder(char const *prefix, clingo_string_builder_t const *bld) {
    clingo_string_t str = {NULL, 0};
    if (!clingo_string_builder_string(bld, &str)) {
        return false;
    }
    printf("%s%.*s\n", prefix, (int)str.size, str.data);
    return true;
}

//! Print the given theory atom along with its elements and guard.
static bool print_theory_atom(clingo_theory_base_t const *theory, clingo_id_t atom, clingo_string_builder_t *bld) {
    bool status = true;
    clingo_id_t const *elements = NULL;
    size_t size = 0;
    clingo_literal_t lit = 0;
    bool has_guard = false;

    // print the whole atom and its program literal
    clingo_string_builder_clear(bld);
    CHECK(clingo_theory_base_atom_to_string(theory, atom, bld));
    CHECK(print_builder("theory atom: ", bld));
    CHECK(clingo_theory_base_atom_literal(theory, atom, &lit));
    printf("  literal: %d\n", (int)lit);

    // print the elements of the atom
    CHECK(clingo_theory_base_atom_elements(theory, atom, &elements, &size));
    for (size_t i = 0; i < size; ++i) {
        clingo_string_builder_clear(bld);
        CHECK(clingo_theory_base_element_to_string(theory, elements[i], bld));
        CHECK(print_builder("  element: ", bld));
    }

    // print the guard of the atom
    CHECK(clingo_theory_base_atom_has_guard(theory, atom, &has_guard));
    if (has_guard) {
        clingo_string_t op = {NULL, 0};
        clingo_id_t term = 0;
        CHECK(clingo_theory_base_atom_guard(theory, atom, &op, &term));
        clingo_string_builder_clear(bld);
        CHECK(clingo_theory_base_term_to_string(theory, term, bld));
        printf("  guard: %.*s ", (int)op.size, op.data);
        CHECK(print_builder("", bld));
    }

cleanup:
    return status;
}

//! Simple example showing how to inspect the theory atoms of a grounded program.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_string_builder_t *bld = NULL;
    clingo_base_t const *base = NULL;
    clingo_theory_base_t const *theory = NULL;
    size_t size = 0;

    char const *prg = "#theory t {\n"
                      "  term { + : 1, binary, left };\n"
                      "  &a/0 : term, any;\n"
                      "  &b/1 : term, {=}, term, any\n"
                      "}.\n"
                      "{ x; y }.\n"
                      "&a { 1+2: x }.\n"
                      "&b(3) { 4 } = 17 :- y.\n";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};

    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, NULL, 0, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));

    // iterate over the grounded theory atoms
    CHECK(clingo_control_base(ctl, &base));
    CHECK(clingo_base_theory(base, &theory));
    CHECK(clingo_theory_base_size(theory, &size));
    printf("number of grounded theory atoms: %zu\n", size);
    for (size_t i = 0; i < size; ++i) {
        CHECK(print_theory_atom(theory, (clingo_id_t)i, bld));
    }

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
