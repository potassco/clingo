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
#include "Python.h"

#include <pyclingo.h>

#if defined  _WIN32 || defined __CYGWIN__
#    define VISIBILITY_DEFAULT __declspec (dllexport)
#else
#    if __GNUC__ >= 4
#        define VISIBILITY_DEFAULT  __attribute__ ((visibility ("default")))
#    else
#        define VISIBILITY_DEFAULT
#    endif
#endif

#if PY_MAJOR_VERSION >= 3
extern "C" VISIBILITY_DEFAULT PyObject *PyInit_clingo() {
    clingo_register_python_();
    return (PyObject*)clingo_init_python_();
}
#else
extern "C" VISIBILITY_DEFAULT void initclingo() {
    clingo_register_python_();
    clingo_init_python_();
}
#endif

