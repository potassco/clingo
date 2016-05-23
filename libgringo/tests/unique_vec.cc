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

#include "tests/tests.hh"
#include "gringo/hash_set.hh"

namespace Gringo { namespace Test {

TEST_CASE("unique_vec", "[base]") {
    SECTION("push") {
        UniqueVec<unsigned> vec;
        REQUIRE( vec.push(1).second);
        REQUIRE( vec.push(2).second);
        REQUIRE( vec.push(3).second);
        REQUIRE( vec.push(4).second);
        vec.pop();
        vec.pop();
        REQUIRE(!vec.push(1).second);
        REQUIRE(!vec.push(2).second);
        REQUIRE( vec.push(3).second);
        REQUIRE( vec.push(4).second);
        REQUIRE( vec.push(5).second);
        REQUIRE( vec.push(6).second);
        REQUIRE( vec.push(7).second);
        vec.erase([](unsigned val) { return 2 < val && val < 6; });
        REQUIRE(!vec.push(1).second);
        REQUIRE(!vec.push(2).second);
        REQUIRE( vec.push(3).second);
        REQUIRE( vec.push(4).second);
        REQUIRE( vec.push(5).second);
        REQUIRE(!vec.push(6).second);
        REQUIRE(!vec.push(7).second);
    }
}

} } // namespace Test Gringo


