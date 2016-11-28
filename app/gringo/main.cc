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

#ifdef CLINGO_WITH_PYTHON
#   include <pyclingo.h>
#endif
#ifdef CLINGO_WITH_LUA
#   include <luaclingo.h>
#endif

#include <clingo.h>
#include <iostream>

extern "C" CLINGO_VISIBILITY_DEFAULT int gringo_main_(int argc, char *argv[]);

int main(int argc, char *argv[]) {
#   ifdef CLINGO_WITH_PYTHON
    if (!clingo_register_python_()) {
        std::cerr << clingo_error_message() << std::endl;
        return 1;
    }
#   endif
#   ifdef CLINGO_WITH_LUA
    if (!clingo_register_lua_(nullptr)) {
        std::cerr << clingo_error_message() << std::endl;
        return 1;
    }
#   endif
    return gringo_main_(argc, argv);
}
