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

#include "gringo/safetycheck.hh"

#include "tests/tests.hh"
#include "tests/term_helper.hh"

#include <map>

namespace Gringo { namespace Test {

using namespace Gringo::IO;

// {{{ definition of auxiliary functions

namespace {

typedef SafetyChecker<std::string, std::string> C;
typedef std::tuple<std::string, std::vector<std::string>, std::vector<std::string>> T;

std::string check(std::vector<T> list) {
    C dep;
    std::unordered_map<std::string, C::VarNode*> vars;
    auto iv = [&vars, &dep](std::string const &y) -> C::VarNode& {
        auto &pVar = vars[y];
        if (!pVar) { pVar = &dep.insertVar(y); }
        return *pVar;
    };
    for (auto &x : list) {
        auto &ent(dep.insertEnt(std::get<0>(x)));
        for (auto &y : std::get<1>(x)) { dep.insertEdge(iv(y), ent); }
        for (auto &y : std::get<2>(x)) { dep.insertEdge(ent, iv(y)); }
    }
    std::vector<std::string> order, open;
    for (auto &x : dep.order()) { order.push_back(x->data); }
    for (auto &x : dep.open()) { open.push_back(x->data); }
    std::sort(open.begin(), open.end());
    return to_string(std::make_pair(std::move(order), std::move(open)));
}

} // namespace

// }}}

TEST_CASE("safetycheck", "[base]") {
    SECTION("safety") {
        REQUIRE("([Y=1,X=Y],[])" == check({ T{"X=Y",{"Y"},{"X"}}, T{"Y=1",{},{"Y"}} }));
        REQUIRE("([],[X,Y])" == check({ T{"0=X+Y",{"X","Y"}, {}} }));
        REQUIRE("([A=1,B=A,C=B,D=C],[])" == check({ T{"A=1",{}, {"A"}}, T{"B=A",{"A"}, {"B"}}, T{"C=B",{"B"}, {"C"}}, T{"D=C",{"C"}, {"D"}} }));
        REQUIRE("([],[A,B])" == check({ T{"A=B",{"B"}, {"A"}}, T{"B=A",{"A"}, {"B"}} }));
        REQUIRE("([A=1,B=A,A=B],[])" == check({ T{"A=B",{"B"}, {"A"}}, T{"B=A",{"A"}, {"B"}}, T{"A=1",{}, {"A"}} }));
        REQUIRE("([(X,Y)=(1,1),2=X+Y],[])" == check({ T{"(X,Y)=(1,1)",{}, {"X","Y"}}, T{"2=X+Y",{"X", "Y"}, {}} }));
    }
}

} } // namespace Test Gringo
