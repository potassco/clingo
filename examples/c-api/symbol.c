// NOLINTNEXTLINE(STDC_FORMAT_MACROS)
#define STDC_FORMAT_MACROS
#include <assert.h>
#include <clingo.h>
#include <stdio.h>
#include <stdlib.h>

enum constants {
    message_limit = 25,
    example_number = 42,
};

typedef struct {
    char *string;
    size_t string_n;
} string_buffer_t;

bool handle_result(clingo_result_t res) {
    if (res != clingo_result_success) {
        printf("%s\n", clingo_result_string(res));
        return false;
    }
    return true;
}

void free_string_buffer(string_buffer_t *buf) {
    if (buf->string) {
        free(buf->string);
        buf->string = NULL;
        buf->string_n = 0;
    }
}

clingo_result_t print_symbol(clingo_lib_t *lib, clingo_symbol_t symbol, string_buffer_t *buf) {
    clingo_result_t ret = clingo_result_success;
    char *string = NULL;
    size_t n = 0;

    // determine size of the string representation of the next symbol in the model
    ret = clingo_symbol_to_string_size(symbol, &n);
    if (ret != clingo_result_success) {
        return ret;
    }

    if (buf->string_n < n) {
        // allocate required memory to hold the symbol's string
        string = (char *)realloc(buf->string, sizeof(*buf->string) * n);
        if (string == NULL) {
            return clingo_result_bad_alloc;
        }

        buf->string = string;
        buf->string_n = n;
    }

    // retrieve the symbol's string
    ret = clingo_symbol_to_string(symbol, buf->string, n);
    if (ret != clingo_result_success) {
        return ret;
    }

    printf("%s", buf->string);
    return clingo_result_success;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    clingo_result_t res = clingo_result_success;
    int ret = 0;
    clingo_lib_t *lib = NULL;
    clingo_symbol_t symbols[] = {0, 0, 0};
    string_buffer_t buf = {NULL, 0};
    clingo_symbol_t const *args = NULL;
    size_t size = 0;

    res = clingo_lib_new(0, NULL, NULL, NULL, message_limit, &lib);
    if (res != clingo_result_success) {
        handle_result(clingo_result_bad_alloc);
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
        res = print_symbol(lib, symbols[i], &buf);
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
        res = print_symbol(lib, symbols[0], &buf);
        if (!handle_result(res)) {
            goto out;
        }
        printf(" %s ", clingo_symbol_equal(symbols[0], args[i]) ? "is equal to" : "is not equal to");
        res = print_symbol(lib, args[i], &buf);
        if (!handle_result(res)) {
            goto out;
        }
        printf("\n");
    }
    // less than comparison
    res = print_symbol(lib, symbols[0], &buf);
    if (!handle_result(res)) {
        goto out;
    }
    printf(" %s ", clingo_symbol_compare(symbols[0], symbols[1]) < 0 ? "is less than" : "is not less than");
    res = print_symbol(lib, symbols[1], &buf);
    if (!handle_result(res)) {
        goto out;
    }
    printf("\n");
out:
    free_string_buffer(&buf);
    clingo_lib_free(lib, true);

    return ret;
}
