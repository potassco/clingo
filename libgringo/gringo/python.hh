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

#ifndef _GRINGO_PYTHON_HH
#define _GRINGO_PYTHON_HH

#include <ostream>
#include <gringo/locatable.hh>
#include <gringo/control.hh>

namespace Gringo {

struct PythonImpl;
struct Python {
    Python(GringoModule &module);
    bool exec(Location const &loc, FWString code);
    ValVec call(Location const &loc, FWString name, ValVec const &args);
    bool callable(FWString name);
    void main(Control &ctl);
    static void *initlib(GringoModule &gringo);
    ~Python();

    static std::unique_ptr<PythonImpl> impl;
};

} // namespace Gringo

#endif // _GRINGO_PYTHON_HH
