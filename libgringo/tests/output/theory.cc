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
#include "tests/ground/grounder_helper.hh"

namespace Gringo { namespace Output { namespace Test {

// {{{ declaration of TestTheory

class TestTheory : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestTheory);
        CPPUNIT_TEST(test_directive);
        CPPUNIT_TEST(test_head);
        CPPUNIT_TEST(test_body);
        CPPUNIT_TEST(test_term);
        CPPUNIT_TEST(test_guard);
    CPPUNIT_TEST_SUITE_END();
    using S = std::string;

public:
    void test_directive();
    void test_head();
    void test_body();
    void test_term();
    void test_guard();
    virtual ~TestTheory();
};

// }}}

// {{{ definition of TestTheory

void TestTheory::test_directive() {
    std::string theory =
        "#theory t {"
        "    group { };"
        "    &a/0 : group, directive"
        "}.";
    CPPUNIT_ASSERT_EQUAL(
        S(
            "9 1 0 1 a\n"
            "9 5 0 0 0\n"
            "0\n"
        ),
        Gringo::Ground::Test::groundAspif(
            theory +
            "&a { }."));
}

void TestTheory::test_head() {
    std::string theory =
        "#theory t {"
        "    group { };"
        "    &a/0 : group, head"
        "}.";
    CPPUNIT_ASSERT_EQUAL(
        S(
            "9 1 0 1 a\n"
            "9 5 1 0 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ),
        Gringo::Ground::Test::groundAspif(
            theory +
            "&a { }."));
}

void TestTheory::test_body() {
    std::string theory =
        "#theory t {"
        "    group { };"
        "    &a/0 : group, body"
        "}.";
    CPPUNIT_ASSERT_EQUAL(
        S(
            "9 1 0 1 a\n"
            "9 5 1 0 0\n"
            "1 0 0 0 1 1\n"
            "0\n"
        ),
        Gringo::Ground::Test::groundAspif(
            theory +
            ":-&a { }."));
}

void TestTheory::test_guard() {
    std::string theory =
        "#theory t {\n"
        "    group { };\n"
        "    &a/0 : group, {=}, group, directive\n"
        "}.\n";
    CPPUNIT_ASSERT_EQUAL(
        S(
            "9 1 0 1 a\n"
            "9 1 2 1 =\n"
            "9 0 1 1\n"
            "9 6 0 0 0 2 1\n"
            "0\n"
        ),
        Gringo::Ground::Test::groundAspif(
            theory +
            "&a { } = 1."));
}

void TestTheory::test_term() {
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
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 4 0 0 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { : }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 1 1\n"
            "9 4 0 2 1 1 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { 1,1 }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 2 1 -1 0\n"
            "9 4 0 1 1 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { () }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 1 1 4 #sup\n"
            "9 4 0 1 1 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { #sup }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 1 1 4 #inf\n"
            "9 4 0 1 1 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { #inf }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 2 1\n"
            "9 1 1 1 f\n"
            "9 2 3 1 1 2\n"
            "9 4 0 1 3 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { f(1) }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 1 1\n"
            "9 2 2 -1 1 1\n"
            "9 4 0 1 2 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { (1,) }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 1 1\n"
            "9 0 2 2\n"
            "9 2 3 -1 2 1 2\n"
            "9 4 0 1 3 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { (1,2) }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 1 1\n"
            "9 0 2 2\n"
            "9 2 3 -2 2 1 2\n"
            "9 4 0 1 3 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { {1,2} }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 1 1\n"
            "9 0 2 2\n"
            "9 2 3 -3 2 1 2\n"
            "9 4 0 1 3 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"

        ), Gringo::Ground::Test::groundAspif(theory + "&a { [1,2] }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 2 1\n"
            "9 1 1 1 -\n"
            "9 2 3 1 1 2\n"
            "9 4 0 1 3 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { (-1) }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 2 1\n"
            "9 1 1 1 -\n"
            "9 2 3 1 1 2\n"
            "9 4 0 1 3 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { X } :- X=-1."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 3 1\n"
            "9 1 2 1 +\n"
            "9 2 4 2 1 3\n"
            "9 1 1 1 -\n"
            "9 2 5 1 1 4\n"
            "9 4 0 1 5 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { - + 1 }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 2 1\n"
            "9 0 3 2\n"
            "9 1 1 1 ^\n"
            "9 2 4 1 2 2 3\n"
            "9 4 0 1 4 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { 1^2 }."));
    CPPUNIT_ASSERT_EQUAL(S(
            "9 1 0 1 a\n"
            "9 0 2 1\n"
            "9 0 4 2\n"
            "9 0 5 3\n"
            "9 1 3 1 *\n"
            "9 2 6 3 2 4 5\n"
            "9 1 1 1 +\n"
            "9 2 7 1 2 2 6\n"
            "9 4 0 1 7 0\n"
            "9 5 1 0 1 0\n"
            "1 0 1 1 0 0\n"
            "0\n"
        ), Gringo::Ground::Test::groundAspif(theory + "&a { 1+2*3 }."));
}

TestTheory::~TestTheory() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestTheory);

} } } // namespace Test Output Gringo

