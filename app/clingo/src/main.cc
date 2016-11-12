// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Benjamin Kaufmann

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

#ifdef WITH_PYTHON
#  include <Python.h>
#endif
#ifdef WITH_LUA
#  include <lua.hpp>
#endif
#include <python.hh>

typedef struct clingo_application {
    char const *python_version;
    char const *lua_version;
    void (*setup) (clingo_control_t *control);
} clingo_application_t;

extern "C" CLINGO_VISIBILITY_DEFAULT int clingo_main_(int argc, char *argv[], clingo_application_t *application);

int main(int argc, char** argv) {
    clingo_application_t app = {
#ifdef WITH_PYTHON
        "with Python " PY_VERSION
#else
        "without Python"
#endif
        ,
#ifdef WITH_LUA
        "with " LUA_RELEASE
#else
        "without Lua"
#endif
        ,
        [](clingo_control_t *ctl) {
            Gringo::registerPython(ctl, clingo_control_new);
            // TOOD: register lua too!
        }
    };
    return clingo_main_(argc, argv, &app);
}

