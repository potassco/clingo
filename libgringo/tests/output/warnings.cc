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
#include "tests/output/solver_helper.hh"

#include "gringo/ground/dependency.hh"
#include "gringo/input/nongroundparser.hh"

namespace Gringo { namespace Output { namespace Test {

// {{{ declaration of TestWarnings

class TestWarnings : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestWarnings);
        CPPUNIT_TEST(test_warnings);
        CPPUNIT_TEST(test_warnings_simplify);
    CPPUNIT_TEST_SUITE_END();
    using S = std::string;

public:
    virtual void setUp();
    virtual void tearDown();

    void test_warnings();
    void test_warnings_simplify();

    virtual ~TestWarnings();
};

// }}}

// {{{ definition of TestWarnings

void TestWarnings::setUp() {
}

void TestWarnings::tearDown() {
}

void TestWarnings::test_warnings_simplify() {
    Gringo::Test::Messages msg;
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("p(a+0).")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:3-6: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("q :- X=a+0.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:8-11: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("q :- p(a+0).")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:8-11: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("p(a+0) : #true.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:3-6: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("q :- p(a+0) : #true.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:8-11: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve(":~ p(a+0). [0]")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:6-9: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve(":~ . [a+0]")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:7-10: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();

    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("{ p(a+0) }.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:5-8: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("{ q : p(a+0) }.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:9-12: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#count { q : p(a+0) }.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:16-19: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#count { p(a+0) : q }.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:12-15: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#count { q : q : p(a+0) }.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:20-23: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();

    CPPUNIT_ASSERT_EQUAL(S("[[q]]"), IO::to_string(solve("q :- #true : p(a+0).")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:16-19: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[q]]"), IO::to_string(solve("q :- 0 { p(a+0) } 0.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:12-15: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[q]]"), IO::to_string(solve("q :- 0 { q : p(a+0) } 0.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:16-19: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[q]]"), IO::to_string(solve("q :- 0 #count { p(a+0) } 0.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:19-22: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[q]]"), IO::to_string(solve("q :- 0 #count { q : p(a+0) } 0.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:23-26: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();

    CPPUNIT_ASSERT_EQUAL(S("[]"), IO::to_string(solve("#true : q(a+0).")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:11-14: info: operation undefined:\n  (a+0)\n]"), IO::to_string(msg));
    msg.clear();
}

void TestWarnings::test_warnings() {
    Gringo::Test::Messages msg;
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("p(1..f).")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:3-7: info: interval undefined:\n  1..f\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[p(1)]]"), IO::to_string(solve("p(1). :-p(1..f).")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:11-15: info: interval undefined:\n  1..f\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[p(bot)]]"), IO::to_string(solve("p(bot).\nq(1/X):-p(X).\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:2:3-6: info: operation undefined:\n  (1/X)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[p(bot)]]"), IO::to_string(solve("p(bot).\n#sum{X:q(X):p(X)}.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:2:6-7: info: operation undefined:\n  (X+0)\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[p(bot)]]"), IO::to_string(solve("p(bot).\nx:-1#sum{X:p(X)}.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:2:10-11: info: tuple ignored:\n  bot\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[p(bot)]]"), IO::to_string(solve("p(bot).\nx:-1#sum{:p(X)}.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:2:4-16: info: empty tuple ignored\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#show bot/0.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:1-13: info: no atoms over signature occur in program:\n  bot/0\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("x:-bot.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:4-7: info: atom does not occur in any rule head:\n  bot\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[p(bot)]]"), IO::to_string(solve("p(bot).\n:~ p(X). [X]\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:2:11-12: info: tuple ignored:\n  bot@0\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[a]]"), IO::to_string(solve("a:-#sum{-1:a;1:a}>=0.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[a]]"), IO::to_string(solve("a:-#sum{1:a;2:a}!=1.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[a]]"), IO::to_string(solve("a:-X=#sum{-1:a;1:a},X>=0,X<=0.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[]"), IO::to_string(solve("#const a=b.\n#const b=a.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:1-12: error: cyclic constant definition:\n  #const a=b.\n-:2:1-12: note: cycle involves definition:\n  #const b=a.\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#const a=a.\n")));
    CPPUNIT_ASSERT_EQUAL(S("[]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[]"), IO::to_string(solve("#const a=1.\n#const a=2.\np(a).\n")));
    CPPUNIT_ASSERT_EQUAL(S("[-:2:1-12: error: redefinition of constant:\n  #const a=2.\n-:1:1-12: note: constant also defined here\n]"), IO::to_string(msg));
    msg.clear();
    std::ofstream("/tmp/wincluded.lp");
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#include \"/tmp/wincluded.lp\".#include \"/tmp/wincluded.lp\".")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:30-59: warning: already included file:\n  /tmp/wincluded.lp\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[x=1,y=-1,z=0]]"), IO::to_string(solve("$x $> 0.\n$y $< 0.\na:-$z $> 0.\n")));
    CPPUNIT_ASSERT_EQUAL(S(
        "[warning: unbounded constraint variable:\n  domain of 'x' is set to [1,1]\n"
        ",warning: unbounded constraint variable:\n  domain of 'y' is set to [-1,-1]\n"
        ",warning: unbounded constraint variable:\n  domain of 'z' is set to [0,0]\n"
        "]"),
        IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#show $y/0.")));
    CPPUNIT_ASSERT_EQUAL(S("[-:1:1-12: info: no constraint variables over signature occur in program:\n  $y/0\n]"), IO::to_string(msg));
    msg.clear();
    CPPUNIT_ASSERT_EQUAL(S("[[]]"), IO::to_string(solve("#show $y.")));
    CPPUNIT_ASSERT_EQUAL(S("[info: constraint variable does not occur in program:\n  $y\n]"), IO::to_string(msg));
    msg.clear();
}

TestWarnings::~TestWarnings() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestWarnings);

} } } // namespace Test Output Gringo


