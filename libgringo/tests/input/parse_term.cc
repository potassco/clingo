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
#include "tests/gringo_module.hh"

#include <climits>
#include <sstream>

namespace Gringo { namespace Input { namespace Test {

// {{{ declaration of TestParseTerm

class TestParseTerm : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestParseTerm);
        CPPUNIT_TEST(test_parse);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_parse();

    virtual ~TestParseTerm();
};

// }}}

using namespace Gringo::Test;

// {{{ definition of TestParseTerm

void TestParseTerm::test_parse() {
    TestGringoModule m;
    Messages msg;
    CPPUNIT_ASSERT_THROW(m.parseValue("a."), std::runtime_error);
    CPPUNIT_ASSERT_EQUAL(Symbol(), m.parseValue("x+y"));
    CPPUNIT_ASSERT_EQUAL(NUM(1), m.parseValue("1"));
    CPPUNIT_ASSERT_EQUAL(NUM(-1), m.parseValue("-1"));
    CPPUNIT_ASSERT_EQUAL(ID("x"), m.parseValue("x"));
    CPPUNIT_ASSERT_EQUAL(ID("x", true), m.parseValue("-x"));
    CPPUNIT_ASSERT_EQUAL(FUN("f", {ID("x")}), m.parseValue("f(x)"));
    CPPUNIT_ASSERT_EQUAL(FUN("f", {ID("x")}, true), m.parseValue("-f(x)"));
    CPPUNIT_ASSERT_EQUAL(NUM(39), m.parseValue("7?32"));
    CPPUNIT_ASSERT_EQUAL(NUM(5), m.parseValue("7&13"));
    CPPUNIT_ASSERT_EQUAL(NUM(8), m.parseValue("13-5"));
    CPPUNIT_ASSERT_EQUAL(NUM(18), m.parseValue("13+5"));
    CPPUNIT_ASSERT_EQUAL(NUM(65), m.parseValue("13*5"));
    CPPUNIT_ASSERT_EQUAL(NUM(3), m.parseValue("13\\5"));
    CPPUNIT_ASSERT_EQUAL(NUM(2), m.parseValue("13/5"));
    CPPUNIT_ASSERT_EQUAL(NUM(371293), m.parseValue("13**5"));
    CPPUNIT_ASSERT_EQUAL(NUM(-2), m.parseValue("~1"));
    CPPUNIT_ASSERT_EQUAL(FUN("", {}), m.parseValue("()"));
    CPPUNIT_ASSERT_EQUAL(FUN("", {NUM(1)}), m.parseValue("(1,)"));
    CPPUNIT_ASSERT_EQUAL(FUN("", {NUM(1),NUM(2)}), m.parseValue("(1,2)"));
    CPPUNIT_ASSERT_EQUAL(NUM(2), m.parseValue("|-2|"));
    CPPUNIT_ASSERT_EQUAL(STR("hallo"), m.parseValue("\"hallo\""));
    CPPUNIT_ASSERT_EQUAL(INF(), m.parseValue("#inf"));
    CPPUNIT_ASSERT_EQUAL(SUP(), m.parseValue("#sup"));
}

TestParseTerm::~TestParseTerm() { }
// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestParseTerm);

} } } // namespace Test Input Gringo

