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

void free_string_buffer(string_buffer_t *buf) {
    if (buf->string) {
        free(buf->string);
        buf->string = NULL;
        buf->string_n = 0;
    }
}

bool print_symbol(clingo_lib_t *lib, clingo_symbol_t symbol, string_buffer_t *buf) {
    bool ret = true;
    char *string = NULL;
    size_t n = 0;

    // determine size of the string representation of the next symbol in the model
    if (!clingo_symbol_to_string_size(symbol, &n)) {
        goto error;
    }

    if (buf->string_n < n) {
        // allocate required memory to hold the symbol's string
        string = (char *)realloc(buf->string, sizeof(*buf->string) * n);
        if (string == NULL) {
            clingo_set_error(lib, clingo_error_bad_alloc, "could not allocate memory for symbol's string");
            goto error;
        }

        buf->string = string;
        buf->string_n = n;
    }

    // retrieve the symbol's string
    if (!clingo_symbol_to_string(symbol, buf->string, n)) {
        goto error;
    }
    printf("%s", buf->string);
    goto out;

error:
    ret = false;

out:
    return ret;
}

int main() {
    char const *error_message = NULL;
    int ret = 0;
    clingo_lib_t *lib = NULL;
    clingo_symbol_t symbols[] = {0, 0, 0};
    string_buffer_t buf = {NULL, 0};
    clingo_symbol_t const *args = NULL;
    size_t size = 0;

    lib = clingo_lib_new(0, NULL, NULL, message_limit);
    if (lib == NULL) {
        goto error;
    }

    // create a number, identifier (function without arguments), and a function symbol
    symbols[0] = clingo_symbol_create_number(example_number);
    if (!clingo_symbol_create_id(lib, "x", true, &symbols[1])) {
        goto error;
    }
    if (!clingo_symbol_create_function(lib, "x", symbols, 2, true, &symbols[2])) {
        goto error;
    }

    // print the symbols along with their hash values
    for (size_t i = 0; i < sizeof(symbols) / sizeof(*symbols); ++i) {
        printf("the hash of ");
        if (!print_symbol(lib, symbols[i], &buf)) {
            goto error;
        }
        printf(" is %zu\n", clingo_symbol_hash(symbols[i]));
    }

    // compare symbols
    if (!clingo_symbol_arguments(symbols[2], &args, &size)) {
        goto error;
    }
    assert(size == 2);
    // equal to comparison
    for (size_t i = 0; i < size; ++i) {
        if (!print_symbol(lib, symbols[0], &buf)) {
            goto error;
        }
        printf(" %s ", clingo_symbol_is_equal_to(symbols[0], args[i]) ? "is equal to" : "is not equal to");
        if (!print_symbol(lib, args[i], &buf)) {
            goto error;
        }
        printf("\n");
    }
    // less than comparison
    if (!print_symbol(lib, symbols[0], &buf)) {
        goto error;
    }
    printf(" %s ", clingo_symbol_is_less_than(symbols[0], symbols[1]) ? "is less than" : "is not less than");
    if (!print_symbol(lib, symbols[1], &buf)) {
        goto error;
    }
    printf("\n");

    goto out;

error:
    error_message = clingo_error_message(lib);
    if (error_message == NULL) {
        error_message = "error";
    }

    printf("%s\n", error_message);
    ret = clingo_error_code(lib);

out:
    free_string_buffer(&buf);
    clingo_lib_free(lib);

    return ret;
}
