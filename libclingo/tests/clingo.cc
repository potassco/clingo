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

TEST_CASE("clingo C-interface", "[clingo]") {
    SECTION("with module") {
        clingo_module *mod;
        REQUIRE(clingo_module_new(&mod) == clingo_error_success);
        SECTION("with control") {
            clingo_control_t *ctl;
            char const *argv[] = { "test_libclingo", nullptr };
            REQUIRE(clingo_control_new(mod, sizeof(argv), argv, &ctl) == clingo_error_success);
            SECTION("ground and solve base") {
                char const * params_base[] = { nullptr };
                REQUIRE(clingo_control_add(ctl, "base", params_base, "a.") == clingo_error_success);
                clingo_symbol_t values_base[0] = {};
                clingo_part_t parts[] = { {"base", {values_base, sizeof(values_base) / sizeof(clingo_symbol_t)}} };
                REQUIRE(clingo_control_ground(ctl, {parts, sizeof(parts) / sizeof(clingo_part_t)}, nullptr, nullptr) == clingo_error_success);
                clingo_solve_result_t ret;
                clingo_symbolic_literal_t ass[0] = { };
                ModelVec models;
                REQUIRE(clingo_control_solve(ctl, {ass, sizeof(ass) / sizeof(clingo_symbolic_literal_t)}, &mcb, &models, &ret) == clingo_error_success);
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

