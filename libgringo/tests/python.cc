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

namespace Gringo { namespace Test {

using namespace Gringo::IO;
using S = std::string;

namespace {

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

} // namspace

TEST_CASE("python", "[base][python]") {
    TestGringoModule module;
    SECTION("parse") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Python py(module);
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('1')\n"
            );
        REQUIRE("[1]" == to_string(py.call(loc, "get", {}, module)));
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('p(1+2)')\n"
            );
        REQUIRE("[p(3)]" == to_string(py.call(loc, "get", {}, module)));
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('-p')\n"
            );
        REQUIRE("[-p]" == to_string(py.call(loc, "get", {}, module)));
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('-p(1)')\n"
            );
        REQUIRE("[-p(1)]" == to_string(py.call(loc, "get", {}, module)));
    }
    SECTION("values") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Python py(module);
        py.exec(loc,
            "import clingo\n"
            "x = clingo.Function(\"f\", [2, 3, 4])\n"
            "y = clingo.Function(\"f\", [2, 3, 4], True)\n"
            "def getX(): return x\n"
            "def fail(): return clingo.Function(\"g\", [None])\n"
            "def none(): return None\n"
            "values = ["
            "clingo.Function(\"f\", [1, 2, 3]),"
            "clingo.Sup,"
            "clingo.Inf,"
            "clingo.Function(\"id\"),"
            "(1, 2, 3),"
            "123,"
            "\"abc\","
            "tuple(x.args),"
            "x.name,"
            "x.sign,"
            "y.sign,"
            "x,"
            "y,"
            "]\n"
            "def getValues(): return values\n"
            );
        REQUIRE("[f(2,3,4)]" == to_string(py.call(loc, "getX", {}, module)));
        REQUIRE("[f(1,2,3),#sup,#inf,id,(1,2,3),123,\"abc\",(2,3,4),\"f\",0,1,f(2,3,4),-f(2,3,4)]" == to_string(py.call(loc, "getValues", {}, module)));
        {
            REQUIRE("[]" == to_string(py.call(loc, "none", {}, module)));
            REQUIRE(
                "["
                "dummy:1:1: info: operation undefined:\n"
                "  RuntimeError: cannot convert to value: unexpected NoneType() object\n"
                "]" == IO::to_string(module));
        }
        {
            module.reset();
            REQUIRE("[]" == to_string(py.call(loc, "fail", {}, module)));
            REQUIRE(
                "["
                "dummy:1:1: info: operation undefined:\n"
                "  Traceback (most recent call last):\n"
                "    File \"<dummy:1:1>\", line 5, in fail\n"
                "  RuntimeError: cannot convert to value: unexpected NoneType() object\n"
                "]" == IO::to_string(module));
        }
        {
            module.reset();
            try {
                py.exec(loc, "(");
                FAIL("no exception");
            }
            catch (std::runtime_error const &e) {
                REQUIRE(
                    "dummy:1:1: error: parsing failed:\n"
                    "    File \"<dummy:1:1>\", line 1\n"
                    "      (\n"
                    "      ^\n"
                    "  SyntaxError: unexpected EOF while parsing\n"
                    "" == replace(e.what(), "column 1", "column 2"));
            }
        }
    }

    SECTION("cmp") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Python py(module);
        py.exec(loc,
            "import clingo\n"
            "def cmp():\n"
            "  return ["
            "int(clingo.Function(\"a\") < clingo.Function(\"b\")),"
            "int(clingo.Function(\"b\") < clingo.Function(\"a\")),"
            "]\n"
            );
        REQUIRE("[1,0]" == to_string(py.call(loc, "cmp", {}, module)));
    }

    SECTION("callable") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Python py(module);
        py.exec(loc,
            "import clingo\n"
            "def a(): pass\n"
            "b = 1\n"
            );
        REQUIRE(py.callable("a"));
        REQUIRE(!py.callable("b"));
        REQUIRE(!py.callable("c"));
    }

}

} } // namespace Test Gringo

#endif // WITH_PYTHON

