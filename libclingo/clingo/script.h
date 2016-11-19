// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef CLINGO_SCRIPT_H
#define CLINGO_SCRIPT_H

#include <clingo.h>

typedef struct clingo_script_ {
    bool (*execute) (clingo_location_t loc, char const *code, void *data);
    bool (*call) (clingo_location_t loc, char const *name, clingo_symbol_t const *arguments, size_t arguments_size, clingo_symbol_callback_t *symbol_callback, void *symbol_callback_data, void *data);
    bool (*callable) (char const * name, bool *ret, void *data);
    bool (*main) (clingo_control_t *ctl, void *data);
    void (*free) (void *data);
    char const *version;
} clingo_script_t_;

extern "C" CLINGO_VISIBILITY_DEFAULT bool clingo_register_script_(clingo_ast_script_type_t type, clingo_script_t_ *script, void *data);
extern "C" CLINGO_VISIBILITY_DEFAULT char const *clingo_script_version_(clingo_ast_script_type_t type);

#endif // CLINGO_SCRIPT_H


