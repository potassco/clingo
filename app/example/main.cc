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

#include <clingo.hh>
#include <iostream>

using namespace Clingo;

int main(int argc, char const **argv) {
    try {
        Logger logger = [](Clingo::WarningCode, char const *message) {
            std::cerr << message << std::endl;
        };
        Control ctl{{argv+1, size_t(argc-1)}, logger, 20};
        ctl.add("base", {}, "a :- not b. b :- not a.");
        ctl.ground({{"base", {}}});
        for (auto m : ctl.solve_iteratively()) {
            std::cout << "Model:";
            for (auto &atom : m.atoms()) {
                std::cout << " " << atom;
            }
            std::cout << "\n";
        };
    }
    catch (std::exception const &e) {
        std::cerr << "example failed with: " << e.what() << std::endl;
    }
}

