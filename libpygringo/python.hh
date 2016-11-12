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

#ifndef GRINGO_PYTHON_HH
#define GRINGO_PYTHON_HH

#include <gringo/script.h>

using clingo_control_new_t = bool (*) (char const *const *, size_t, clingo_logger_t , void *, unsigned, clingo_control_t **);
extern "C" CLINGO_VISIBILITY_DEFAULT void *clingo_init_python_(clingo_control_new_t new_control);

namespace Gringo {

void registerPython(clingo_control_t *ctl, clingo_control_new_t new_control);

} // namespace Gringo

#endif // GRINGO_PYTHON_HH
