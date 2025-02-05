// NOLINTNEXTLINE(STDC_FORMAT_MACROS)
#define STDC_FORMAT_MACROS

#include <clingo/symbol.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

enum constants {
    message_limit = 25,
    example_number = 42,
};

bool handle_result(clingo_result_t res) {
    if (res != clingo_result_success) {
        printf("%s\n", clingo_result_string(res));
        return false;
    }
    return true;
}

clingo_result_t print_symbol(clingo_symbol_t symbol, clingo_string_builder_t *builder) {
    clingo_result_t ret = clingo_result_success;
    char const *string = NULL;

    // clear the string builder
    clingo_string_builder_clear(builder);

    // retrieve the symbol's string
    ret = clingo_symbol_to_string(symbol, builder);
    if (ret != clingo_result_success) {
        return ret;
    }

    // obtain the string stored in the builder
    ret = clingo_string_builder_string(builder, &string, NULL);
    if (ret != clingo_result_success) {
        return ret;
    }

    // print the string
    printf("%s", string);
    return clingo_result_success;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    clingo_result_t res = clingo_result_success;
    int ret = 0;
    clingo_lib_t *lib = NULL;
    clingo_symbol_t symbols[] = {0, 0, 0};
    clingo_string_builder_t *builder = NULL;
    clingo_symbol_t const *args = NULL;
    size_t size = 0;

    res = clingo_string_builder_new(&builder);
    if (res != clingo_result_success) {
        handle_result(res);
        goto out;
    }

    res = clingo_lib_new(0, NULL, NULL, NULL, message_limit, &lib);
    if (res != clingo_result_success) {
        handle_result(res);
        goto out;
    }

    // create a number, identifier (function without arguments), and a function symbol
    symbols[0] = clingo_symbol_create_number(example_number);

    res = clingo_symbol_create_id(lib, "x", true, &symbols[1]);
    if (!handle_result(res)) {
        goto out;
    }
    res = clingo_symbol_create_function(lib, "x", symbols, 2, true, &symbols[2]);
    if (!handle_result(res)) {
        goto out;
    }

    // print the symbols along with their hash values
    for (size_t i = 0; i < sizeof(symbols) / sizeof(*symbols); ++i) {
        printf("the hash of ");
        res = print_symbol(symbols[i], builder);
        if (!handle_result(res)) {
            goto out;
        }
        printf(" is %zu\n", clingo_symbol_hash(symbols[i]));
    }

    // compare symbols
    res = clingo_symbol_arguments(symbols[2], &args, &size);
    if (!handle_result(res)) {
        goto out;
    }
    assert(size == 2);
    // equal to comparison
    for (size_t i = 0; i < size; ++i) {
        res = print_symbol(symbols[0], builder);
        if (!handle_result(res)) {
            goto out;
        }
        printf(" %s ", clingo_symbol_equal(symbols[0], args[i]) ? "is equal to" : "is not equal to");
        res = print_symbol(args[i], builder);
        if (!handle_result(res)) {
            goto out;
        }
        printf("\n");
    }
    // less than comparison
    res = print_symbol(symbols[0], builder);
    if (!handle_result(res)) {
        goto out;
    }
    printf(" %s ", clingo_symbol_compare(symbols[0], symbols[1]) < 0 ? "is less than" : "is not less than");
    res = print_symbol(symbols[1], builder);
    if (!handle_result(res)) {
        goto out;
    }
    printf("\n");
out:
    clingo_string_builder_free(builder);
    clingo_lib_free(lib, true);

    return ret;
}
