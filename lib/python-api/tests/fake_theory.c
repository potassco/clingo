#include <clingo/theory.h>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    clingo_symbol_t syms[3];
} fake_data_t;

static void fake_destroy(void *self) {
    free(self);
}

static bool fake_info(void *self, clingo_string_t *name, int *maj, int *min, int *rev, clingo_string_t *hash) {
    (void)self;
    static const char n[] = "fake-theory";
    static const char h[] = "deadbeef";
    if (name) {
        name->data = n;
        name->size = sizeof(n) - 1;
    }
    if (maj) {
        *maj = 1;
    }
    if (min) {
        *min = 2;
    }
    if (rev) {
        *rev = 3;
    }
    if (hash) {
        hash->data = h;
        hash->size = sizeof(h) - 1;
    }
    return true;
}

static bool fake_lookup_symbol(void *self, clingo_symbol_t sym, size_t *idx, bool *found) {
    fake_data_t *d = (fake_data_t *)self;
    for (size_t i = 0; i < 3; ++i) {
        if (d->syms[i] == sym) {
            *idx = i;
            *found = true;
            return true;
        }
    }
    *found = false;
    return true;
}

static bool fake_assignment_next(void *self, uint32_t thread_id, bool *init, size_t *idx, bool *has_value) {
    (void)self;
    (void)thread_id;
    if (*init) {
        *idx = 0;
        *init = false;
        *has_value = true;
        return true;
    }
    if (*idx < 2) {
        *idx += 1;
        *has_value = true;
        return true;
    }
    *has_value = false;
    return true;
}

static bool fake_assignment_get_value(void *self, uint32_t thread_id, size_t idx, clingo_symbol_t *sym,
                                      clingo_theory_value_t *val, bool *has_value) {
    (void)thread_id;
    fake_data_t *d = (fake_data_t *)self;
    if (idx > 2) {
        *has_value = false;
        return true;
    }
    *has_value = true;
    if (sym) {
        *sym = d->syms[idx];
    }
    if (idx == 0) {
        val->type = clingo_theory_value_type_int;
        val->int_number = 7;
    } else if (idx == 1) {
        val->type = clingo_theory_value_type_double;
        val->double_number = 3.5;
    } else {
        val->type = clingo_theory_value_type_symbol;
        val->symbol = d->syms[0];
    }
    return true;
}

static bool fake_rewrite_ast(void *self, clingo_ast_t *ast, clingo_ast_callback_t cb, void *data) {
    (void)self;
    return cb(ast, data);
}

static bool fake_register_theory(void *self, clingo_control_t *control) {
    (void)self;
    (void)control;
    return true;
}

static bool fake_prepare(void *self, clingo_control_t *control) {
    (void)self;
    (void)control;
    return true;
}

static bool fake_on_model(void *self, clingo_model_t *model) {
    (void)self;
    (void)model;
    return true;
}

static bool fake_on_stats(void *self, clingo_stats_t *stats) {
    (void)self;
    (void)stats;
    return true;
}

static bool fake_register_options(void *self, clingo_options_t *options) {
    (void)self;
    (void)options;
    return true;
}

static bool fake_validate_options(void *self) {
    (void)self;
    return true;
}

CLINGO_VISIBILITY_DEFAULT
bool fake_theory_create(clingo_lib_t *lib, clingo_theory_t *theory) {
    (void)lib;
    fake_data_t *d = (fake_data_t *)calloc(1, sizeof(fake_data_t));
    if (!d) {
        return false;
    }

    d->syms[0] = clingo_symbol_create_number(10);
    d->syms[1] = clingo_symbol_create_number(20);
    d->syms[2] = clingo_symbol_create_number(30);

    memset(theory, 0, sizeof(*theory));
    theory->self = d;
    theory->destroy = fake_destroy;
    theory->lookup_symbol = fake_lookup_symbol;
    theory->rewrite_ast = fake_rewrite_ast;
    theory->register_theory = fake_register_theory;
    theory->prepare = fake_prepare;
    theory->on_model = fake_on_model;
    theory->on_stats = fake_on_stats;
    theory->register_options = fake_register_options;
    theory->validate_options = fake_validate_options;
    theory->info = fake_info;
    theory->assignment_next = fake_assignment_next;
    theory->assignment_get_value = fake_assignment_get_value;

    return true;
}
