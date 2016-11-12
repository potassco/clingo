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

#include <lua.hpp>

#if defined  _WIN32 || defined __CYGWIN__
#    define VISIBILITY_DEFAULT __declspec (dllexport)
#else
#    if __GNUC__ >= 4
#        define VISIBILITY_DEFAULT  __attribute__ ((visibility ("default")))
#    else
#        define VISIBILITY_DEFAULT
#    endif
#endif

extern "C" VISIBILITY_DEFAULT int clingo_lua_(lua_State *L);

extern "C" VISIBILITY_DEFAULT int luaopen_clingo(lua_State *L) {
    return clingo_lua_(L);
}

