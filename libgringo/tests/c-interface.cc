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
#include "clingo.h"

namespace Gringo { namespace Test {

TEST_CASE("c-interface", "[base]") {
    clingo_error_t success = static_cast<clingo_error_t>(clingo_error_success);
    using S = std::string;

    SECTION("symbol") {
        std::vector<clingo_symbol_t> args;
        // numbers
        clingo_symbol_t sym;
        clingo_symbol_new_num(42, &sym);
        int num;
        REQUIRE(success == clingo_symbol_num(sym, &num));
        REQUIRE(42 == num);
        args.emplace_back(sym);
        // inf
        clingo_symbol_new_inf(&sym);
        REQUIRE(static_cast<clingo_symbol_type_t>(clingo_symbol_type_inf) == clingo_symbol_type(sym));
        args.emplace_back(sym);
        // sup
        clingo_symbol_new_sup(&sym);
        REQUIRE(static_cast<clingo_symbol_type_t>(clingo_symbol_type_sup) == clingo_symbol_type(sym));
        args.emplace_back(sym);
        // str
        REQUIRE(success == clingo_symbol_new_str("x", &sym));
        char const *str;
        REQUIRE(success == clingo_symbol_string(sym, &str));
        REQUIRE("x" == S(str));
        args.emplace_back(sym);
        // id
        REQUIRE(success == clingo_symbol_new_id("x", true, &sym));
        REQUIRE(static_cast<clingo_symbol_type_t>(clingo_symbol_type_fun) == clingo_symbol_type(sym));
        bool sign;
        REQUIRE(static_cast<clingo_error_t>(clingo_error_success) == clingo_symbol_sign(sym, &sign));
        REQUIRE(sign);
        REQUIRE(static_cast<clingo_error_t>(clingo_error_success) == clingo_symbol_name(sym, &str));
        REQUIRE(S("x") == S(str));
        args.emplace_back(sym);
        // fun
        REQUIRE(success == clingo_symbol_new_fun("f", {args.data(), args.size()}, false, &sym));
        REQUIRE(static_cast<clingo_symbol_type_t>(clingo_symbol_type_fun) == clingo_symbol_type(sym));
        REQUIRE(success == clingo_symbol_sign(sym, &sign));
        REQUIRE(!sign);
        REQUIRE(success == clingo_symbol_name(sym, &str));
        REQUIRE("f" == S(str));
        std::string res;
        REQUIRE(static_cast<clingo_error_t>(-42) == clingo_symbol_to_string(sym,
            [](char const *str, void *data) -> clingo_error_t {
                *reinterpret_cast<std::string*>(data) = str;
                return -42;
            }, &res));
        REQUIRE("f(42,#inf,#sup,\"x\",-x)" == res);
        clingo_symbol_span_t span;
        REQUIRE(success == clingo_symbol_args(sym, &span));
        auto it = span.first;
        for (auto &x : args) { REQUIRE(clingo_symbol_eq(x, *it++)); }
        REQUIRE(clingo_error_success != clingo_symbol_num(sym, &num));
        // comparison
        clingo_symbol_t a, b;
        clingo_symbol_new_num(1, &a);
        clingo_symbol_new_num(2, &b);
        REQUIRE( clingo_symbol_lt(a, b));
        REQUIRE(!clingo_symbol_lt(a, a));
        REQUIRE(!clingo_symbol_lt(b, a));
        REQUIRE( clingo_symbol_eq(a, a));
        REQUIRE(!clingo_symbol_eq(a, b));
        REQUIRE(clingo_symbol_hash(a) == clingo_symbol_hash(a));
        REQUIRE(clingo_symbol_hash(a) != clingo_symbol_hash(b));
    }
}

} } // namespace Test Gringo
