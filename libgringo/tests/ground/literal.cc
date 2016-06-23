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

#include "gringo/ground/literals.hh"
#include "gringo/scripts.hh"

#include "tests/tests.hh"
#include "tests/term_helper.hh"

#include <functional>

namespace Gringo { namespace Ground { namespace Test {

using namespace Gringo::IO;
using namespace Gringo::Test;

namespace {

using S = std::string;
using V = Symbol;
template <class T>
using L = std::initializer_list<T>;
template <class A, class B>
using P = std::pair<A,B>;
template <class T>
using U = std::unique_ptr<T>;

std::string evalRange(UTerm assign, UTerm l, UTerm r) {
    Gringo::Test::TestGringoModule module;
    Potassco::TheoryData theory;
    DomainData data(theory);
    Scripts scripts(module);
    RangeLiteral lit(get_clone(assign), get_clone(l), get_clone(r));
    Term::VarSet bound;
    UIdx idx(lit.index(scripts, BinderType::ALL, bound));
    SymVec vals;
    idx->match(module.logger);
    bool undefined = false;
    while (idx->next()) { vals.emplace_back(assign->eval(undefined, module.logger)); }
    return to_string(vals);
}

std::string evalRelation(Relation rel, UTerm l, UTerm r) {
    Gringo::Test::TestGringoModule module;
    Potassco::TheoryData theory;
    DomainData data(theory);
    Scripts scripts(module);
    Term::VarSet bound;
    RelationLiteral lit(rel, get_clone(l), get_clone(r));
    UIdx idx(lit.index(scripts, BinderType::ALL, bound));
    std::vector<S> vals;
    idx->match(module.logger);
    bool undefined = false;
    while (idx->next()) {
        vals.emplace_back();
        vals.back() += to_string(l->eval(undefined, module.logger));
        vals.back() += to_string(rel);
        vals.back() += to_string(r->eval(undefined, module.logger));
    }
    return to_string(vals);
}

S evalPred(L<L<V>> vals, L<P<S,V>> bound, BinderType type, NAF naf, UTerm &&repr, bool recursive = false) {
    Gringo::Test::TestGringoModule module;
    Potassco::TheoryData theory;
    DomainData data(theory);
    Scripts scripts(module);
    Term::VarSet boundSet;
    for (auto &x : bound) {
        U<VarTerm> v(var(x.first.c_str()));
        boundSet.emplace(x.first.c_str());
        *v->ref = x.second;
    }
    auto &dom = data.add(Sig("f", 2, false));
    PredicateLiteral lit(false, dom, naf, get_clone(repr));
    if (recursive) { lit.setType(OccurrenceType::UNSTRATIFIED); }
    UIdx idx = lit.index(scripts, type, boundSet);
    std::vector<std::vector<S>> ret;
    dom.init();
    for (auto &x : vals) {
        ret.emplace_back();
        for (auto &y : x) {
            dom.define(y, y < NUM(0));
        }
        IndexUpdater *up{idx->getUpdater()};
        if (up) { up->update(); }
        dom.nextGeneration();
        idx->match(module.logger);
        while (idx->next()) {
            ret.back().emplace_back();
            if (lit.offset == std::numeric_limits<PredicateDomain::SizeType>::max()) {
                ret.back().back() += "#false";
            }
            else {
                ret.back().back() += to_string(static_cast<Symbol>(dom[lit.offset]));
            }
        }
        std::sort(ret.back().begin(), ret.back().end());
    }
    return to_string(ret);
}

}// namespace

TEST_CASE("ground-literal", "[ground]") {

    SECTION("range") {
        REQUIRE("[]"    == evalRange(var("X"), val(NUM(1)), val(NUM(0))));
        REQUIRE("[1]"   == evalRange(var("X"), val(NUM(1)), val(NUM(1))));
        REQUIRE("[1,2]" == evalRange(var("X"), val(NUM(1)), val(NUM(2))));
        REQUIRE("[]"    == evalRange(var("X"), val(NUM(1)), val(ID("a"))));
        REQUIRE("[]"    == evalRange(val(NUM(0)), val(NUM(1)), val(NUM(2))));
        REQUIRE("[1]"   == evalRange(val(NUM(1)), val(NUM(1)), val(NUM(2))));
        REQUIRE("[2]"   == evalRange(val(NUM(2)), val(NUM(1)), val(NUM(2))));
        REQUIRE("[]"    == evalRange(val(NUM(3)), val(NUM(1)), val(NUM(2))));
    }

    SECTION("relation") {
        REQUIRE("[]"                    == evalRelation(Relation::LT, val(NUM(1)), val(NUM(1))));
        REQUIRE("[1<2]"                 == evalRelation(Relation::LT, val(NUM(1)), val(NUM(2))));
        REQUIRE("[1=1]"                 == evalRelation(Relation::EQ, var("X"), val(NUM(1))));
        REQUIRE("[f(1,g(1))=f(1,g(1))]" == evalRelation(Relation::EQ, fun("f", var("X"), fun("g", var("X"))), val(FUN("f", {NUM(1), FUN("g", {NUM(1)})}))));
        REQUIRE("[]"                    == evalRelation(Relation::EQ, fun("f", var("X"), fun("g", var("X"))), val(FUN("f", {NUM(1), FUN("g", {NUM(2)})}))));
    }

    SECTION("pred") {
        // BIND + LOOKUP + POS + OLD/NEW/ALL
        REQUIRE("[[f(1,1),f(1,2)],[f(1,1),f(1,2),f(1,3)]]" == evalPred({{FUN("f",{NUM(1),NUM(1)}),FUN("f",{NUM(2),NUM(2)}),FUN("f",{NUM(1),NUM(2)})},{FUN("f",{NUM(1),NUM(3)})}}, {{"X",NUM(1)}}, BinderType::ALL, NAF::POS, fun("f",var("X"),var("Y")), true));
        REQUIRE("[[],[f(1,1),f(1,2)]]"                     == evalPred({{FUN("f",{NUM(1),NUM(1)}),FUN("f",{NUM(2),NUM(2)}),FUN("f",{NUM(1),NUM(2)})},{FUN("f",{NUM(1),NUM(3)})}}, {{"X",NUM(1)}}, BinderType::OLD, NAF::POS, fun("f",var("X"),var("Y")), true));
        REQUIRE("[[f(1,1),f(1,2)],[f(1,3)]]"               == evalPred({{FUN("f",{NUM(1),NUM(1)}),FUN("f",{NUM(2),NUM(2)}),FUN("f",{NUM(1),NUM(2)})},{FUN("f",{NUM(1),NUM(3)})}}, {{"X",NUM(1)}}, BinderType::NEW, NAF::POS, fun("f",var("X"),var("Y")), true));
        // MATCH + POS + OLD/NEW/ALL + recursive
        REQUIRE("[[],[1],[1]]" == evalPred({{NUM(2)},{NUM(1)},{NUM(3)}}, {}, BinderType::ALL, NAF::POS, val(NUM(1)), true));
        REQUIRE("[[],[],[1]]"  == evalPred({{NUM(2)},{NUM(1)},{NUM(3)}}, {}, BinderType::OLD, NAF::POS, val(NUM(1)), true));
        REQUIRE("[[],[1],[]]"  == evalPred({{NUM(2)},{NUM(1)},{NUM(3)}}, {}, BinderType::NEW, NAF::POS, val(NUM(1)), true));
        REQUIRE("[[],[1],[1]]" == evalPred({{NUM(2)},{NUM(1)},{NUM(3)}}, {}, BinderType::ALL, NAF::POS, val(NUM(1))));
        // MATCH NOT fact/unknown
        REQUIRE("[[1],[1]]"           == evalPred({{NUM(1),NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOT, val(NUM(1))));
        REQUIRE("[[],[]]"             == evalPred({{NUM(-1),NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOT, val(NUM(-1))));
        REQUIRE("[[1],[1]]"           == evalPred({{NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOT, val(NUM(1)), true));
        REQUIRE("[[#false],[#false]]" == evalPred({{NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOT, val(NUM(1))));
        // MATCH NOTNOT fact/unknown
        REQUIRE("[[1],[1]]"   == evalPred({{NUM(1),NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOTNOT, val(NUM(1))));
        REQUIRE("[[-1],[-1]]" == evalPred({{NUM(-1),NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOTNOT, val(NUM(-1))));
        REQUIRE("[[1],[1]]"   == evalPred({{NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOTNOT, val(NUM(1)), true));
        REQUIRE("[[],[]]"     == evalPred({{NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::NOTNOT, val(NUM(1))));
        // FULL + OLD/NEW/ALL
        REQUIRE("[[1,2],[1,2,3]]" == evalPred({{NUM(1),NUM(2)},{NUM(3)}}, {}, BinderType::ALL, NAF::POS, var("X"), true));
        REQUIRE("[[1,2],[3]]"     == evalPred({{NUM(1),NUM(2)},{NUM(3)}}, {}, BinderType::NEW, NAF::POS, var("X"), true));
        REQUIRE("[[],[1,2]]"      == evalPred({{NUM(1),NUM(2)},{NUM(3)}}, {}, BinderType::OLD, NAF::POS, var("X"), true));
        // BIND + LOOKUP + POS + OLD/NEW/ALL
        REQUIRE("[[f(1,1),f(1,2)],[f(1,1),f(1,2),f(1,3)]]" == evalPred({{FUN("f",{NUM(1),NUM(1)}),FUN("f",{NUM(2),NUM(2)}),FUN("f",{NUM(1),NUM(2)})},{FUN("f",{NUM(1),NUM(3)})}}, {{"X",NUM(1)}}, BinderType::ALL, NAF::POS, fun("f",var("X"),var("Y")), true));
        REQUIRE("[[],[f(1,1),f(1,2)]]"                     == evalPred({{FUN("f",{NUM(1),NUM(1)}),FUN("f",{NUM(2),NUM(2)}),FUN("f",{NUM(1),NUM(2)})},{FUN("f",{NUM(1),NUM(3)})}}, {{"X",NUM(1)}}, BinderType::OLD, NAF::POS, fun("f",var("X"),var("Y")), true));
        REQUIRE("[[f(1,1),f(1,2)],[f(1,3)]]"               == evalPred({{FUN("f",{NUM(1),NUM(1)}),FUN("f",{NUM(2),NUM(2)}),FUN("f",{NUM(1),NUM(2)})},{FUN("f",{NUM(1),NUM(3)})}}, {{"X",NUM(1)}}, BinderType::NEW, NAF::POS, fun("f",var("X"),var("Y")), true));
    }
}

} } } // namespace Test Ground Gringo

