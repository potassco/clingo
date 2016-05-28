//
// Copyright (c) 2015, Benjamin Kaufmann
//
// This file is part of Potassco. See http://potassco.sourceforge.net/
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "catch.hpp"
#include "clingo.h"
#include <vector>
#include <iostream>

using namespace Clingo;

using AtomVec = std::vector<Symbol>;
using ModelVec = std::vector<AtomVec>;

struct MCB {
    MCB(ModelVec &models) : models(models) { }
    bool operator()(Model m) {
        models.emplace_back();
        for (auto sym : m.atoms(ShowType::All)) {
            models.back().emplace_back(sym);
        }
        std::sort(models.back().begin(), models.back().end());
        return true;
    };
    ModelVec &models;
};

TEST_CASE("c-interface", "[clingo]") {
    using S = std::string;

    SECTION("symbol") {
        std::vector<Symbol> args;
        // numbers
        auto sym = Num(42);
        REQUIRE(42 == sym.num());
        args.emplace_back(sym);
        // inf
        sym = Inf();
        REQUIRE(SymbolType::Inf == sym.type());
        args.emplace_back(sym);
        // sup
        sym = Sup();
        REQUIRE(SymbolType::Sup == sym.type());
        args.emplace_back(sym);
        // str
        sym = Str("x");
        REQUIRE(S("x") == sym.string());
        args.emplace_back(sym);
        // id
        sym = Id("x", true);
        REQUIRE(SymbolType::Fun == sym.type());
        REQUIRE(sym.sign());
        REQUIRE(S("x") == sym.name());
        args.emplace_back(sym);
        // fun
        sym = Fun("f", args);
        REQUIRE(SymbolType::Fun == sym.type());
        REQUIRE(!sym.sign());
        REQUIRE(S("f") == sym.name());
        REQUIRE("f(42,#inf,#sup,\"x\",-x)" == sym.toString());
        REQUIRE((args.size() == sym.args().size() && std::equal(args.begin(), args.end(), sym.args().begin())));
        REQUIRE_THROWS(sym.num());
        // comparison
        auto a = Num(1), b = Num(2);
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
    SECTION("with module") {
        clingo_module *mod;
        REQUIRE(clingo_module_new(&mod) == clingo_error_success);
        SECTION("with control") {
            clingo_control_t *cctl;
            char const *argv[] = { "test_libclingo", nullptr };
            REQUIRE(clingo_control_new(mod, sizeof(argv) / sizeof(char const *), argv, nullptr, nullptr, 20, &cctl) == clingo_error_success);
            Control ctl{cctl};
            SECTION("ground and solve base") {
                ctl.add("base", {}, "a.");
                ctl.ground({{"base", {}}}, nullptr);
                ModelVec models;
                REQUIRE(ctl.solve({}, MCB(models)).sat());
                clingo_symbol_t a;
                REQUIRE(models == ModelVec{{Id("a")}});
            }
        }
        clingo_module_free(mod);
    }
}

