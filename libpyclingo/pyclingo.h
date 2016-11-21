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

#ifndef PYCLINGO_PYCLINGO_H
#define PYCLINGO_PYCLINGO_H

#include <clingo/script.h>

#if defined _WIN32 || defined __CYGWIN__
#   define PYCLINGO_WIN
#endif
#ifdef PYCLINGO_NO_VISIBILITY
#   define PYCLINGO_VISIBILITY_DEFAULT
#   define PYCLINGO_VISIBILITY_PRIVATE
#else
#   ifdef PYCLINGO_WIN
#       ifdef PYCLINGO_BUILD_LIBRARY
#           define PYCLINGO_VISIBILITY_DEFAULT __declspec (dllexport)
#       else
#           define PYCLINGO_VISIBILITY_DEFAULT __declspec (dllimport)
#       endif
#       define PYCLINGO_VISIBILITY_PRIVATE
#   else
#       if __GNUC__ >= 4
#           define PYCLINGO_VISIBILITY_DEFAULT  __attribute__ ((visibility ("default")))
#           define PYCLINGO_VISIBILITY_PRIVATE __attribute__ ((visibility ("hidden")))
#       else
#           define PYCLINGO_VISIBILITY_DEFAULT
#           define PYCLINGO_VISIBILITY_PRIVATE
#       endif
#   endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

PYCLINGO_VISIBILITY_DEFAULT void *clingo_init_python_();
PYCLINGO_VISIBILITY_DEFAULT bool clingo_register_python_();

#ifdef __cplusplus
}
#endif

#endif // PYCLINGO_PYCLINGO_H
