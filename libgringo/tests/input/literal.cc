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
#include "tests/input/lit_helper.hh"

#include <climits>
#include <sstream>

namespace Gringo { namespace Input { namespace Test {

using namespace Gringo::IO;

// {{{ auxiliary functions and classes

namespace {

ULit rewrite(ULit &&x) {
    Gringo::Test::TestMessagePrinter log;
    Projections project;
    Literal::AssignVec assign;
    SimplifyState state;
    Term::ArithmeticsMap arith;
    x->simplify(log, project, state);
    x->rewriteArithmetics(arith, assign, state.gen);
    return std::move(x);
}

} // namespace

// }}}

TEST_CASE("input-literal", "[input]") {

    SECTION("print") {
        REQUIRE("x" == to_string(pred(NAF::POS, val(ID("x")))));
        REQUIRE("not x" == to_string(pred(NAF::NOT, val(ID("x")))));
        REQUIRE("not not x" == to_string(pred(NAF::NOTNOT, val(ID("x")))));
        REQUIRE("x=y" == to_string(rel(Relation::EQ, val(ID("x")), val(ID("y")))));
        REQUIRE("#range(x,y,z)" == to_string(range(val(ID("x")), val(ID("y")), val(ID("z")))));
        REQUIRE("1$*$x$+1$*$y$<=23$<=42" == to_string(csplit(cspadd(cspmul(val(NUM(1)), val(ID("x"))), cspmul(val(NUM(1)), val(ID("y")))), Relation::LEQ, cspadd(cspmul(val(ID("23")))), Relation::LEQ, cspadd(cspmul(val(ID("42")))))));
    }

    SECTION("clone") {
        REQUIRE("x" == to_string(get_clone(pred(NAF::POS, val(ID("x"))))));
        REQUIRE("not x" == to_string(get_clone(pred(NAF::NOT, val(ID("x"))))));
        REQUIRE("not not x" == to_string(get_clone(pred(NAF::NOTNOT, val(ID("x"))))));
        REQUIRE("x=x" == to_string(get_clone(rel(Relation::EQ, val(ID("x")), val(ID("x"))))));
        REQUIRE("#range(x,y,z)" == to_string(get_clone(range(val(ID("x")), val(ID("y")), val(ID("z"))))));
        REQUIRE("1$*$x$+1$*$y$<=23$<=42" == to_string(get_clone(csplit(cspadd(cspmul(val(NUM(1)), val(ID("x"))), cspmul(val(NUM(1)), val(ID("y")))), Relation::LEQ, cspadd(cspmul(val(ID("23")))), Relation::LEQ, cspadd(cspmul(val(ID("42"))))))));
    }

    SECTION("hash") {
        CHECK(pred(NAF::POS, val(ID("x")))->hash() == pred(NAF::POS, val(ID("x")))->hash());
        CHECK(!(pred(NAF::POS, val(ID("x")))->hash() == pred(NAF::NOT, val(ID("x")))->hash()));
        CHECK(!(pred(NAF::POS, val(ID("x")))->hash() == pred(NAF::POS, val(ID("y")))->hash()));
        CHECK(rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash());
        CHECK(!(rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::LT, val(ID("x")), val(ID("x")))->hash()));
        CHECK(!(rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::EQ, val(ID("y")), val(ID("x")))->hash()));
        CHECK(!(rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::EQ, val(ID("x")), val(ID("y")))->hash()));
        CHECK(range(val(ID("x")), val(ID("y")), val(ID("z")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash());
        CHECK(!(range(val(ID("x")), val(ID("x")), val(ID("z")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash()));
        CHECK(!(range(val(ID("x")), val(ID("y")), val(ID("x")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash()));
        CHECK(!(range(val(ID("y")), val(ID("y")), val(ID("z")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash()));
        CHECK( (cspmul(val(NUM(1)), val(ID("x"))).hash() == cspmul(val(NUM(1)), val(ID("x"))).hash()));
        CHECK(!(cspmul(val(NUM(1)), val(ID("x"))).hash() == cspmul(val(NUM(2)), val(ID("x"))).hash()));
        CHECK(!(cspmul(val(NUM(1)), val(ID("y"))).hash() == cspmul(val(NUM(1)), val(ID("x"))).hash()));
        CHECK(!(cspmul(val(NUM(1))).hash() == cspmul(val(NUM(1)), val(ID("x"))).hash()));
        auto mt = [](int i) { return cspmul(val(NUM(i)), var("x")); };
        CHECK( (cspadd(mt(1), mt(2), mt(3)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
        CHECK(!(cspadd(mt(2), mt(2), mt(3)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
        CHECK(!(cspadd(mt(1), mt(3), mt(3)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
        CHECK(!(cspadd(mt(1), mt(2), mt(4)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
        auto at = [&mt](int i) { return cspadd(mt(i), mt(i+1)); };
        CHECK( (csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
        CHECK(!(csplit(at(2), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
        CHECK(!(csplit(at(1), Relation::LEQ, at(3), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
        CHECK(!(csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(4))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
        CHECK(!(csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
        CHECK(!(csplit(at(1), Relation::LEQ, at(2), Relation::NEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
        CHECK(!(csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
    }

    SECTION("equal") {
        REQUIRE(is_value_equal_to(pred(NAF::POS, val(ID("x"))), pred(NAF::POS, val(ID("x")))));
        REQUIRE(!is_value_equal_to(pred(NAF::POS, val(ID("x"))), pred(NAF::NOT, val(ID("x")))));
        REQUIRE(!is_value_equal_to(pred(NAF::POS, val(ID("x"))), pred(NAF::POS, val(ID("y")))));
        REQUIRE(is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::EQ, val(ID("x")), val(ID("x")))));
        REQUIRE(!is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::LT, val(ID("x")), val(ID("x")))));
        REQUIRE(!is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::EQ, val(ID("y")), val(ID("x")))));
        REQUIRE(!is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::EQ, val(ID("x")), val(ID("y")))));
        REQUIRE(is_value_equal_to(range(val(ID("x")), val(ID("y")), val(ID("z"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
        REQUIRE(!is_value_equal_to(range(val(ID("x")), val(ID("x")), val(ID("z"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
        REQUIRE(!is_value_equal_to(range(val(ID("x")), val(ID("y")), val(ID("x"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
        REQUIRE(!is_value_equal_to(range(val(ID("y")), val(ID("y")), val(ID("z"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
        REQUIRE( (cspmul(val(NUM(1)), val(ID("x"))) == cspmul(val(NUM(1)), val(ID("x")))));
        REQUIRE(!(cspmul(val(NUM(1)), val(ID("x"))) == cspmul(val(NUM(2)), val(ID("x")))));
        REQUIRE(!(cspmul(val(NUM(1)), val(ID("y"))) == cspmul(val(NUM(1)), val(ID("x")))));
        REQUIRE(!(cspmul(val(NUM(1))) == cspmul(val(NUM(1)), val(ID("x")))));
        auto mt = [](int i) { return cspmul(val(NUM(i)), var("x")); };
        REQUIRE( (cspadd(mt(1), mt(2), mt(3)) == cspadd(mt(1), mt(2), mt(3))));
        REQUIRE(!(cspadd(mt(2), mt(2), mt(3)) == cspadd(mt(1), mt(2), mt(3))));
        REQUIRE(!(cspadd(mt(1), mt(3), mt(3)) == cspadd(mt(1), mt(2), mt(3))));
        REQUIRE(!(cspadd(mt(1), mt(2), mt(4)) == cspadd(mt(1), mt(2), mt(3))));
        auto at = [&mt](int i) { return cspadd(mt(i), mt(i+1)); };
        REQUIRE( (*csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
        REQUIRE(!(*csplit(at(2), Relation::LEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
        REQUIRE(!(*csplit(at(1), Relation::LEQ, at(3), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
        REQUIRE(!(*csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(4)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
        REQUIRE(!(*csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
        REQUIRE(!(*csplit(at(1), Relation::LEQ, at(2), Relation::NEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
        REQUIRE(!(*csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
    }

    SECTION("unpool") {
        REQUIRE("[x,y,z]" == to_string(pred(NAF::POS, pool(val(ID("x")), val(ID("y")), val(ID("z"))))->unpool(true)));
        REQUIRE("[not x,not y,not z]" == to_string(pred(NAF::NOT, pool(val(ID("x")), val(ID("y")), val(ID("z"))))->unpool(true)));
        REQUIRE("[a!=x,a!=y,b!=x,b!=y]" == to_string(rel(Relation::NEQ, pool(val(ID("a")), val(ID("b"))), pool(val(ID("x")), val(ID("y"))))->unpool(true)));
        REQUIRE(
            "[1$*$x$+1$*$y$<=23$<=42"
            ",2$*$x$+1$*$y$<=23$<=42"
            ",1$*$y$+1$*$y$<=23$<=42"
            ",2$*$y$+1$*$y$<=23$<=42"
            ",1$*$x$+1$*$y$<=23$<=43"
            ",2$*$x$+1$*$y$<=23$<=43"
            ",1$*$y$+1$*$y$<=23$<=43"
            ",2$*$y$+1$*$y$<=23$<=43"
            "]" == to_string(csplit(
                cspadd(
                    cspmul(pool(val(NUM(1)), val(NUM(2))), pool(val(ID("x")), val(ID("y")))),
                    cspmul(val(NUM(1)), val(ID("y")))),
                Relation::LEQ,
                cspadd(cspmul(val(ID("23")))),
                Relation::LEQ,
                cspadd(cspmul(pool(val(ID("42")), val(NUM(43))))))->unpool(true)));
    }

    SECTION("rewrite") {
        REQUIRE("3" == to_string(rewrite(pred(NAF::POS, binop(BinOp::ADD, val(NUM(1)), val(NUM(2)))))));
        REQUIRE("3>7" == to_string(rewrite(rel(Relation::GT, binop(BinOp::ADD, val(NUM(1)), val(NUM(2))), binop(BinOp::ADD, val(NUM(3)), val(NUM(4)))))));
        REQUIRE("3$*$5$+4$*$y$<=42" ==
            to_string(rewrite(csplit(
                cspadd(
                    cspmul(binop(BinOp::ADD, val(NUM(1)), val(NUM(2))), binop(BinOp::ADD, val(NUM(2)), val(NUM(3)))),
                    cspmul(binop(BinOp::ADD, val(NUM(1)), val(NUM(3))), val(ID("y")))),
                Relation::LEQ,
                cspadd(cspmul(binop(BinOp::MUL, val(NUM(6)), val(NUM(7)))))))));
    }

}

} } } // namespace Test Input Gringo

