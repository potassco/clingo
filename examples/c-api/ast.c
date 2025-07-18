#include <clingo/ast.h>
#include <clingo/control.h>
#include <clingo/solve.h>
#include <clingo/symbol.h>

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
bool print_symbols(clingo_symbol_t const *symbols, size_t size, void *data) {
    bool status = true;
    clingo_string_builder_t *bld = (clingo_string_builder_t *)data;
    for (size_t i = 0; i < size; ++i) {
        clingo_string_builder_clear(bld);
        CHECK(clingo_symbol_to_string(symbols[i], bld));
        clingo_string_t str = {NULL, 0};
        CHECK(clingo_string_builder_string(bld, &str));
        printf(" %.*s", (int)str.size, str.data);
    }
cleanup:
    return status;
}

//! Print the given model.
bool on_model(clingo_model_t *model, void *data, bool *goon) {
    bool status = true;
    *goon = true;
    printf("Answer:");
    CHECK(clingo_model_symbols(model, clingo_show_type_shown, &print_symbols, data));
    printf("\n");
cleanup:
    return status;
}

// Build ":- not a." as a statement AST.
bool build_constraint(clingo_lib_t *lib, clingo_location_t const *loc, clingo_ast_t **rule) {
    bool status = true;
    char const *atom_str = "a";
    clingo_ast_t *args = NULL;
    clingo_ast_t *term = NULL;
    clingo_ast_t *fail = NULL;
    clingo_ast_t *atom = NULL;
    clingo_ast_t *body = NULL;
    clingo_ast_t *head = NULL;

    // Construct emty argument tuple "()".
    CHECK(clingo_ast_construct(lib, clingo_ast_type_argument_tuple, &args, NULL, (size_t)0));
    // Construct function term "a()" equivalent to "a".
    CHECK(clingo_ast_construct(lib, clingo_ast_type_term_function, &term, loc, atom_str, strlen(atom_str), &args,
                               (size_t)1, 0));
    // Construct simple literal "not a".
    CHECK(clingo_ast_construct(lib, clingo_ast_type_literal_symbolic, &atom, loc, 1, term));
    // Construct body literal "not a".
    CHECK(clingo_ast_construct(lib, clingo_ast_type_body_simple_literal, &body, atom));
    // Construct simple literal "#false".
    CHECK(clingo_ast_construct(lib, clingo_ast_type_literal_boolean, &fail, loc, 0, false));
    // Construct head literal "#false".
    CHECK(clingo_ast_construct(lib, clingo_ast_type_head_simple_literal, &head, fail));
    // Construct integrity constraint ":- not a.".
    CHECK(clingo_ast_construct(lib, clingo_ast_type_statement_rule, rule, loc, head, &body, (size_t)1));

cleanup:
    clingo_ast_free(args);
    clingo_ast_free(term);
    clingo_ast_free(atom);
    clingo_ast_free(fail);
    clingo_ast_free(head);
    clingo_ast_free(body);
    return status;
}

// Simple example that shows how to work with ASTs in C.
int main(void) {
    bool status = true;

    clingo_string_builder_t *bld = NULL;

    clingo_lib_t *lib = NULL;
    clingo_lib_flags_t flags = clingo_lib_flags_fast_release | clingo_lib_flags_slotted;
    clingo_log_level_t level = clingo_log_level_info;
    size_t const limit = 20;

    clingo_control_t *ctl = NULL;
    clingo_solve_handle_t *hnd = NULL;
    clingo_solve_event_handler_t seh = {&on_model, NULL, NULL, NULL, NULL};
    clingo_solve_result_bitset_t res = clingo_solve_result_empty;
    char const *model_str = "0";
    char const *base_str = "base";
    clingo_string_t str = {model_str, strlen(model_str)};
    clingo_part_t part = {base_str, strlen(base_str), NULL, 0};

    clingo_program_t *prog = NULL;
    clingo_location_t const *loc = NULL;
    clingo_ast_t *parsed = NULL;
    clingo_ast_t *built = NULL;
    char const *stm = "{a}.";

    //! Construct a control object along with a library and a string builder.
    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(flags, level, NULL, NULL, limit, &lib));
    CHECK(clingo_control_new(lib, &str, 1, &ctl));

    // Parse "{a}." as a statement AST.
    CHECK(clingo_ast_parse_expression(lib, clingo_ast_parse_type_statement, stm, strlen(stm), &parsed));
    CHECK(clingo_ast_attribute_get_location(parsed, clingo_ast_attribute_location, &loc));

    // Build ":- not a." as a statement AST.
    CHECK(build_constraint(lib, loc, &built));

    // Create a program and add both statements.
    CHECK(clingo_program_new(lib, &prog));
    CHECK(clingo_program_add(prog, parsed));
    CHECK(clingo_program_add(prog, built));

    // Add program to control.
    CHECK(clingo_control_join(ctl, prog));

    // Ground and solve.
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

    clingo_ast_free(parsed);
    clingo_ast_free(built);
    clingo_program_free(prog);
    clingo_solve_handle_close(hnd);
    clingo_control_release(ctl);
    clingo_lib_release(lib);
    clingo_string_builder_free(bld);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
