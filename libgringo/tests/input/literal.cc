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

// {{{ declaration of TestLiteral

class TestLiteral : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestLiteral);
        CPPUNIT_TEST(test_print);
        CPPUNIT_TEST(test_clone);
        CPPUNIT_TEST(test_hash);
        CPPUNIT_TEST(test_equal);
        CPPUNIT_TEST(test_unpool);
        CPPUNIT_TEST(test_rewrite);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

    void test_print();
    void test_clone();
    void test_hash();
    void test_equal();
    void test_unpool();
    void test_rewrite();

    virtual ~TestLiteral();
};

// }}}

using namespace Gringo::IO;

// {{{ auxiliary functions and classes

namespace {


ULit rewrite(ULit &&x) {
    Projections project;
    Literal::AssignVec assign;
    SimplifyState state;
    Term::ArithmeticsMap arith;
    x->simplify(project, state);
    x->rewriteArithmetics(arith, assign, state.gen);
    return std::move(x);
}

} // namespace

// }}}
// {{{ definition of TestLiteral

void TestLiteral::setUp() { }

void TestLiteral::tearDown() { }

void TestLiteral::test_print() {
    CPPUNIT_ASSERT_EQUAL(std::string("x"), to_string(pred(NAF::POS, val(ID("x")))));
    CPPUNIT_ASSERT_EQUAL(std::string("not x"), to_string(pred(NAF::NOT, val(ID("x")))));
    CPPUNIT_ASSERT_EQUAL(std::string("not not x"), to_string(pred(NAF::NOTNOT, val(ID("x")))));
    CPPUNIT_ASSERT_EQUAL(std::string("x=y"), to_string(rel(Relation::EQ, val(ID("x")), val(ID("y")))));
    CPPUNIT_ASSERT_EQUAL(std::string("#range(x,y,z)"), to_string(range(val(ID("x")), val(ID("y")), val(ID("z")))));
    CPPUNIT_ASSERT_EQUAL(std::string("1$*$x$+1$*$y$<=23$<=42"), to_string(csplit(cspadd(cspmul(val(NUM(1)), val(ID("x"))), cspmul(val(NUM(1)), val(ID("y")))), Relation::LEQ, cspadd(cspmul(val(ID("23")))), Relation::LEQ, cspadd(cspmul(val(ID("42")))))));
}

void TestLiteral::test_clone() {
    CPPUNIT_ASSERT_EQUAL(std::string("x"), to_string(get_clone(pred(NAF::POS, val(ID("x"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("not x"), to_string(get_clone(pred(NAF::NOT, val(ID("x"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("not not x"), to_string(get_clone(pred(NAF::NOTNOT, val(ID("x"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("x=x"), to_string(get_clone(rel(Relation::EQ, val(ID("x")), val(ID("x"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("#range(x,y,z)"), to_string(get_clone(range(val(ID("x")), val(ID("y")), val(ID("z"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("1$*$x$+1$*$y$<=23$<=42"), to_string(get_clone(csplit(cspadd(cspmul(val(NUM(1)), val(ID("x"))), cspmul(val(NUM(1)), val(ID("y")))), Relation::LEQ, cspadd(cspmul(val(ID("23")))), Relation::LEQ, cspadd(cspmul(val(ID("42"))))))));
}

void TestLiteral::test_hash() {
    char const *msg = "warning: hashes are very unlikely to collide";
    CPPUNIT_ASSERT_MESSAGE(msg, pred(NAF::POS, val(ID("x")))->hash() == pred(NAF::POS, val(ID("x")))->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, !(pred(NAF::POS, val(ID("x")))->hash() == pred(NAF::NOT, val(ID("x")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(pred(NAF::POS, val(ID("x")))->hash() == pred(NAF::POS, val(ID("y")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, !(rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::LT, val(ID("x")), val(ID("x")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::EQ, val(ID("y")), val(ID("x")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(rel(Relation::EQ, val(ID("x")), val(ID("x")))->hash() == rel(Relation::EQ, val(ID("x")), val(ID("y")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, range(val(ID("x")), val(ID("y")), val(ID("z")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, !(range(val(ID("x")), val(ID("x")), val(ID("z")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(range(val(ID("x")), val(ID("y")), val(ID("x")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(range(val(ID("y")), val(ID("y")), val(ID("z")))->hash() == range(val(ID("x")), val(ID("y")), val(ID("z")))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg,  (cspmul(val(NUM(1)), val(ID("x"))).hash() == cspmul(val(NUM(1)), val(ID("x"))).hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(cspmul(val(NUM(1)), val(ID("x"))).hash() == cspmul(val(NUM(2)), val(ID("x"))).hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(cspmul(val(NUM(1)), val(ID("y"))).hash() == cspmul(val(NUM(1)), val(ID("x"))).hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(cspmul(val(NUM(1))).hash() == cspmul(val(NUM(1)), val(ID("x"))).hash()));
    auto mt = [](int i) { return cspmul(val(NUM(i)), var("x")); };
    CPPUNIT_ASSERT_MESSAGE(msg,  (cspadd(mt(1), mt(2), mt(3)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(cspadd(mt(2), mt(2), mt(3)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(cspadd(mt(1), mt(3), mt(3)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(cspadd(mt(1), mt(2), mt(4)).hash() == cspadd(mt(1), mt(2), mt(3)).hash()));
    auto at = [&mt](int i) { return cspadd(mt(i), mt(i+1)); };
    CPPUNIT_ASSERT_MESSAGE(msg,  (csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(csplit(at(2), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(csplit(at(1), Relation::LEQ, at(3), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(4))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(csplit(at(1), Relation::LEQ, at(2), Relation::NEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
    CPPUNIT_ASSERT_MESSAGE(msg, !(csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3))->hash() == csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))->hash()));
}

void TestLiteral::test_equal() {
    CPPUNIT_ASSERT(is_value_equal_to(pred(NAF::POS, val(ID("x"))), pred(NAF::POS, val(ID("x")))));
    CPPUNIT_ASSERT(!is_value_equal_to(pred(NAF::POS, val(ID("x"))), pred(NAF::NOT, val(ID("x")))));
    CPPUNIT_ASSERT(!is_value_equal_to(pred(NAF::POS, val(ID("x"))), pred(NAF::POS, val(ID("y")))));
    CPPUNIT_ASSERT(is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::EQ, val(ID("x")), val(ID("x")))));
    CPPUNIT_ASSERT(!is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::LT, val(ID("x")), val(ID("x")))));
    CPPUNIT_ASSERT(!is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::EQ, val(ID("y")), val(ID("x")))));
    CPPUNIT_ASSERT(!is_value_equal_to(rel(Relation::EQ, val(ID("x")), val(ID("x"))), rel(Relation::EQ, val(ID("x")), val(ID("y")))));
    CPPUNIT_ASSERT(is_value_equal_to(range(val(ID("x")), val(ID("y")), val(ID("z"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
    CPPUNIT_ASSERT(!is_value_equal_to(range(val(ID("x")), val(ID("x")), val(ID("z"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
    CPPUNIT_ASSERT(!is_value_equal_to(range(val(ID("x")), val(ID("y")), val(ID("x"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
    CPPUNIT_ASSERT(!is_value_equal_to(range(val(ID("y")), val(ID("y")), val(ID("z"))), range(val(ID("x")), val(ID("y")), val(ID("z")))));
    CPPUNIT_ASSERT( (cspmul(val(NUM(1)), val(ID("x"))) == cspmul(val(NUM(1)), val(ID("x")))));
    CPPUNIT_ASSERT(!(cspmul(val(NUM(1)), val(ID("x"))) == cspmul(val(NUM(2)), val(ID("x")))));
    CPPUNIT_ASSERT(!(cspmul(val(NUM(1)), val(ID("y"))) == cspmul(val(NUM(1)), val(ID("x")))));
    CPPUNIT_ASSERT(!(cspmul(val(NUM(1))) == cspmul(val(NUM(1)), val(ID("x")))));
    auto mt = [](int i) { return cspmul(val(NUM(i)), var("x")); };
    CPPUNIT_ASSERT( (cspadd(mt(1), mt(2), mt(3)) == cspadd(mt(1), mt(2), mt(3))));
    CPPUNIT_ASSERT(!(cspadd(mt(2), mt(2), mt(3)) == cspadd(mt(1), mt(2), mt(3))));
    CPPUNIT_ASSERT(!(cspadd(mt(1), mt(3), mt(3)) == cspadd(mt(1), mt(2), mt(3))));
    CPPUNIT_ASSERT(!(cspadd(mt(1), mt(2), mt(4)) == cspadd(mt(1), mt(2), mt(3))));
    auto at = [&mt](int i) { return cspadd(mt(i), mt(i+1)); };
    CPPUNIT_ASSERT( (*csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
    CPPUNIT_ASSERT(!(*csplit(at(2), Relation::LEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
    CPPUNIT_ASSERT(!(*csplit(at(1), Relation::LEQ, at(3), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
    CPPUNIT_ASSERT(!(*csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(4)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
    CPPUNIT_ASSERT(!(*csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
    CPPUNIT_ASSERT(!(*csplit(at(1), Relation::LEQ, at(2), Relation::NEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
    CPPUNIT_ASSERT(!(*csplit(at(1), Relation::NEQ, at(2), Relation::LEQ, at(3)) == *csplit(at(1), Relation::LEQ, at(2), Relation::LEQ, at(3))));
}

void TestLiteral::test_unpool() {
    CPPUNIT_ASSERT_EQUAL(std::string("[x,y,z]"), to_string(pred(NAF::POS, pool(val(ID("x")), val(ID("y")), val(ID("z"))))->unpool(true)));
    CPPUNIT_ASSERT_EQUAL(std::string("[not x,not y,not z]"), to_string(pred(NAF::NOT, pool(val(ID("x")), val(ID("y")), val(ID("z"))))->unpool(true)));
    CPPUNIT_ASSERT_EQUAL(std::string("[a!=x,a!=y,b!=x,b!=y]"), to_string(rel(Relation::NEQ, pool(val(ID("a")), val(ID("b"))), pool(val(ID("x")), val(ID("y"))))->unpool(true)));
    CPPUNIT_ASSERT_EQUAL(std::string(
        "[1$*$x$+1$*$y$<=23$<=42"
        ",2$*$x$+1$*$y$<=23$<=42"
        ",1$*$y$+1$*$y$<=23$<=42"
        ",2$*$y$+1$*$y$<=23$<=42"
        ",1$*$x$+1$*$y$<=23$<=43"
        ",2$*$x$+1$*$y$<=23$<=43"
        ",1$*$y$+1$*$y$<=23$<=43"
        ",2$*$y$+1$*$y$<=23$<=43"
        "]"
        ), to_string(
        csplit(
            cspadd(
                cspmul(pool(val(NUM(1)), val(NUM(2))), pool(val(ID("x")), val(ID("y")))),
                cspmul(val(NUM(1)), val(ID("y")))),
            Relation::LEQ,
            cspadd(cspmul(val(ID("23")))),
            Relation::LEQ,
            cspadd(cspmul(pool(val(ID("42")), val(NUM(43))))))->unpool(true)));
}

void TestLiteral::test_rewrite() {
    CPPUNIT_ASSERT_EQUAL(std::string("3"), to_string(rewrite(pred(NAF::POS, binop(BinOp::ADD, val(NUM(1)), val(NUM(2)))))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>7"), to_string(rewrite(rel(Relation::GT, binop(BinOp::ADD, val(NUM(1)), val(NUM(2))), binop(BinOp::ADD, val(NUM(3)), val(NUM(4)))))));
    CPPUNIT_ASSERT_EQUAL(std::string("3$*$5$+4$*$y$<=42"),
        to_string(rewrite(csplit(
            cspadd(
                cspmul(binop(BinOp::ADD, val(NUM(1)), val(NUM(2))), binop(BinOp::ADD, val(NUM(2)), val(NUM(3)))),
                cspmul(binop(BinOp::ADD, val(NUM(1)), val(NUM(3))), val(ID("y")))),
            Relation::LEQ,
            cspadd(cspmul(binop(BinOp::MUL, val(NUM(6)), val(NUM(7)))))))));
}

TestLiteral::~TestLiteral() { }
// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestLiteral);

} } } // namespace Test Input Gringo

