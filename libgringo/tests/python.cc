// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

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
    SymSpan const empty{nullptr, 0};
    SECTION("parse") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Python py(module);
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('1')\n"
            );
        REQUIRE("[1]" == to_string(py.call(loc, "get", empty, module)));
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('p(1+2)')\n"
            );
        REQUIRE("[p(3)]" == to_string(py.call(loc, "get", empty, module)));
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('-p')\n"
            );
        REQUIRE("[-p]" == to_string(py.call(loc, "get", empty, module)));
        py.exec(loc,
            "import clingo\n"
            "def get(): return clingo.parse_term('-p(1)')\n"
            );
        REQUIRE("[-p(1)]" == to_string(py.call(loc, "get", empty, module)));
    }
    SECTION("values") {
        Location loc("dummy", 1, 1, "dummy", 1, 1);
        Python py(module);
        py.exec(loc,
            "import clingo\n"
            "x = clingo.Function(\"f\", [2, 3, 4])\n"
            "y = clingo.Function(\"f\", [2, 3, 4], False)\n"
            "def getX(): return x\n"
            "def fail(): return clingo.Function(\"g\", [None])\n"
            "def none(): return None\n"
            "values = ["
            "clingo.Function(\"f\", [1, 2, 3]),"
            "clingo.Supremum,"
            "clingo.Infimum,"
            "clingo.Function(\"id\"),"
            "(1, 2, 3),"
            "123,"
            "\"abc\","
            "tuple(x.arguments),"
            "x.name,"
            "x.negative,"
            "y.negative,"
            "x,"
            "y,"
            "]\n"
            "def getValues(): return values\n"
            );
        REQUIRE("[f(2,3,4)]" == to_string(py.call(loc, "getX", empty, module)));
        REQUIRE("[f(1,2,3),#sup,#inf,id,(1,2,3),123,\"abc\",(2,3,4),\"f\",0,1,f(2,3,4),-f(2,3,4)]" == to_string(py.call(loc, "getValues", empty, module)));
        {
            REQUIRE("[]" == to_string(py.call(loc, "none", empty, module)));
            REQUIRE(
                "["
                "dummy:1:1: info: operation undefined:\n"
                "  RuntimeError: cannot convert to value: unexpected NoneType() object\n"
                "]" == IO::to_string(module));
        }
        {
            module.reset();
            REQUIRE("[]" == to_string(py.call(loc, "fail", empty, module)));
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
        REQUIRE("[1,0]" == to_string(py.call(loc, "cmp", empty, module)));
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

