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

TEST_CASE("parse-ast", "[clingo]") {
    SECTION("statement") {
        REQUIRE(parse("a.") == "a.");
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
        REQUIRE(parse(":-a.") == "#false :- a.");
        REQUIRE(parse(":-not a.") == "#false :- not a.");
        REQUIRE(parse(":-not not a.") == "#false :- not not a.");
        REQUIRE(parse(":-a:b.") == "#false :- a : b.");
        REQUIRE(parse(":-a:b,c;d.") == "#false :- a : b, c; d.");
        REQUIRE(parse(":-1{a:b,c;e}2.") == "#false :- 1 <= { a : b, c; e :  } <= 2.");
        REQUIRE(parse(":-{a:b,c;e}2.") == "#false :- 2 >= { a : b, c; e :  }.");
        REQUIRE(parse(":-1#min{1,2:b,c;1:e}2.") == "#false :- 1 <= #min { 1,2 : b, c; 1 : e } <= 2.");
        REQUIRE(parse(":-&p { 1 : a,b; 2 : c }.") == "#false :- &p { 1 : a,b; 2 : c }.");
        REQUIRE(parse(":-#disjoint {1,2:$x:a,b}.") == "#false :- #disjoint { 1,2 : 1$*x : a,b }.");
    }
    SECTION("head literal") {
        REQUIRE(parse("a.") == "a.");
        REQUIRE(parse("a:b.") == "a : b.");
        REQUIRE(parse("a:b,c;d.") == "d : ; a : b, c.");
        REQUIRE(parse("1{a:b,c;e}2.") == "1 <= { a : b, c; e :  } <= 2.");
        REQUIRE(parse("{a:b,c;e}2.") == "2 >= { a : b, c; e :  }.");
        REQUIRE(parse("1#min{1,2:h:b,c;1:e}2.") == "1 <= #min { 1,2 : h : b, c; 1 : e :  } <= 2.");
        REQUIRE(parse("&p { 1 : a,b; 2 : c }.") == "&p { 1 : a,b; 2 : c }.");
        REQUIRE(parse("&p { 1 : a,b; 2 : c } ** 33.") == "&p { 1 : a,b; 2 : c } ** 33.");
    }
    SECTION("literal") {
        REQUIRE(parse("#true.") == "#true.");
        REQUIRE(parse("#false.") == "#false.");
        REQUIRE(parse("a.") == "a.");
        REQUIRE(parse("not a.") == "not a.");
        REQUIRE(parse("not not a.") == "not not a.");
        REQUIRE(parse("1 != 3.") == "1!=3.");
        REQUIRE(parse("1 $< 2 $< 3.") == "1$<2$<3.");
        REQUIRE(parse("1 $< 2 $< 3.") == "1$<2$<3.");
    }
    SECTION("terms") {
        REQUIRE(parse("p(a).") == "p(a).");
        REQUIRE(parse("p(X).") == "p(X).");
        REQUIRE(parse("p(-a).") == "p(-a).");
        REQUIRE(parse("p(~a).") == "p(~a).");
        REQUIRE(parse("p(|a|).") == "p(|a|).");
        REQUIRE(parse("p((a+b)).") == "p((a+b)).");
        REQUIRE(parse("p((a-b)).") == "p((a-b)).");
        REQUIRE(parse("p((a*b)).") == "p((a*b)).");
        REQUIRE(parse("p((a/b)).") == "p((a/b)).");
        REQUIRE(parse("p((a\\b)).") == "p((a\\b)).");
        REQUIRE(parse("p((a?b)).") == "p((a?b)).");
        REQUIRE(parse("p((a^b)).") == "p((a^b)).");
        REQUIRE(parse("p(a..b).") == "p((a..b)).");
        REQUIRE(parse("p((),(1,),f(),f(1,2)).") == "p((),(1,),f,f(1,2)).");
        REQUIRE(parse("p(a;b).") == "(p(a);p(b)).");
        REQUIRE(parse("p((a,;b)).") == "p(((a,);b)).");
        REQUIRE(parse("1 $+ 3 $* $x $+ 7 $< 2 $< 3.") == "1$+3$*x$+7$<2$<3.");
    }
    SECTION("theory terms") {
        REQUIRE(parse("&p{ } !! a.") == "&p {  } !! a.");
        REQUIRE(parse("&p{ } !! X.") == "&p {  } !! X.");
        REQUIRE(parse("&p{ } !! [].") == "&p {  } !! [].");
        REQUIRE(parse("&p{ } !! [1].") == "&p {  } !! [1].");
        REQUIRE(parse("&p{ } !! [1,2].") == "&p {  } !! [1,2].");
        REQUIRE(parse("&p{ } !! ().") == "&p {  } !! ().");
        REQUIRE(parse("&p{ } !! (a).") == "&p {  } !! a.");
        REQUIRE(parse("&p{ } !! (a,).") == "&p {  } !! (a,).");
        REQUIRE(parse("&p{ } !! {}.") == "&p {  } !! {}.");
        REQUIRE(parse("&p{ } !! f().") == "&p {  } !! f.");
        REQUIRE(parse("&p{ } !! f(a,1).") == "&p {  } !! f(a,1).");
        REQUIRE(parse("&p{ } !! 1 + (x + y * z).") == "&p {  } !! (1 + (x + y * z)).");
    }
}

} } // namespace Test Clingo
