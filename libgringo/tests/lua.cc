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

using namespace Gringo::IO;
using S = std::string;

} // namespace

TEST_CASE("lua", "[base]") {

    SECTION("parse") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Lua lua(getTestModule());
        lua.exec(loc,
            "clingo = require(\"clingo\")\n"
            "function get() return clingo.parse_term('1') end\n"
            );
        REQUIRE("[1]" == to_string(lua.call(loc, "get", {})));
        lua.exec(loc,
            "clingo = require(\"clingo\")\n"
            "function get() return clingo.parse_term('p(2+1)') end\n"
            );
        REQUIRE("[p(3)]" == to_string(lua.call(loc, "get", {})));
    }

    SECTION("values") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Lua lua(Gringo::Test::getTestModule());
        lua.exec(loc,
            "clingo = require(\"clingo\")\n"
            "x = clingo.fun(\"f\", {2, 3, 4})\n"
            "y = clingo.fun(\"f\", {2, 3, 4}, true)\n"
            "function getX() return x end\n"
            "function fail() return clingo.fun(\"g\", {{}}) end\n"
            "function none() return nil end\n"
            "values = {"
            "clingo.fun(\"f\",{1, 2, 3}),"
            "clingo.Sup,"
            "clingo.Inf,"
            "clingo.fun(\"id\"),"
            "clingo.tuple({clingo.number(1), 2, 3}),"
            "123,"
            "clingo.str(\"abc\"),"
            "\"x\","
            "clingo.tuple(x.args),"
            "x.name,"
            "tostring(x.sign),"
            "tostring(y.sign),"
            "x,"
            "y,"
            "}\n"
            "function getValues() return values end\n"
            );
        REQUIRE("[f(2,3,4)]" == to_string(lua.call(loc, "getX", {})));
        REQUIRE("[f(1,2,3),#sup,#inf,id,(1,2,3),123,\"abc\",\"x\",(2,3,4),\"f\",\"false\",\"true\",f(2,3,4),-f(2,3,4)]" == to_string(lua.call(loc, "getValues", {})));
        {
            Gringo::Test::Messages msg;
            REQUIRE("[]" == to_string(lua.call(loc, "none", {})));
            REQUIRE(
                "["
                "dummy:1:1: info: operation undefined:\n"
                "  RuntimeError: cannot convert to value\n"
                "stack traceback:\n"
                "  ...\n"
                "]" == removeTrace(IO::to_string(msg)));
        }
        {
            Gringo::Test::Messages msg;
            REQUIRE("[]" == to_string(lua.call(loc, "fail", {})));
            REQUIRE(
                "["
                "dummy:1:1: info: operation undefined:\n"
                "  RuntimeError: [string \"dummy:1:1\"]:5: cannot convert to value\n"
                "stack traceback:\n"
                "  ...\n"
                "]" == removeTrace(IO::to_string(msg)));
        }
        {
            Gringo::Test::Messages msg;
            REQUIRE_THROWS_AS(lua.exec(loc, "("), std::runtime_error);
            REQUIRE(
                "["
                "dummy:1:1: error: parsing lua script failed:\n"
                "  SyntaxError: [string \"dummy:1:1\"]:1: unexpected symbol near <eof>\n"
                "]" == replace_all(IO::to_string(msg), "'<eof>'", "<eof>"));
        }
    }

    SECTION("cmp") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Lua lua(Gringo::Test::getTestModule());
        lua.exec(loc,
            "clingo = require(\"clingo\")\n"
            "function int(x) if x then return 1 else return 0 end end\n"
            "function cmp()\n"
            "  return {"
            "int(clingo.fun(\"a\") < clingo.fun(\"b\")),"
            "int(clingo.fun(\"b\") < clingo.fun(\"a\")),"
            "} end\n"
            );
        REQUIRE("[1,0]" == to_string(lua.call(loc, "cmp", {})));
    }

    SECTION("callable") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Lua lua(Gringo::Test::getTestModule());
        lua.exec(loc,
            "function a() end\n"
            "b = 1\n"
            );
        REQUIRE(lua.callable("a"));
        REQUIRE(!lua.callable("b"));
        REQUIRE(!lua.callable("c"));
    }

}

} } // namespace Test Gringo

#endif // WITH_LUA

