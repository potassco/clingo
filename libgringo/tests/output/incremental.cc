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

#include "tests/term_helper.hh"
#include "tests/tests.hh"
#include "tests/term_helper.hh"
#include "tests/output/solver_helper.hh"

namespace Gringo { namespace Output { namespace Test {

using namespace Gringo::Test;

// {{{ declaration of TestIncremental

class TestIncremental : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestIncremental);
        CPPUNIT_TEST(test_assign);
        CPPUNIT_TEST(test_projection);
        CPPUNIT_TEST(test_projectionBug);
        CPPUNIT_TEST(test_lp);
        CPPUNIT_TEST(test_csp);
    CPPUNIT_TEST_SUITE_END();
    using S = std::string;

public:
    virtual void setUp();
    virtual void tearDown();

    void test_assign();
    void test_projection();
    void test_projectionBug();
    void test_lp();
    void test_csp();
    virtual ~TestIncremental();
};

// }}}

// {{{ definition of auxiliary functions

namespace {

std::string iground(std::string in, int last = 3) {
    std::stringstream ss;
    Output::PlainLparseOutputter plo(ss);
    Output::OutputBase out({}, plo);
    Input::Program prg;
    Defines defs;
    Scripts scripts(Gringo::Test::getTestModule());
    Input::NongroundProgramBuilder pb(scripts, prg, out, defs);
    Input::NonGroundParser parser(pb);
    parser.pushStream("-", gringo_make_unique<std::stringstream>(in));
    Models models;
    parser.parse();
    prg.rewrite(defs);
    prg.check();
    //std::cerr << prg;
    // TODO: think about passing params to toGround already...
    if (!message_printer()->hasError()) {
        {
            Ground::Parameters params;
            params.add("base", {});
            prg.toGround(out.domains).ground(params, scripts, out);
        }
        for (int i=1; i < last; ++i) {
            Ground::Parameters params;
            params.add("step", {NUM(i)});
            prg.toGround(out.domains).ground(params, scripts, out);
        }
        {
            Ground::Parameters params;
            params.add("last", {});
            prg.toGround(out.domains).ground(params, scripts, out);
        }
    }
    return ss.str();
}

} // namespace

// }}}
// {{{ definition of TestIncremental

void TestIncremental::setUp() {
}

void TestIncremental::tearDown() {
}

void TestIncremental::test_assign() {
    Gringo::Test::Messages msg;
    CPPUNIT_ASSERT_EQUAL(
        S(
            "0\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "1 2 0 0\n"
            "0\n"
            "2 p(1,0,0)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "1 3 0 0\n"
            "0\n"
            "3 p(2,0,0)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "0\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"), 
        iground(
            "#program base."
            "#program step(k)."
            "p(k,N,Y) :- N = #sum { Y }, Y = #sum { }."
            "#program last."));
    CPPUNIT_ASSERT_EQUAL(
        S(
            "0\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "1 2 0 0\n"
            "0\n"
            "2 p(1,2,1)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "1 3 0 0\n"
            "0\n"
            "3 p(2,4,2)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "0\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"), 
        iground(
            "#program base."
            "#program step(k)."
            "p(k,N,Y) :- N = #sum { 2*Y }, Y = #sum { k }."
            "#program last."));
}
void TestIncremental::test_projection() {
    CPPUNIT_ASSERT_EQUAL(
        S(
            "1 2 1 0 3\n"
            "1 2 1 0 4\n"
            "1 2 1 0 5\n"
            "3 3 3 4 5 0 0\n"
            "0\n"
            "3 p(1)\n"
            "4 p(2)\n"
            "5 p(3)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "1 6 1 0 2\n"
            "3 1 7 1 0 6\n"
            "0\n"
            "7 q(1)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "1 8 1 0 6\n"
            "3 1 9 1 0 8\n"
            "0\n"
            "9 q(2)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 10 1 0 8\n"
            "0\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"), 
        iground(
            "#program base."
            "{p(1..3)}."
            "#program step(k)."
            "{q(k)} :- p(_)."
            "#program last."));
}

void TestIncremental::test_lp() {
    CPPUNIT_ASSERT_EQUAL(
        S(
            "1 2 0 0\n"
            "0\n"
            "2 base\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 3 0 0\n"
            "0\n"
            "3 step(1)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 4 0 0\n"
            "0\n"
            "4 step(2)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 5 0 0\n"
            "0\n"
            "5 last\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"), 
        iground(
            "#program base."
            "#show base."
            "#program step(k)."
            "#show step(k)."
            "#program last."
            "#show last."));
    CPPUNIT_ASSERT_EQUAL(
        S(
            "1 2 0 0\n"
            "0\n"
            "2 base\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 3 0 0\n"
            "0\n"
            "3 step(1)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 4 0 0\n"
            "0\n"
            "4 step(2)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 5 0 0\n"
            "0\n"
            "5 last\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"), 
        iground(
            "#program base."
            "base."
            "#program step(k)."
            "step(k)."
            "#program last."
            "last."));
}
void TestIncremental::test_csp() {
    CPPUNIT_ASSERT_EQUAL(
        S(
            "1 2 0 0\n"
            "0\n"
            "2 base=1\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 3 0 0\n"
            "3 1 4 1 0 5\n"
            "3 1 5 1 0 6\n"
            "3 1 6 1 0 7\n"
            "3 1 7 0 0\n"
            "3 1 8 1 0 9\n"
            "3 1 9 1 0 10\n"
            "3 1 10 0 0\n"
            "3 1 11 1 0 12\n"
            "3 1 12 0 0\n"
            "1 13 1 0 4\n"
            "1 14 2 1 4 5\n"
            "1 15 2 1 5 6\n"
            "1 16 2 1 6 7\n"
            "1 17 1 1 7\n"
            "1 18 1 0 8\n"
            "1 19 2 1 8 9\n"
            "1 20 2 1 9 10\n"
            "1 21 1 1 10\n"
            "1 22 1 0 11\n"
            "1 23 2 1 11 12\n"
            "1 24 1 1 12\n"
            "0\n"
            "13 up=0\n"
            "14 up=1\n"
            "15 up=2\n"
            "16 up=3\n"
            "17 up=4\n"
            "18 lo=1\n"
            "19 lo=2\n"
            "20 lo=3\n"
            "21 lo=4\n"
            "22 ib=1\n"
            "23 ib=2\n"
            "24 ib=3\n"
            "3 step(1)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 25 0 0\n"
            "1 1 1 1 7\n"
            "1 1 1 0 8\n"
            "1 1 2 1 11 12\n"
            "0\n"
            "25 step(2)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 26 0 0\n"
            "0\n"
            "26 last=1\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"), 
        iground(
            "#program base."
            "$base$=1."
            "#program step(k)."
            "step(k)."
            "0 $<= $up $<= 5-k."
            "k $<= $lo $<= 4."
            "1 $<= $ib $<= 3."
            "$ib $= 1; $ib $=3 :- k == 2."
            "#program last."
            "$last$=1."));
}

void TestIncremental::test_projectionBug() {
    CPPUNIT_ASSERT_EQUAL(
        S(
            "1 2 1 0 3\n"
            "3 1 3 0 0\n"
            "0\n"
            "3 p(0,0)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            "1 4 1 0 2\n"
            "1 5 1 0 6\n"
            "3 1 6 1 0 4\n"
            "0\n"
            "6 p(1,1)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 7 1 0 4\n"
            "1 8 1 0 5\n"
            "1 9 1 0 10\n"
            "3 1 10 1 0 8\n"
            "0\n"
            "10 p(2,2)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n"
            "1 11 1 0 7\n"
            "1 12 1 0 8\n"
            "1 13 1 0 9\n"
            "1 11 1 0 14\n"
            "3 1 14 0 0\n"
            "3 1 15 1 0 11\n"
            "3 1 16 1 0 12\n"
            "3 1 17 1 0 13\n"
            "0\n"
            "14 p(1,0)\n"
            "15 r(0)\n"
            "16 r(1)\n"
            "17 r(2)\n"
            "0\nB+\n0\nB-\n1\n0\n1\n" 
            ), 
        iground(
            "#program base."
            "{p(0,0)}."
            "#program step(k)."
            "{p(k,k)} :- p(_,k-1)."
            "#program last."
            "{p(1,0)}."
            "{r(X)} :- p(_,X)."
            ));
}

TestIncremental::~TestIncremental() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestIncremental);

} } } // namespace Test Output Gringo
