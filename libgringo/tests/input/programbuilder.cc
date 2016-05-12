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

#include "gringo/input/programbuilder.hh"
#include "gringo/input/program.hh"
#include "gringo/output/output.hh"
#include "gringo/scripts.hh"

#include "tests/tests.hh"
#include "tests/term_helper.hh"
#include "tests/gringo_module.hh"

#include <climits>
#include <sstream>

namespace Gringo { namespace Input { namespace Test {

using namespace Gringo::Test;

// {{{ declaration of TestProgramBuilder

class TestProgramBuilder : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestProgramBuilder);
        CPPUNIT_TEST(test_term);
        CPPUNIT_TEST(test_literal);
        CPPUNIT_TEST(test_bdaggr);
        CPPUNIT_TEST(test_hdaggr);
        CPPUNIT_TEST(test_rule);
    CPPUNIT_TEST_SUITE_END();

public:
    TestProgramBuilder();

    virtual void setUp();
    virtual void tearDown();
    // {{{ auxiliary function
    typedef std::initializer_list<char const*> Lits;
    typedef std::initializer_list<int> Terms;
    typedef std::initializer_list<std::pair<char const*, Lits>> CondLits;
    typedef std::initializer_list<std::pair<Relation, int>> Bounds;
    typedef std::initializer_list<std::pair<Terms, Lits>> BodyAggrElems;
    typedef std::initializer_list<std::tuple<Terms, char const *, Lits>> HeadAggrElems;
    LitUid lit(const char *name);
    LitVecUid litvec(Lits names);
    TermVecUid termvec(Terms terms);
    CondLitVecUid condlitvec(CondLits names);
    BdAggrElemVecUid bdaggrelemvec(BodyAggrElems elems);
    HdAggrElemVecUid hdaggrelemvec(HeadAggrElems elems);
    BoundVecUid boundvec(Bounds bounds);
    BdLitVecUid bodyaggr(NAF naf, AggregateFunction fun, Bounds, CondLits);
    BdLitVecUid bodyaggr(NAF naf, AggregateFunction fun, Bounds, BodyAggrElems);
    HdLitUid headaggr(AggregateFunction fun, Bounds, CondLits);
    HdLitUid headaggr(AggregateFunction fun, Bounds, HeadAggrElems);
    // }}}
    void test_term();
    void test_literal();
    void test_bdaggr();
    void test_hdaggr();
    void test_rule();

    virtual ~TestProgramBuilder();

    std::ostringstream oss;
    Potassco::TheoryData td;
    Output::OutputBase out;
    Location l;
    Defines defs;
    Program prg;
    Scripts scripts;
    NongroundProgramBuilder p;
};

// }}}

using namespace Gringo::IO;

// {{{ definition of TestProgramBuilder

TestProgramBuilder::TestProgramBuilder()
    : out(td, {}, oss)
    , l(FWString("dummy"), 1, 1, FWString("dummy"), 1, 1)
    , scripts(Gringo::Test::getTestModule())
    , p(scripts, prg, out, defs) { }

void TestProgramBuilder::setUp() {
    defs.~Defines();
    new (&defs) Defines();
    prg.~Program();
    new (&prg) Program();
    p.~NongroundProgramBuilder();
    new (&p) NongroundProgramBuilder(scripts, prg, out, defs);
}

void TestProgramBuilder::tearDown() { }

// {{{ auxiliary functions

LitUid TestProgramBuilder::lit(const char *name) {
    return p.predlit(l, NAF::POS, false, name, p.termvecvec(p.termvecvec(), p.termvec()));
}

LitVecUid TestProgramBuilder::litvec(Lits names) {
    LitVecUid lits = p.litvec();
    for (auto &x : names) { lits = p.litvec(lits, lit(x)); }
    return lits;
}

TermVecUid TestProgramBuilder::termvec(Terms terms) {
    TermVecUid vec = p.termvec();
    for (auto &x : terms) { vec = p.termvec(vec, p.term(l, NUM(x))); }
    return vec;
}

CondLitVecUid TestProgramBuilder::condlitvec(CondLits elems) {
    CondLitVecUid vec = p.condlitvec();
    for (auto &x : elems) { p.condlitvec(vec, lit(x.first), litvec(x.second)); }
    return vec;
}

HdAggrElemVecUid TestProgramBuilder::hdaggrelemvec(HeadAggrElems elems) {
    HdAggrElemVecUid vec = p.headaggrelemvec();
    for (auto &x : elems) { p.headaggrelemvec(vec, termvec(std::get<0>(x)), lit(std::get<1>(x)), litvec(std::get<2>(x))); }
    return vec;
}

BdAggrElemVecUid TestProgramBuilder::bdaggrelemvec(BodyAggrElems elems) {
    BdAggrElemVecUid vec = p.bodyaggrelemvec();
    for (auto &x : elems) { p.bodyaggrelemvec(vec, termvec(x.first), litvec(x.second)); }
    return vec;
}

BoundVecUid TestProgramBuilder::boundvec(Bounds bounds) {
    BoundVecUid vec = p.boundvec();
    for (auto &x : bounds) { vec = p.boundvec(vec, x.first, p.term(l, NUM(x.second))); }
    return vec;
}

BdLitVecUid TestProgramBuilder::bodyaggr(NAF naf, AggregateFunction fun, Bounds bounds, CondLits elems) {
    return p.bodyaggr(p.body(), l, naf, fun, boundvec(bounds), condlitvec(elems));
}

BdLitVecUid TestProgramBuilder::bodyaggr(NAF naf, AggregateFunction fun, Bounds bounds, BodyAggrElems elems) {
    return p.bodyaggr(p.body(), l, naf, fun, boundvec(bounds), bdaggrelemvec(elems));
}

HdLitUid TestProgramBuilder::headaggr(AggregateFunction fun, Bounds bounds, CondLits elems) {
    return p.headaggr(l, fun, boundvec(bounds), condlitvec(elems));
}

HdLitUid TestProgramBuilder::headaggr(AggregateFunction fun, Bounds bounds, HeadAggrElems elems) {
    return p.headaggr(l, fun, boundvec(bounds), hdaggrelemvec(elems));
}

// }}}

void TestProgramBuilder::test_term() {
    auto t = [&](TermUid uid) -> std::string {
        p.rule(l, p.headlit(p.predlit(l, NAF::POS, false, "p", p.termvecvec(p.termvecvec(), p.termvec(p.termvec(), uid)))));
        auto s(to_string(prg));
        setUp();
        if (!s.empty()) { s.pop_back(); }
        return s;
    };
    CPPUNIT_ASSERT_EQUAL(std::string("p(1)."), t(p.term(l, NUM(1))));
    CPPUNIT_ASSERT_EQUAL(std::string("p(X)."), t(p.term(l, FWString("X"))));
    CPPUNIT_ASSERT_EQUAL(std::string("p(_)."), t(p.term(l, FWString("_"))));
    CPPUNIT_ASSERT_EQUAL(std::string("p(|1|)."), t(p.term(l, UnOp::ABS, p.term(l, NUM(1)))));
    CPPUNIT_ASSERT_EQUAL(std::string("p(|1|;|2|;|3|)."), t(p.term(l, UnOp::ABS, p.termvec(p.termvec(p.termvec(p.termvec(), p.term(l, NUM(1))), p.term(l, NUM(2))), p.term(l, NUM(3))))));
    CPPUNIT_ASSERT_EQUAL(std::string("p((1+2))."), t(p.term(l, BinOp::ADD, p.term(l, NUM(1)), p.term(l, NUM(2)))));
    CPPUNIT_ASSERT_EQUAL(std::string("p((1..2))."), t(p.term(l, p.term(l, NUM(1)), p.term(l, NUM(2)))));
    CPPUNIT_ASSERT_EQUAL(std::string("p(f(1);f(2,3,4))."), t(p.term(l, FWString("f"), p.termvecvec(p.termvecvec(p.termvecvec(), p.termvec(p.termvec(), p.term(l, NUM(1)))), p.termvec(p.termvec(p.termvec(p.termvec(), p.term(l, NUM(2))), p.term(l, NUM(3))), p.term(l, NUM(4)))), false)));
    CPPUNIT_ASSERT_EQUAL(std::string("p(@f(1);@f(2,3,4))."), t(p.term(l, FWString("f"), p.termvecvec(p.termvecvec(p.termvecvec(), p.termvec(p.termvec(), p.term(l, NUM(1)))), p.termvec(p.termvec(p.termvec(p.termvec(), p.term(l, NUM(2))), p.term(l, NUM(3))), p.term(l, NUM(4)))), true)));
    CPPUNIT_ASSERT_EQUAL(std::string("p(1;(2,3,4))."), t(p.pool(l,p.termvec(p.termvec(p.termvec(),p.term(l, p.termvec(p.termvec(), p.term(l, NUM(1))), false)), p.term(l, p.termvec(p.termvec(p.termvec(p.termvec(), p.term(l, NUM(2))), p.term(l, NUM(3))), p.term(l, NUM(4))), false)))));
    CPPUNIT_ASSERT_EQUAL(std::string("p((1,);(2,3,4))."), t(
        p.pool(l,
            p.termvec(
                p.termvec(
                    p.termvec(),
                    p.term(l, p.termvec(p.termvec(), p.term(l, NUM(1))), true)),
                p.term(l, p.termvec(p.termvec(p.termvec(p.termvec(), p.term(l, NUM(2))), p.term(l, NUM(3))), p.term(l, NUM(4))), false)))));
}

void TestProgramBuilder::test_literal() {
    auto t = [&](LitUid uid) -> std::string {
        p.rule(l, p.headlit(uid));
        auto s(to_string(prg));
        setUp();
        if (!s.empty()) { s.pop_back(); }
        return s;
    };
    CPPUNIT_ASSERT_EQUAL(std::string("0=0."), t(p.boollit(l, true)));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0."), t(p.boollit(l, false)));
    CPPUNIT_ASSERT_EQUAL(std::string("p."), t(p.predlit(l, NAF::POS, false, "p", p.termvecvec(p.termvecvec(), p.termvec()))));
    CPPUNIT_ASSERT_EQUAL(std::string("(-p)."), t(p.predlit(l, NAF::POS, true, "p", p.termvecvec(p.termvecvec(), p.termvec()))));
    CPPUNIT_ASSERT_EQUAL(std::string("not p."), t(p.predlit(l, NAF::NOT, false, "p", p.termvecvec(p.termvecvec(), p.termvec()))));
    CPPUNIT_ASSERT_EQUAL(std::string("not not p."), t(p.predlit(l, NAF::NOTNOT, false, "p", p.termvecvec(p.termvecvec(), p.termvec()))));
    CPPUNIT_ASSERT_EQUAL(std::string("1<2."), t(p.rellit(l, Relation::LT, p.term(l, NUM(1)), p.term(l, NUM(2)))));
    CPPUNIT_ASSERT_EQUAL(std::string("1>2."), t(p.rellit(l, Relation::GT, p.term(l, NUM(1)), p.term(l, NUM(2)))));
}

void TestProgramBuilder::test_bdaggr() {
    auto t = [&](BdLitVecUid uid) -> std::string {
        p.rule(l, p.headlit(p.boollit(l, false)), uid);
        auto s(to_string(prg));
        setUp();
        if (!s.empty()) { s.pop_back(); }
        return s;
    };
    // BodyAggrElems
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, BodyAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-not #sum{}."), t(bodyaggr(NAF::NOT, AggregateFunction::SUM, {}, BodyAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-not not #sum{}."), t(bodyaggr(NAF::NOTNOT, AggregateFunction::SUM, {}, BodyAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#count{}."), t(bodyaggr(NAF::POS, AggregateFunction::COUNT, {}, BodyAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-1<#sum{}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {{Relation::GT, 1}}, BodyAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-1<#sum{}<2."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {{Relation::GT, 1},{Relation::LT, 2}}, BodyAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-1<#sum{}<2<3."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {{Relation::GT, 1},{Relation::LT, 2},{Relation::LT, 3}}, BodyAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{:}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, BodyAggrElems({ {{}, {}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{1:p}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, BodyAggrElems({ {{1}, {"p"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{1,2:p,q}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, BodyAggrElems({ {{1, 2},{"p", "q"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{:;:}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, BodyAggrElems({ {{},{}}, {{},{}} }))));
    // CondLits
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-not #sum{}."), t(bodyaggr(NAF::NOT, AggregateFunction::SUM, {}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-not not #sum{}."), t(bodyaggr(NAF::NOTNOT, AggregateFunction::SUM, {}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#count{}."), t(bodyaggr(NAF::POS, AggregateFunction::COUNT, {}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-1<#sum{}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {{Relation::GT, 1}}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-1<#sum{}<2."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {{Relation::GT, 1},{Relation::LT, 2}}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-1<#sum{}<2<3."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {{Relation::GT, 1},{Relation::LT, 2},{Relation::LT, 3}}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{p:}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, CondLits({ {"p",{}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{p:q}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, CondLits({ {"p",{"q"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{p:q,r}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, CondLits({ {"p",{"q", "r"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-#sum{p:;q:}."), t(bodyaggr(NAF::POS, AggregateFunction::SUM, {}, CondLits({ {"p",{}}, {"q",{}} }))));
    // Conjunction
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-p:."), t(p.conjunction(p.body(), l, lit("p"), litvec({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-p:q."), t(p.conjunction(p.body(), l, lit("p"), litvec({"q"}))));
    CPPUNIT_ASSERT_EQUAL(std::string("0!=0:-p:q,r."), t(p.conjunction(p.body(), l, lit("p"), litvec({"q", "r"}))));
}

void TestProgramBuilder::test_hdaggr() {
    auto t = [&](HdLitUid uid) -> std::string {
        p.rule(l, uid);
        auto s(to_string(prg));
        setUp();
        if (!s.empty()) { s.pop_back(); }
        return s;
    };
    // HeadAggrElems
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{}."), t(headaggr(AggregateFunction::SUM, {}, HeadAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("#count{}."), t(headaggr(AggregateFunction::COUNT, {}, HeadAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("1<#sum{}."), t(headaggr(AggregateFunction::SUM, {{Relation::GT, 1}}, HeadAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("1<#sum{}<2."), t(headaggr(AggregateFunction::SUM, {{Relation::GT, 1},{Relation::LT, 2}}, HeadAggrElems({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("1<#sum{}<2<3."), t(headaggr(AggregateFunction::SUM, {{Relation::GT, 1},{Relation::LT, 2},{Relation::LT, 3}}, HeadAggrElems({}))));
    typedef std::tuple<Terms, char const *, Lits> E;
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{:p:}."), t(headaggr(AggregateFunction::SUM, {}, HeadAggrElems({ E{{},"p",{}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{1:p:q}."), t(headaggr(AggregateFunction::SUM, {}, HeadAggrElems({ E{{1},"p",{"q"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{1,2:p:q,r}."), t(headaggr(AggregateFunction::SUM, {}, HeadAggrElems({ E{{1,2},"p",{"q", "r"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{:p:;:p:}."), t(headaggr(AggregateFunction::SUM, {}, HeadAggrElems({ E{{},"p",{}}, E{{},"p",{}} }))));
    // CondLits
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{}."), t(headaggr(AggregateFunction::SUM, {}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("#count{}."), t(headaggr(AggregateFunction::COUNT, {}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("1<#sum{}."), t(headaggr(AggregateFunction::SUM, {{Relation::GT,1}}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("1<#sum{}<2."), t(headaggr(AggregateFunction::SUM, {{Relation::GT,1},{Relation::LT,2}}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("1<#sum{}<2<3."), t(headaggr(AggregateFunction::SUM, {{Relation::GT,1},{Relation::LT,2},{Relation::LT,3}}, CondLits({}))));
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{p:}."), t(headaggr(AggregateFunction::SUM, {}, CondLits({ {"p",{}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{p:q}."), t(headaggr(AggregateFunction::SUM, {}, CondLits({ {"p",{"q"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{p:q,r}."), t(headaggr(AggregateFunction::SUM, {}, CondLits({ {"p",{"q","r"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("#sum{p:;q:}."), t(headaggr(AggregateFunction::SUM, {}, CondLits({ {"p",{}},{"q",{}} }))));
    // Disjunction
    CPPUNIT_ASSERT_EQUAL(std::string("p::."), t(p.disjunction(l, condlitvec({ {"p",{}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("p::q."), t(p.disjunction(l, condlitvec({ {"p", {"q"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("p::q,r."), t(p.disjunction(l, condlitvec({ {"p",{"q","r"}} }))));
    CPPUNIT_ASSERT_EQUAL(std::string("p::;q::."), t(p.disjunction(l, condlitvec({ {"p",{}}, {"q",{}} }))));
}

void TestProgramBuilder::test_rule() {
    auto t = [&](char const *name, Lits lits) -> std::string {
        BdLitVecUid vec = p.body();
        for (auto &x : lits) { vec = p.bodylit(vec, lit(x)); }
        p.rule(l, p.headlit(lit(name)), vec);
        auto s(to_string(prg));
        setUp();
        if (!s.empty()) { s.pop_back(); }
        return s;
    };
    CPPUNIT_ASSERT_EQUAL(std::string("a."), t("a",{}));
    CPPUNIT_ASSERT_EQUAL(std::string("a:-b."), t("a",{"b"}));
    CPPUNIT_ASSERT_EQUAL(std::string("a:-b;c."), t("a",{"b","c"}));
    CPPUNIT_ASSERT_EQUAL(std::string("a:-b;c;d."), t("a",{"b","c","d"}));
}

TestProgramBuilder::~TestProgramBuilder() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestProgramBuilder);

} } } // namespace Test Input Gringo


