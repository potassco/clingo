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

#include <reify/reify.hh>
#include <reify/util.hh>
#include <sstream>
#include "catch.hpp"

namespace Reify {

namespace {

std::string toLparse(std::string const &s) {
    Program prg;
    Parser p(prg);
    p.parse("<input>", gringo_make_unique<std::istringstream>(s));
    std::ostringstream oss;
    prg.printLparse(oss);
    return oss.str();
}

} // namespace

TEST_CASE("reify") {
    SECTION("normal") {
        std::string s =
            "1 2 2 1 4 3\n"
            "0\n"
            "0\nB+\n0\nB-\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("card()") {
        std::string s =
            "2 2 3 1 2 3 4 5\n"
            "0\n"
            "0\nB+\n0\nB-\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("weight()") {
        std::string s =
            "5 2 3 3 1 3 4 5 1 2 3\n"
            "0\n"
            "0\nB+\n0\nB-\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("disjunctive()") {
        std::string s =
            "8 2 2 3 2 1 4 5\n"
            "0\n"
            "0\nB+\n0\nB-\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("choice()") {
        std::string s =
            "3 2 2 3 2 1 4 5\n"
            "0\n"
            "0\nB+\n0\nB-\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("minimize()") {
        std::string s =
            "6 0 3 1 3 4 5 1 2 3\n"
            "6 0 3 1 5 4 3 1 2 3\n"
            "0\n"
            "0\nB+\n0\nB-\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("symbols()") {
        std::string s =
            "0\n"
            "2 a\n"
            "3 b\n"
            "4 c\n"
            "0\nB+\n0\nB-\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("models()") {
        std::string s =
            "0\n"
            "0\nB+\n0\nB-\n0\n42\n";
        REQUIRE(s == toLparse(s));
    }
    SECTION("compute()") {
        std::string s =
            "0\n"
            "0\nB+\n1\n2\n0\nB-\n3\n4\n0\n1\n";
        REQUIRE(s == toLparse(s));
    }
}

} // namespace Reify
