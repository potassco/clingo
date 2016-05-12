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

#ifdef WITH_PYTHON

#include "tests/tests.hh"
#include "gringo/python.hh"
#include "gringo_module.hh"

namespace Gringo { namespace Test {

// {{{ declaration of TestPython

class TestPython : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestPython);
        CPPUNIT_TEST(test_parse);
        CPPUNIT_TEST(test_callable);
        CPPUNIT_TEST(test_values);
        CPPUNIT_TEST(test_cmp);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

    void test_parse();
    void test_values();
    void test_cmp();
    void test_callable();

    virtual ~TestPython();
};

// }}}

using namespace Gringo::IO;
using S = std::string;

// {{{ definition of TestPython

void TestPython::setUp() {
}

void TestPython::tearDown() {
}

std::string replace(std::string &&x, std::string const &y, std::string const &z) {
    size_t index = 0;
    while (true) {
         index = x.find(y, index);
         if (index == std::string::npos) { break; }
         x.replace(index, y.size(), z);
         index += z.size();
    }
    return std::move(x);
}
void TestPython::test_parse() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Python py(getTestModule());
    py.exec(loc,
        "import gringo\n"
        "def get(): return gringo.parse_term('1')\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[1]"), to_string(py.call(Any(), loc, "get", {})));
    py.exec(loc,
        "import gringo\n"
        "def get(): return gringo.parse_term('p(1+2)')\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[p(3)]"), to_string(py.call(Any(), loc, "get", {})));
}
void TestPython::test_values() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Python py(getTestModule());
    py.exec(loc,
        "import gringo\n"
        "x = gringo.Fun(\"f\", [2, 3, 4])\n"
        "def getX(): return x\n"
        "def fail(): return gringo.Fun(\"g\", [None])\n"
        "def none(): return None\n"
        "values = ["
        "gringo.Fun(\"f\", [1, 2, 3]),"
        "gringo.Sup,"
        "gringo.Inf,"
        "gringo.Fun(\"id\"),"
        "(1, 2, 3),"
        "123,"
        "\"abc\","
        "tuple(x.args()),"
        "x.name(),"
        "]\n"
        "def getValues(): return values\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[f(2,3,4)]"), to_string(py.call(Any(), loc, "getX", {})));
    CPPUNIT_ASSERT_EQUAL(S("[f(1,2,3),#sup,#inf,id,(1,2,3),123,\"abc\",(2,3,4),\"f\"]"), to_string(py.call(Any(), loc, "getValues", {})));
    {
        Gringo::Test::Messages msg;
        CPPUNIT_ASSERT_EQUAL(S("[]"), to_string(py.call(Any(), loc, "none", {})));
        CPPUNIT_ASSERT_EQUAL(S(
            "["
            "dummy:1:1: info: operation undefined:\n"
            "  RuntimeError: cannot convert to value: unexpected NoneType() object\n"
            "]"), IO::to_string(msg));
    }
    {
        Gringo::Test::Messages msg;
        CPPUNIT_ASSERT_EQUAL(S("[]"), to_string(py.call(Any(), loc, "fail", {})));
        CPPUNIT_ASSERT_EQUAL(S(
            "["
            "dummy:1:1: info: operation undefined:\n"
            "  Traceback (most recent call last):\n"
            "    File \"<dummy:1:1>\", line 4, in fail\n"
            "  RuntimeError: cannot convert to value: unexpected NoneType() object\n"
            "]"), IO::to_string(msg));
    }
    {
        Gringo::Test::Messages msg;
        CPPUNIT_ASSERT_THROW(py.exec(loc, "("), std::runtime_error);
        CPPUNIT_ASSERT_EQUAL(S(
            "["
            "dummy:1:1: error: parsing failed:\n"
            "    File \"<dummy:1:1>\", line 1\n"
            "      (\n"
            "      ^\n"
            "  SyntaxError: unexpected EOF while parsing\n"
            "]"), replace(IO::to_string(msg), "column 1", "column 2"));
    }
}

void TestPython::test_cmp() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Python py(getTestModule());
    py.exec(loc, 
        "import gringo\n"
        "def cmp():\n"
        "  return ["
        "int(gringo.Fun(\"a\") < gringo.Fun(\"b\")),"
        "int(gringo.Fun(\"b\") < gringo.Fun(\"a\")),"
        "gringo.cmp(gringo.Fun(\"a\"), gringo.Fun(\"b\")),"
        "gringo.cmp(gringo.Fun(\"b\"), gringo.Fun(\"a\")),"
        "]\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[1,0,-1,1]"), to_string(py.call(Any(), loc, "cmp", {})));
}

void TestPython::test_callable() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Python py(getTestModule());
    py.exec(loc, 
        "import gringo\n"
        "def a(): pass\n"
        "b = 1\n"
        );
    CPPUNIT_ASSERT(py.callable(Any(), "a"));
    CPPUNIT_ASSERT(!py.callable(Any(), "b"));
    CPPUNIT_ASSERT(!py.callable(Any(), "c"));
}

TestPython::~TestPython() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestPython);

} } // namespace Test Gringo

#endif // WITH_PYTHON

