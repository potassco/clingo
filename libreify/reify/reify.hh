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

#include <reify/parser.hh>
#include <reify/program.hh>

#ifndef REIFY_REIFY_HH
#define REIFY_REIFY_HH

namespace Reify {

inline void reify(std::string const &file, bool calculateSCCs) {
    Program prg;
    Parser p(prg);
    p.parse(file.empty() ? "-" : file);
    Reifier reifier(std::cout, calculateSCCs);
    prg.printReified(reifier);
}

}

#endif // REIFY_REIFY_HH
