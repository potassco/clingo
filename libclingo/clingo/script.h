// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#ifndef CLINGO_SCRIPT_H
#define CLINGO_SCRIPT_H

#include <clingo.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_script_ {
    bool (*execute) (clingo_location_t const *loc, char const *code, void *data);
    bool (*call) (clingo_location_t const *loc, char const *name, clingo_symbol_t const *arguments, size_t arguments_size, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data);
    bool (*callable) (char const * name, bool *ret, void *data);
    bool (*main) (clingo_control_t *ctl, void *data);
    void (*free) (void *data);
    char const *version;
} clingo_script_t_;

CLINGO_VISIBILITY_DEFAULT bool clingo_register_script_(clingo_ast_script_type_t type, clingo_script_t_ const *script, void *data);
CLINGO_VISIBILITY_DEFAULT char const *clingo_script_version_(clingo_ast_script_type_t type);

#ifdef __cplusplus
}
#endif

#endif // CLINGO_SCRIPT_H


