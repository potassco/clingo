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

#ifdef WITH_LUA

#include "tests/tests.hh"
#include "gringo/lua.hh"
#include "gringo_module.hh"

namespace Gringo { namespace Test {

// {{{ declaration of TestLua

class TestLua : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestLua);
        CPPUNIT_TEST(test_parse);
        CPPUNIT_TEST(test_callable);
        CPPUNIT_TEST(test_values);
        CPPUNIT_TEST(test_cmp);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

    void test_values();
    void test_parse();
    void test_cmp();
    void test_callable();

    virtual ~TestLua();
};

// }}}

using namespace Gringo::IO;
using S = std::string;

// {{{ definition of TestLua

void TestLua::setUp() {
}

void TestLua::tearDown() {
}

void TestLua::test_parse() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Lua lua(getTestModule());
    lua.exec(loc,
        "gringo = require(\"gringo\")\n"
        "function get() return gringo.parse_term('1') end\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[1]"), to_string(lua.call(Any(), loc, "get", {})));
    lua.exec(loc,
        "gringo = require(\"gringo\")\n"
        "function get() return gringo.parse_term('p(2+1)') end\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[p(3)]"), to_string(lua.call(Any(), loc, "get", {})));
}

namespace {

std::string removeTrace(std::string &&s) {
    auto needle = "stack traceback:\n";
    auto begin = s.find(needle);
    if (begin != std::string::npos) {
        begin+= strlen(needle);
        auto end = begin;
        while (end < s.size() && s[end] == ' ') {
            auto next = s.find("\n", end);
            if (next == std::string::npos) {
                break;
            }
            else {
                end = next + 1;
            }
        }
        s.replace(begin, end - begin, "  ...\n");
    }
    return std::move(s);
}

} // namespace

void TestLua::test_values() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Lua lua(Gringo::Test::getTestModule());
    lua.exec(loc, 
        "gringo = require(\"gringo\")\n"
        "x = gringo.Fun(\"f\", {2, 3, 4})\n"
        "function getX() return x end\n"
        "function fail() return gringo.Fun(\"g\", {{}}) end\n"
        "function none() return nil end\n"
        "values = {"
        "gringo.Fun(\"f\", {1, 2, 3}),"
        "gringo.Sup,"
        "gringo.Inf,"
        "gringo.Fun(\"id\"),"
        "gringo.Tuple({1, 2, 3}),"
        "123,"
        "\"abc\","
        "gringo.Tuple(x:args()),"
        "x:name(),"
        "}\n"
        "function getValues() return values end\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[f(2,3,4)]"), to_string(lua.call(Any(), loc, "getX", {})));
    CPPUNIT_ASSERT_EQUAL(S("[f(1,2,3),#sup,#inf,id,(1,2,3),123,\"abc\",(2,3,4),\"f\"]"), to_string(lua.call(Any(), loc, "getValues", {})));
    {
        Gringo::Test::Messages msg;
        CPPUNIT_ASSERT_EQUAL(S("[]"), to_string(lua.call(Any(), loc, "none", {})));
    CPPUNIT_ASSERT_EQUAL(S(
            "["
            "dummy:1:1: info: operation undefined:\n"
            "  RuntimeError: cannot convert to value\n"
            "stack traceback:\n"
            "  ...\n"
            "]"), removeTrace(IO::to_string(msg)));
    }
    {
        Gringo::Test::Messages msg;
        CPPUNIT_ASSERT_EQUAL(S("[]"), to_string(lua.call(Any(), loc, "fail", {})));
        CPPUNIT_ASSERT_EQUAL(S(
            "["
            "dummy:1:1: info: operation undefined:\n"
            "  RuntimeError: [string \"dummy:1:1\"]:4: cannot convert to value\n"
            "stack traceback:\n"
            "  ...\n"
            "]"), removeTrace(IO::to_string(msg)));
    }
    {
        Gringo::Test::Messages msg;
        CPPUNIT_ASSERT_THROW(lua.exec(loc, "("), std::runtime_error);
        CPPUNIT_ASSERT_EQUAL(S(
            "["
            "dummy:1:1: error: parsing lua script failed:\n"
            "  SyntaxError: [string \"dummy:1:1\"]:1: unexpected symbol near <eof>\n"
            "]"), replace_all(IO::to_string(msg), "'<eof>'", "<eof>"));
    }
}

void TestLua::test_cmp() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Lua lua(Gringo::Test::getTestModule());
    lua.exec(loc,
        "gringo = require(\"gringo\")\n"
        "function int(x) if x then return 1 else return 0 end end\n"
        "function cmp()\n"
        "  return {"
        "int(gringo.Fun(\"a\") < gringo.Fun(\"b\")),"
        "int(gringo.Fun(\"b\") < gringo.Fun(\"a\")),"
        "gringo.cmp(gringo.Fun(\"a\"), gringo.Fun(\"b\")),"
        "gringo.cmp(gringo.Fun(\"b\"), gringo.Fun(\"a\")),"
        "} end\n"
        );
    CPPUNIT_ASSERT_EQUAL(S("[1,0,-1,1]"), to_string(lua.call(Any(), loc, "cmp", {})));
}

void TestLua::test_callable() {
    Location loc("dummy", 1, 1, "dummy", 1, 1);
    Lua lua(Gringo::Test::getTestModule());
    lua.exec(loc, 
        "function a() end\n"
        "b = 1\n"
        );
    CPPUNIT_ASSERT(lua.callable(Any(), "a"));
    CPPUNIT_ASSERT(!lua.callable(Any(), "b"));
    CPPUNIT_ASSERT(!lua.callable(Any(), "c"));
}

TestLua::~TestLua() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestLua);

} } // namespace Test Gringo

#endif // WITH_LUA

