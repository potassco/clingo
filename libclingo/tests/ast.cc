// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#include "tests.hh"
#include <iostream>
#include <fstream>

namespace Clingo { namespace Test {

namespace {

struct CB {
    CB(std::string &ret)
    : ret(ret) { ret.clear(); }
    void operator()(AST::Statement &&x) {
        std::ostringstream oss;
        oss << x;
        if (first) {
            REQUIRE(oss.str() == "#program base.");
            first = false;
        }
        else {
            if (!ret.empty()) { ret+= "\n"; }
            ret+= oss.str();
        }
    }
    bool first = true;
    std::string &ret;
};

std::string parse(char const *prg) {
    std::string ret;
    parse_program(prg, CB(ret));
    return ret;
}

}

TEST_CASE("ast", "[clingo]") {
    SECTION("statement") {
        REQUIRE(parse("a:-b.") == "a :- b.");
        REQUIRE(parse("#const a=10.") == "#const a = 10. [default]");
        REQUIRE(parse("#show a/1.") == "#show a/1.");
        REQUIRE(parse("#show $a/1.") == "#show $a/1.");
        REQUIRE(parse("#show a : b.") == "#show a : b.");
        REQUIRE(parse("#show $a : b.") == "#show $a : b.");
        REQUIRE(parse("#show $a : b.") == "#show $a : b.");
        REQUIRE(parse("#minimize{ 1:b }.") == ":~ b. [1@0]");
        REQUIRE(parse("#script (python) 42 #end.") == "#script (python) 42 #end.");
        REQUIRE(parse("#program p(k).") == "#program p(k).");
        REQUIRE(parse("#external p(k).") == "#external p(k).");
        REQUIRE(parse("#external p(k) : a, b.") == "#external p(k) : a; b.");
        REQUIRE(parse("#edge (u,v) : a, b.") == "#edge (u,v) : a; b.");
        REQUIRE(parse("#heuristic a : b, c. [L@P,level]") == "#heuristic a : b; c. [L@P,level]");
        REQUIRE(parse("#project a : b.") == "#project a : b.");
        REQUIRE(parse("#project a/2.") == "#project a/2.");
        REQUIRE(parse("#theory x {}.") == "#theory x {\n}.");
    }
    SECTION("theory definition") {
        REQUIRE(parse("#theory x { t { ++ : 1, unary } }.") == "#theory x {\n" "  t {\n" "    ++ : 1, unary\n" "  }\n" "}.");
        REQUIRE(parse("#theory x { &a/0 : t, any }.") == "#theory x {\n  &a/0 : t, any\n}.");
        REQUIRE(parse("#theory x { &a/0 : t, {+, -}, u, any }.") == "#theory x {\n  &a/0 : t, { +, - }, u, any\n}.");
    }
    SECTION("body literal") {
        // TODO: ...
    }
    SECTION("head literal") {
        // TODO: ...
    }
    SECTION("aggregates") {
        // TODO: ...
    }
    SECTION("literal") {
        // TODO: ...
    }
    SECTION("terms") {
        // TODO: ...
    }
}

} } // namespace Test Clingo
