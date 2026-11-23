#include <clingo/symbol.h>

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

//! Print the given symbol.
static bool print_symbol(clingo_symbol_t symbol, clingo_string_builder_t *bld) {
    bool status = true;
    clingo_string_t str = {NULL, 0};

    clingo_string_builder_clear(bld);
    CHECK(clingo_symbol_to_string(symbol, bld));
    CHECK(clingo_string_builder_string(bld, &str));
    printf("%.*s", (int)str.size, str.data);

cleanup:
    return status;
}

//! Simple example showing how to create, print, and compare symbols.
int main(void) {
    bool status = true;

    clingo_lib_t *lib = NULL;
    clingo_string_builder_t *bld = NULL;
    clingo_symbol_t symbols[] = {0, 0, 0};
    size_t const n = sizeof(symbols) / sizeof(*symbols);
    clingo_symbol_t const *args = NULL;
    size_t size = 0;
    int32_t const number = 42;

    CHECK(clingo_string_builder_new(&bld));
    CHECK(clingo_lib_new(0, clingo_log_level_info, NULL, NULL, 20, &lib));

    // create a number, an identifier (function without arguments), and a function symbol
    //
    // Numbers do not need reference counting while symbols created with
    // functions taking a library object have to be released again.
    symbols[0] = clingo_symbol_create_number(number);
    CHECK(clingo_symbol_create_id(lib, "x", 1, true, &symbols[1]));
    CHECK(clingo_symbol_create_function(lib, "x", 1, symbols, 2, true, &symbols[2]));

    // print the symbols along with their hash values
    for (size_t i = 0; i < n; ++i) {
        printf("the hash of ");
        CHECK(print_symbol(symbols[i], bld));
        printf(" is %zu\n", clingo_symbol_hash(symbols[i]));
    }

    // inspect the arguments of the function symbol
    CHECK(clingo_symbol_arguments(symbols[2], &args, &size));
    ERROR(size != 2, logic, "unexpected number of arguments");

    // equal to comparison
    for (size_t i = 0; i < size; ++i) {
        CHECK(print_symbol(symbols[0], bld));
        printf(" %s ", clingo_symbol_equal(symbols[0], args[i]) ? "is equal to" : "is not equal to");
        CHECK(print_symbol(args[i], bld));
        printf("\n");
    }

    // less than comparison
    CHECK(print_symbol(symbols[0], bld));
    printf(" %s ", clingo_symbol_compare(symbols[0], symbols[1]) < 0 ? "is less than" : "is not less than");
    CHECK(print_symbol(symbols[1], bld));
    printf("\n");

cleanup:
    if (!status) {
        clingo_string_t err_msg = {NULL, 0};
        clingo_result_t code = 0;
        clingo_get_error(&code, &err_msg);
        fprintf(stderr, "Error: %.*s\n", (int)err_msg.size, err_msg.data);
    }

    clingo_symbol_release(symbols[1]);
    clingo_symbol_release(symbols[2]);
    clingo_string_builder_free(bld);
    clingo_lib_release(lib);

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
