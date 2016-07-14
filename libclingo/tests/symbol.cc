// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#include "tests.hh"
#include <iostream>
#include <fstream>

namespace Clingo { namespace Test {

TEST_CASE("symbol", "[clingo]") {
    SECTION("signature") {
        Signature a("a", 2, false);
        Signature b("a", 2, false);
        Signature c("a", 2, true);
        REQUIRE(a.name() == S("a"));
        REQUIRE(a.arity() == 2);
        REQUIRE(a.negative());
        REQUIRE(!a.positive());
        REQUIRE(b == a);
        REQUIRE(c != a);
        REQUIRE(c < a);
        REQUIRE(c <= a);
        REQUIRE(a <= b);
        REQUIRE_FALSE(a <= c);
        REQUIRE(a > c);
        REQUIRE(a >= c);
        REQUIRE(a >= b);
        REQUIRE_FALSE(c >= a);
        REQUIRE(c.hash() != a.hash());
        REQUIRE(b.hash() == a.hash());
    }
    SECTION("symbol") {
        std::vector<Symbol> args;
        // numbers
        auto sym = Number(42);
        REQUIRE(42 == sym.number());
        args.emplace_back(sym);
        // inf
        sym = Infimum();
        REQUIRE(SymbolType::Infimum == sym.type());
        args.emplace_back(sym);
        // sup
        sym = Supremum();
        REQUIRE(SymbolType::Supremum == sym.type());
        args.emplace_back(sym);
        // str
        sym = String("x");
        REQUIRE(S("x") == sym.string());
        args.emplace_back(sym);
        // id
        sym = Id("x", false);
        REQUIRE(SymbolType::Function == sym.type());
        REQUIRE(sym.is_negative());
        REQUIRE(!sym.is_positive());
        REQUIRE(S("x") == sym.name());
        args.emplace_back(sym);
        // fun
        sym = Function("f", args);
        REQUIRE(SymbolType::Function == sym.type());
        REQUIRE(!sym.is_negative());
        REQUIRE(S("f") == sym.name());
        REQUIRE("f(42,#inf,#sup,\"x\",-x)" == sym.to_string());
        REQUIRE((args.size() == sym.arguments().size() && std::equal(args.begin(), args.end(), sym.arguments().begin())));
        try { sym.number(); }
        catch (std::exception const &e) { REQUIRE(e.what() == S("unexpected")); }
        // comparison
        auto a = Number(1), b = Number(2);
        REQUIRE(a < b);
        REQUIRE_FALSE(a < a);
        REQUIRE_FALSE(b < a);
        REQUIRE(b > a);
        REQUIRE_FALSE(a > a);
        REQUIRE_FALSE(a > b);
        REQUIRE(a <= a);
        REQUIRE(a <= b);
        REQUIRE_FALSE(b <= a);
        REQUIRE(a >= a);
        REQUIRE(b >= a);
        REQUIRE_FALSE(a >= b);
        REQUIRE(a == a);
        REQUIRE_FALSE(a == b);
        REQUIRE(a != b);
        REQUIRE_FALSE(a != a);
        REQUIRE(a.hash() == a.hash());
        REQUIRE(a.hash() != b.hash());
    }
}

} } // namespace Test Clingo
