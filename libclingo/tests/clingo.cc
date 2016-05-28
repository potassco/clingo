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

using Model = std::vector<clingo_symbol_t>;
using ModelVec = std::vector<Model>;

clingo_error_t mcb(clingo_model *m, void *data, bool *ret) {
    auto &models = *static_cast<ModelVec*>(data);
    models.emplace_back();
    clingo_symbol_span_t model;
    clingo_model_atoms(m, clingo_show_type_all, &model);
    for (auto it = model.first, ie = it + model.size; it != ie; ++it) {
        models.back().emplace_back(*it);
    }
    std::sort(models.back().begin(), models.back().end(), clingo_symbol_lt);
    *ret = true;
    return clingo_error_success;
};

bool operator==(Model const &a, Model const &b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), clingo_symbol_eq);
}

bool operator==(ModelVec const &a, ModelVec const &b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

TEST_CASE("c-interface-symbol", "[clingo]") {
    using namespace Clingo;
    clingo_error_t success = static_cast<clingo_error_t>(clingo_error_success);
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
}

TEST_CASE("clingo C-interface", "[clingo]") {
    SECTION("with module") {
        clingo_module *mod;
        REQUIRE(clingo_module_new(&mod) == clingo_error_success);
        SECTION("with control") {
            clingo_control_t *ctl;
            char const *argv[] = { "test_libclingo", nullptr };
            REQUIRE(clingo_control_new(mod, sizeof(argv), argv, nullptr, nullptr, 20, &ctl) == clingo_error_success);
            SECTION("ground and solve base") {
                char const * params_base[] = { nullptr };
                REQUIRE(clingo_control_add(ctl, "base", params_base, "a.") == clingo_error_success);
                clingo_symbol_t *values_base = nullptr;
                clingo_part_t parts[] = { {"base", {values_base, 0}} };
                REQUIRE(clingo_control_ground(ctl, {parts, sizeof(parts) / sizeof(clingo_part_t)}, nullptr, nullptr) == clingo_error_success);
                clingo_solve_result_t ret;
                clingo_symbolic_literal_t *ass = nullptr;
                ModelVec models;
                REQUIRE(clingo_control_solve(ctl, {ass, 0}, &mcb, &models, &ret) == clingo_error_success);
                REQUIRE((ret & clingo_solve_result_sat) == clingo_solve_result_sat);
                clingo_symbol_t a;
                REQUIRE(clingo_symbol_new_id("a", false, &a) == clingo_error_success);
                REQUIRE(models == ModelVec{{a}});
            }
            clingo_control_free(ctl);
        }
        clingo_module_free(mod);
    }
}

