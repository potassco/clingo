// {{{ GPL License 

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Benjamin Kaufmann
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

#include <clingo/clingocontrol.hh>

void example1() { 
    std::vector<char const *> args{"clingo", "-e", "brave", nullptr};
    DefaultGringoModule module;
    Gringo::Scripts scripts(module);
    ClingoLib lib(scripts, args.size() - 2, args.data());
    lib.add("base", {}, "a :- not b. b :- not a.");
    lib.ground({{"base", {}}}, nullptr);
    lib.prepareSolve({});
    lib.solve([](Gringo::Model const &m) {
        for (auto &atom : m.atoms(Gringo::Model::SHOWN)) {
            std::cout << atom << " "; 
        }
        std::cout << std::endl;
        return true; 
    });
}

int main() {
    example1();
}

