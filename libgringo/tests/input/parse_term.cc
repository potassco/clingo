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
#include "tests/term_helper.hh"

#include <climits>
#include <sstream>

namespace Gringo { namespace Input { namespace Test {

using namespace Gringo::Test;

TEST_CASE("input-parse_term", "[input]") {

    SECTION("parse") {
        TestGringoModule m;
        REQUIRE_THROWS_AS(m.parseValue("a."), std::runtime_error);
        REQUIRE(Symbol() == m.parseValue("x+y"));
        REQUIRE(NUM(1) == m.parseValue("1"));
        REQUIRE(NUM(-1) == m.parseValue("-1"));
        REQUIRE(ID("x") == m.parseValue("x"));
        REQUIRE(ID("x", true) == m.parseValue("-x"));
        REQUIRE(FUN("f", {ID("x")}) == m.parseValue("f(x)"));
        REQUIRE(FUN("f", {ID("x")}, true) == m.parseValue("-f(x)"));
        REQUIRE(NUM(39) == m.parseValue("7?32"));
        REQUIRE(NUM(5) == m.parseValue("7&13"));
        REQUIRE(NUM(8) == m.parseValue("13-5"));
        REQUIRE(NUM(18) == m.parseValue("13+5"));
        REQUIRE(NUM(65) == m.parseValue("13*5"));
        REQUIRE(NUM(3) == m.parseValue("13\\5"));
        REQUIRE(NUM(2) == m.parseValue("13/5"));
        REQUIRE(NUM(371293) == m.parseValue("13**5"));
        REQUIRE(NUM(-2) == m.parseValue("~1"));
        REQUIRE(FUN("", {}) == m.parseValue("()"));
        REQUIRE(FUN("", {NUM(1)}) == m.parseValue("(1,)"));
        REQUIRE(FUN("", {NUM(1),NUM(2)}) == m.parseValue("(1,2)"));
        REQUIRE(NUM(2) == m.parseValue("|-2|"));
        REQUIRE(STR("hallo") == m.parseValue("\"hallo\""));
        REQUIRE(INF() == m.parseValue("#inf"));
        REQUIRE(SUP() == m.parseValue("#sup"));
    }

}

} } } // namespace Test Input Gringo

