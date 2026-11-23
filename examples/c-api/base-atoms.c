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

//! Print the atoms in the given atom base along with their status.
static bool print_atoms(clingo_base_t const *base, clingo_atom_base_t const *atoms, clingo_string_builder_t *bld) {
    bool status = true;
    size_t size = 0;

    CHECK(clingo_atom_base_size(atoms, &size));
    for (size_t i = 0; i < size; ++i) {
        clingo_symbol_t sym = 0;
        clingo_literal_t lit = 0;
        clingo_string_t str = {NULL, 0};
        bool fact = false;
        bool external = false;

        // print the symbol
        CHECK(clingo_atom_base_symbol(atoms, i, &sym));
        clingo_string_builder_clear(bld);
        CHECK(clingo_symbol_to_string(sym, bld));
        CHECK(clingo_string_builder_string(bld, &str));
        printf("    %.*s", (int)str.size, str.data);

        // check whether the atom is a fact or an external
        CHECK(clingo_atom_base_literal(atoms, i, &lit));
        CHECK(clingo_base_is_fact(base, lit, &fact));
        CHECK(clingo_base_is_external(base, lit, &external));
        if (fact) {
            printf(", fact");
        }
        if (external) {
            printf(", external");
        }
        printf("\n");
    }

cleanup:
    return status;
}

//! Simple example showing how to iterate over the symbolic atoms of a grounded program.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_control_t *ctl = NULL;
    clingo_string_builder_t *bld = NULL;
    clingo_base_t const *base = NULL;
    size_t size = 0;

    char const *prg = "a. {b}. #external c.";
    char const *part_name = "base";
    clingo_part_t part = {part_name, strlen(part_name), NULL, 0};

    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(clingo_lib_flags_fast_release | clingo_lib_flags_slotted, clingo_log_level_info, NULL, NULL,
                         20, &lib));
    CHECK(clingo_control_new(lib, NULL, 0, &ctl));
    CHECK(clingo_control_parse_string(ctl, prg, strlen(prg)));
    CHECK(clingo_control_ground(ctl, &part, 1, NULL, NULL));

    // iterate over the signatures in the base and print their atoms
    CHECK(clingo_control_base(ctl, &base));
    CHECK(clingo_base_atoms_size(base, &size));
    printf("Symbolic atoms:\n");
    for (size_t i = 0; i < size; ++i) {
        clingo_signature_t sig = {NULL, 0, 0, true};
        clingo_atom_base_t const *atoms = NULL;
        CHECK(clingo_base_atoms_at(base, i, &sig, &atoms));
        printf("  %s%.*s/%zu:\n", sig.is_positive ? "" : "-", (int)sig.size, sig.name, sig.arity);
        CHECK(print_atoms(base, atoms, bld));
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
