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
#include "tests/ground/grounder_helper.hh"

namespace Gringo { namespace Ground { namespace Test {

TEST_CASE("ground-theory") {
    Gringo::Test::Messages msg;

    SECTION("directive") {
        std::string theory =
            "#theory t {"
            "    group { };"
            "    &a/0 : group, directive"
            "}.";
        REQUIRE(
            "&a{}.\n" ==
            Gringo::Ground::Test::groundText(
                theory +
                "&a { }."));
        REQUIRE(
            "&a{1; 2; 3; f(1); f(2); f(3)}.\n"
            "p(1).\n"
            "p(2).\n"
            "p(3).\n" == Gringo::Ground::Test::groundText(theory +
                "p(1..3)."
                "&a { X : p(X); f(X) : p(X) }."
            ));
        REQUIRE(
            "&a{1: p(1); 2: p(2); 3: p(3); f(1): p(1); f(2): p(2); f(3): p(3)}.\n"
            "{p(1)}.\n"
            "{p(2)}.\n"
            "{p(3)}.\n" == Gringo::Ground::Test::groundText(theory +
                "{p(1..3)}."
                "&a { X : p(X); f(X) : p(X) }."
            ));
    }

    SECTION("head") {
        std::string theory =
            "#theory t {"
            "    group { };"
            "    &a/0 : group, head"
            "}.";
        REQUIRE(
            "&a{}.\n" ==
            Gringo::Ground::Test::groundText(
                theory +
                "&a { }."));
        REQUIRE(
            "&a{1; 2; 3; f(1); f(2); f(3)}.\n"
            "p(1).\n"
            "p(2).\n"
            "p(3).\n" == Gringo::Ground::Test::groundText(theory +
                "p(1..3)."
                "&a { X : p(X); f(X) : p(X) }."
            ));
        REQUIRE(
            "&a{1: p(1); 2: p(2); 3: p(3); f(1): p(1); f(2): p(2); f(3): p(3)}.\n"
            "{p(1)}.\n"
            "{p(2)}.\n"
            "{p(3)}.\n" == Gringo::Ground::Test::groundText(theory +
                "{p(1..3)}."
                "&a { X : p(X); f(X) : p(X) }."
            ));
        REQUIRE(
            "&a{1: p(1); 2: p(2); 3: p(3)}:-p(1).\n"
            "&a{1: p(1); 2: p(2); 3: p(3)}:-p(2).\n"
            "&a{1: p(1); 2: p(2); 3: p(3)}:-p(3).\n"
            "p(2):-p(1).\n"
            "p(3):-p(2).\n"
            "{p(1)}.\n" == Gringo::Ground::Test::groundText(theory +
                "{p(1)}."
                "p(X+1) :- p(X), X < 3."
                "&a { Y : p(Y) } :- p(X)."
            ));
    }

    SECTION("body") {
        std::string theory =
            "#theory t {"
            "    group { };"
            "    &a/0 : group, body"
            "}.";
        REQUIRE(
            ":-&a{}.\n" == Gringo::Ground::Test::groundText(
                theory +
                ":-&a { }."));
        REQUIRE(
            ":-&a{1; 2; 3; f(1); f(2); f(3)}.\n"
            "p(1).\n"
            "p(2).\n"
            "p(3).\n" == Gringo::Ground::Test::groundText(theory +
                "p(1..3)."
                ":-&a { X : p(X); f(X) : p(X) }."
            ));
        REQUIRE(
            ":-&a{1: p(1); 2: p(2); 3: p(3); f(1): p(1); f(2): p(2); f(3): p(3)}.\n"
            "{p(1)}.\n"
            "{p(2)}.\n"
            "{p(3)}.\n" == Gringo::Ground::Test::groundText(theory +
                "{p(1..3)}."
                ":-&a { X : p(X); f(X) : p(X) }."
            ));
        REQUIRE(
            ":-not &a{1: p(1); 2: p(2); 3: p(3)}.\n"
            "{p(1)}.\n"
            "{p(2)}.\n"
            "{p(3)}.\n" == Gringo::Ground::Test::groundText(theory +
                "{p(1..3)}."
                ":-not &a { X : p(X) }."
            ));
        REQUIRE(
            ":-not not &a{1: p(1); 2: p(2); 3: p(3)}.\n"
            "{p(1)}.\n"
            "{p(2)}.\n"
            "{p(3)}.\n" == Gringo::Ground::Test::groundText(theory +
                "{p(1..3)}."
                ":-not not &a { X : p(X) }."
            ));
        REQUIRE(
            "p(1).\n"
            "p(2):-&a{1; 2: p(2); 3: p(3)}.\n"
            "p(3):-&a{1; 2: p(2); 3: p(3)},p(2).\n" == Gringo::Ground::Test::groundText(theory +
                "p(1)."
                "p(X+1) :- p(X), X < 3, &a { Y : p(Y) }."
            ));
        REQUIRE(
            "p(1).\n"
            "p(2):-not &a{1; 2: p(2); 3: p(3)}.\n"
            "p(3):-not &a{1; 2: p(2); 3: p(3)},p(2).\n" == Gringo::Ground::Test::groundText(theory +
                "p(1)."
                "p(X+1) :- p(X), X < 3, not &a { Y : p(Y) }."
            ));
        REQUIRE(
            "p(1).\n"
            "p(2):-not not &a{1; 2: p(2); 3: p(3)}.\n"
            "p(3):-not not &a{1; 2: p(2); 3: p(3)},p(2).\n" == Gringo::Ground::Test::groundText(theory +
                "p(1)."
                "p(X+1) :- p(X), X < 3, not not &a { Y : p(Y) }."
            ));
        REQUIRE(
            "&sum{height: a; height}<=(1).\n"
            "&sum{height: a; height}<=(2).\n"
            "&sum{height: a; height}<=(3).\n"
            "dom(1,1).\n"
            "dom(1,2).\n"
            "dom(1,3).\n"
            "dom(2,1).\n"
            "dom(2,2).\n"
            "dom(2,3).\n"
            "{a;b}.\n" == Gringo::Ground::Test::groundText(
                "#theory t {\n"
                "  t { };\n"
                "  &sum/0 : t, {<=}, t, head\n"
                "}.\n"
                "dom(1..2,V) :- V=1..3.\n"
                "{ a; b }.\n"
                "&sum{height : a; height; height} <= V :- dom(I,V).\n"
            ));
    }

    SECTION("guard") {
        std::string theory =
            "#theory t {\n"
            "    group { };\n"
            "    &a/0 : group, {=,>=}, group, head\n"
            "}.\n";
        REQUIRE(
            "&a{}=(1).\n"
            "&a{}=(2).\n"
            "&a{}=(3).\n" == Gringo::Ground::Test::groundText(
                theory +
                "&a { } = X :- X=1..3."));
        REQUIRE(
            "&a{}>=(1).\n"
            "&a{}>=(2).\n"
            "&a{}>=(3).\n" == Gringo::Ground::Test::groundText(
                theory +
                "&a { } >= X :- X=1..3."));
    }

    SECTION("term") {
        std::string theory =
            "#theory t {\n"
            "  group {\n"
            "    + : 4, unary;\n"
            "    - : 4, unary;\n"
            "    ^ : 3, binary, right;\n"
            "    * : 2, binary, left;\n"
            "    + : 1, binary, left;\n"
            "    - : 1, binary, left\n"
            "  };\n"
            "  &a/0 : group, head\n"
            "}.\n";
        REQUIRE(
            "&a{: }.\n" == Gringo::Ground::Test::groundText(
                theory +
                "&a { : }."));
        REQUIRE(
            "&a{#inf}.\n"
            "&a{#sup}.\n"
            "&a{(-2)}.\n"
            "&a{(-c)}.\n"
            "&a{(-f(c))}.\n"
            "&a{(1,2)}.\n"
            "&a{1}.\n"
            "&a{c}.\n"
            "&a{f(c)}.\n" ==
            Gringo::Ground::Test::groundText(
                theory +
                "&a { X } :- X=(1;c;f(c);-2;-f(c);-c;#sup;#inf;(1,2))."));
        REQUIRE("&a{()}.\n" == Gringo::Ground::Test::groundText(theory + "&a { () }."));
        REQUIRE("&a{(1,)}.\n" == Gringo::Ground::Test::groundText(theory + "&a { (1,) }."));
        REQUIRE("&a{(1,2)}.\n" == Gringo::Ground::Test::groundText(theory + "&a { (1,2) }."));
        REQUIRE("&a{[1,2]}.\n" == Gringo::Ground::Test::groundText(theory + "&a { [1,2] }."));
        REQUIRE("&a{{1,2}}.\n" == Gringo::Ground::Test::groundText(theory + "&a { {1,2} }."));
        REQUIRE("&a{(-1)}.\n" == Gringo::Ground::Test::groundText(theory + "&a { (-1) }."));
        REQUIRE("&a{(-(+1))}.\n" == Gringo::Ground::Test::groundText(theory + "&a { - + 1 }."));
        REQUIRE("&a{(1^(2^3))}.\n" == Gringo::Ground::Test::groundText(theory + "&a { 1^2^3 }."));
        REQUIRE("&a{((1*2)*3)}.\n" == Gringo::Ground::Test::groundText(theory + "&a { 1*2*3 }."));
        REQUIRE("&a{(1+(2*(3^4)))}.\n" == Gringo::Ground::Test::groundText(theory + "&a { 1+2*3^4 }."));
        REQUIRE("&a{(1+((-2)*((-3)^4)))}.\n" == Gringo::Ground::Test::groundText(theory + "&a { 1 + -2 * -3^4 }."));
    }
}

} } } // namespace Test Ground Gringo

