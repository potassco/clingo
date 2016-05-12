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

#include "gringo/bug.hh"
#include "gringo/input/aggregate.hh"

#include "tests/tests.hh"
#include "tests/term_helper.hh"
#include "tests/input/lit_helper.hh"
#include "tests/input/aggregate_helper.hh"

#include <climits>
#include <sstream>

namespace Gringo { namespace Input { namespace Test {

// {{{ declaration of TestAggregate

class TestAggregate : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestAggregate);
        CPPUNIT_TEST(test_print);
        CPPUNIT_TEST(test_clone);
        CPPUNIT_TEST(test_hash);
        CPPUNIT_TEST(test_equal);
        CPPUNIT_TEST(test_unpool);
        CPPUNIT_TEST(test_simplify);
        CPPUNIT_TEST(test_rewriteArithmetics);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

    void test_print();
    void test_clone();
    void test_hash();
    void test_equal();
    void test_unpool();
    void test_simplify();
    void test_rewriteArithmetics();

    virtual ~TestAggregate();
};

// }}}

using namespace Gringo::IO;

// {{{ auxiliary functions and classes

namespace {

ULit lit(char const *name) { return pred(NAF::POS, val(ID(name))); }
template <class... T>
ULit lit(T... name) { return pred(NAF::POS, pool(val(ID(name))...)); }

UTerm dterm(int a, int b) { return dots(val(NUM(a)), val(NUM(b))); }
ULit dlit(int a, int b) { return pred(NAF::POS, dterm(a, b)); }

UTerm aterm(char const *a, char const * b) { return binop(BinOp::ADD, var(a), var(b)); }
ULit alit(char const * a, char const * b) { return pred(NAF::POS, aterm(a, b)); }

BoundVec bound1() { return boundvec(Relation::LT, val(NUM(3)), Relation::GT, val(NUM(1))); }
BoundVec bound2() { return boundvec(Relation::LT, pool(val(NUM(1)), val(NUM(2))), Relation::GT, pool(val(NUM(3)), val(NUM(4)))); }
BoundVec bound3() { return boundvec(Relation::LT, pool(val(NUM(1)), val(NUM(2)))); }

BodyAggrElemVec bdvec1() { return bdelemvec(); }
BodyAggrElemVec bdvec2() { return bdelemvec(termvec(), litvec()); }
BodyAggrElemVec bdvec3() { return bdelemvec(termvec(val(NUM(1))), litvec(lit("p"))); }
BodyAggrElemVec bdvec4() { return bdelemvec(termvec(val(NUM(1)), val(NUM(2))), litvec(lit("p"), lit("q"))); }
BodyAggrElemVec bdvec5() { return bdelemvec(termvec(), litvec(), termvec(), litvec(lit("p"))); }
// with pool term
BodyAggrElemVec bdvec6() { return bdelemvec(termvec(pool(val(NUM(1)), val(NUM(2)))), litvec(pred(NAF::POS, pool(val(ID("p")), val(ID("q"))))), termvec(), litvec()); }
BodyAggrElemVec bdvec7() { return bdelemvec(termvec(pool(val(NUM(1)), val(NUM(2))), pool(val(NUM(3)), val(NUM(4)))), litvec()); }

CondLitVec condvec1() { return condlitvec(); }
CondLitVec condvec2() { return condlitvec(lit("p"), litvec()); }
CondLitVec condvec3() { return condlitvec(lit("p"), litvec(lit("q"))); }
CondLitVec condvec4() { return condlitvec(lit("p"), litvec(lit("q"), lit("r"))); }
CondLitVec condvec5() { return condlitvec(lit("p"), litvec(), lit("q"), litvec()); }
CondLitVec condvec6() { return condlitvec(lit("p", "q"), litvec(lit("r", "s")), lit("t"), litvec()); }

HeadAggrElemVec hdvec1() { return hdelemvec(); }
HeadAggrElemVec hdvec2() { return hdelemvec(termvec(), lit("p"), litvec()); }
HeadAggrElemVec hdvec3() { return hdelemvec(termvec(val(NUM(1))), lit("p"), litvec(lit("q"))); }
HeadAggrElemVec hdvec4() { return hdelemvec(termvec(val(NUM(1)), val(NUM(2))), lit("p"), litvec(lit("q"), lit("r"))); }
HeadAggrElemVec hdvec5() { return hdelemvec(termvec(), lit("p"), litvec(), termvec(), lit("q"), litvec(lit("r"))); }
// with pool term
HeadAggrElemVec hdvec6() { return hdelemvec(termvec(pool(val(NUM(1)), val(NUM(2)))), lit("p"), litvec(lit("q", "r")), termvec(), lit("s"), litvec()); }
HeadAggrElemVec hdvec7() { return hdelemvec(termvec(pool(val(NUM(1)), val(NUM(2))), pool(val(NUM(3)), val(NUM(4)))), lit("p"), litvec()); }
HeadAggrElemVec hdvec8() { return hdelemvec(termvec(val(NUM(1))), lit("p", "q"), litvec()); }

std::string simplify(UBodyAggr &&x) {
    Projections project;
    SimplifyState state;
    x->simplify(project, state, false);
    return to_string(std::make_pair(std::move(x), std::move(state.dots)));
}

std::string simplify(UHeadAggr &&x) {
    Projections project;
    SimplifyState state;
    x->simplify(project, state);
    return to_string(std::make_pair(std::move(x), std::move(state.dots)));
}

std::string rewrite(UBodyAggr &&x) {
    Literal::AssignVec assign;
    Term::ArithmeticsMap arith;
    arith.emplace_back();
    AuxGen gen;
    AssignLevel v;
    x->assignLevels(v);
    v.assignLevels();
    x->rewriteArithmetics(arith, assign, gen);
    return to_string(std::make_tuple(std::move(x), std::move(arith), std::move(assign)));
}

std::string rewrite(UHeadAggr &&x) {
    Term::ArithmeticsMap arith;
    arith.emplace_back();
    AuxGen gen;
    AssignLevel v;
    x->assignLevels(v);
    v.assignLevels();
    x->rewriteArithmetics(arith, gen);
    return to_string(std::make_pair(std::move(x), std::move(arith)));
}

UHeadAggrVec unpool(UHeadAggr &&x) {
    UHeadAggrVec y;
    x->unpool(y, true);
    return y;
}

UBodyAggrVec unpool(UBodyAggr &&x) {
    UBodyAggrVec y;
    x->unpool(y, true);
    return y;
}

} // namespace

// }}}
// {{{ definition of TestAggregate

void TestAggregate::setUp()
{
}

void TestAggregate::tearDown() { }

void TestAggregate::test_print() {
    // body tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("not 3>#sum{}>1"), to_string(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), bdvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec2())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1:p}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec3())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1,2:p,q}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec4())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:;:p}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec5())));
    // body lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), condvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("not 3>#sum{}>1"), to_string(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), condvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec2())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec3())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q,r}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec4())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:;q:}>1"), to_string(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec5())));
    // conjunction
    CPPUNIT_ASSERT_EQUAL(std::string("p:"), to_string(bdaggr(lit("p"), litvec())));
    CPPUNIT_ASSERT_EQUAL(std::string("p:q"), to_string(bdaggr(lit("p"), litvec(lit("q")))));
    // head tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(hdaggr(AggregateFunction::SUM, bound1(), hdvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), hdvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:p:}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), hdvec2())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1:p:q}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), hdvec3())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1,2:p:q,r}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), hdvec4())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:p:;:q:r}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), hdvec5())));
    // head lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(hdaggr(AggregateFunction::SUM, bound1(), condvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), condvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), condvec2())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), condvec3())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q,r}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), condvec4())));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:;q:}>1"), to_string(hdaggr(AggregateFunction::COUNT, bound1(), condvec5())));
    // disjunction
    CPPUNIT_ASSERT_EQUAL(std::string(""), to_string(hdaggr(condvec1())));
    CPPUNIT_ASSERT_EQUAL(std::string("p::"), to_string(hdaggr(condvec2())));
    CPPUNIT_ASSERT_EQUAL(std::string("p::q"), to_string(hdaggr(condvec3())));
    CPPUNIT_ASSERT_EQUAL(std::string("p::q,r"), to_string(hdaggr(condvec4())));
    CPPUNIT_ASSERT_EQUAL(std::string("p::;q::"), to_string(hdaggr(condvec5())));
}

void TestAggregate::test_clone() {
    // body tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("not 3>#sum{}>1"), to_string(get_clone(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), bdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec2()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1:p}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec3()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1,2:p,q}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec4()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:;:p}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), bdvec5()))));
    // body lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("not 3>#sum{}>1"), to_string(get_clone(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec2()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec3()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q,r}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec4()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:;q:}>1"), to_string(get_clone(bdaggr(NAF::POS, AggregateFunction::COUNT, bound1(), condvec5()))));
    // conjunction
    CPPUNIT_ASSERT_EQUAL(std::string("p:"), to_string(get_clone(bdaggr(lit("p"), litvec()))));
    CPPUNIT_ASSERT_EQUAL(std::string("p:q"), to_string(get_clone(bdaggr(lit("p"), litvec(lit("q"))))));
    // head tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(get_clone(hdaggr(AggregateFunction::SUM, bound1(), hdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), hdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:p:}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), hdvec2()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1:p:q}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), hdvec3()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{1,2:p:q,r}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), hdvec4()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{:p:;:q:r}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), hdvec5()))));
    // head lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("3>#sum{}>1"), to_string(get_clone(hdaggr(AggregateFunction::SUM, bound1(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), condvec2()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), condvec3()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:q,r}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), condvec4()))));
    CPPUNIT_ASSERT_EQUAL(std::string("3>#count{p:;q:}>1"), to_string(get_clone(hdaggr(AggregateFunction::COUNT, bound1(), condvec5()))));
    // disjunction
    CPPUNIT_ASSERT_EQUAL(std::string(""), to_string(get_clone(hdaggr(condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("p::"), to_string(get_clone(hdaggr(condvec2()))));
    CPPUNIT_ASSERT_EQUAL(std::string("p::q"), to_string(get_clone(hdaggr(condvec3()))));
    CPPUNIT_ASSERT_EQUAL(std::string("p::q,r"), to_string(get_clone(hdaggr(condvec4()))));
    CPPUNIT_ASSERT_EQUAL(std::string("p::;q::"), to_string(get_clone(hdaggr(condvec5()))));
}

void TestAggregate::test_hash() {
    char const *msg = "warning: hashes are very unlikely to collide";
    // body tuple aggregate
    auto a1(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec1()));
    auto a2(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec2()));
    auto a3(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), bdvec1()));
    auto a4(bdaggr(NAF::POS, AggregateFunction::COUNT, boundvec(), bdvec1()));
    CPPUNIT_ASSERT_MESSAGE(msg, a1->hash() == get_clone(a1)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a2->hash() == get_clone(a2)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a3->hash() == get_clone(a3)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a4->hash() == get_clone(a4)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a1->hash() != a2->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a1->hash() != a3->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a1->hash() != a4->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a2->hash() != a3->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a2->hash() != a4->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, a3->hash() != a4->hash());
    // body lit aggregate
    auto b1(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), condvec1()));
    auto b2(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), condvec2()));
    auto b3(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), condvec1()));
    auto b4(bdaggr(NAF::POS, AggregateFunction::COUNT, boundvec(), condvec1()));
    CPPUNIT_ASSERT_MESSAGE(msg, b1->hash() == get_clone(b1)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b2->hash() == get_clone(b2)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b3->hash() == get_clone(b3)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b4->hash() == get_clone(b4)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b1->hash() != b2->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b1->hash() != b3->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b1->hash() != b4->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b2->hash() != b3->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b2->hash() != b4->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b3->hash() != b4->hash());
    // conjunction
    auto c1(bdaggr(lit("p"), litvec()));
    auto c2(bdaggr(lit("p"), litvec(lit("q"))));
    CPPUNIT_ASSERT_MESSAGE(msg, c1->hash() == get_clone(c1)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, c2->hash() == get_clone(c2)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, c1->hash() != c2->hash());
    // head tuple aggregate
    auto d1(hdaggr(AggregateFunction::SUM, bound1(), hdvec1()));
    auto d2(hdaggr(AggregateFunction::SUM, bound1(), hdvec2()));
    auto d3(hdaggr(AggregateFunction::COUNT, boundvec(), hdvec1()));
    CPPUNIT_ASSERT_MESSAGE(msg, b1->hash() == get_clone(b1)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b2->hash() == get_clone(b2)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b3->hash() == get_clone(b3)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b1->hash() != b2->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b1->hash() != b3->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, b2->hash() != b3->hash());
    // head lit aggregate
    auto e1(hdaggr(AggregateFunction::SUM, bound1(), condvec1()));
    auto e2(hdaggr(AggregateFunction::SUM, bound1(), condvec2()));
    auto e3(hdaggr(AggregateFunction::COUNT, boundvec(), condvec1()));
    CPPUNIT_ASSERT_MESSAGE(msg, e1->hash() == get_clone(e1)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, e2->hash() == get_clone(e2)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, e3->hash() == get_clone(e3)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, e1->hash() != e2->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, e1->hash() != e3->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, e2->hash() != e3->hash());
    // disjunction
    auto f1(hdaggr(condvec1()));
    auto f2(hdaggr(condvec2()));
    CPPUNIT_ASSERT_MESSAGE(msg, f1->hash() == get_clone(f1)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, f2->hash() == get_clone(f2)->hash());
    CPPUNIT_ASSERT_MESSAGE(msg, f1->hash() != f2->hash());
}

void TestAggregate::test_equal() {
    // body tuple aggregate
    auto a1(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec1()));
    auto a2(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec2()));
    auto a3(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), bdvec1()));
    auto a4(bdaggr(NAF::POS, AggregateFunction::COUNT, boundvec(), bdvec1()));
    CPPUNIT_ASSERT(*a1 == *get_clone(a1));
    CPPUNIT_ASSERT(*a2 == *get_clone(a2));
    CPPUNIT_ASSERT(*a3 == *get_clone(a3));
    CPPUNIT_ASSERT(*a4 == *get_clone(a4));
    CPPUNIT_ASSERT(!(*a1 == *a2));
    CPPUNIT_ASSERT(!(*a1 == *a3));
    CPPUNIT_ASSERT(!(*a1 == *a4));
    CPPUNIT_ASSERT(!(*a2 == *a3));
    CPPUNIT_ASSERT(!(*a2 == *a4));
    CPPUNIT_ASSERT(!(*a3 == *a4));
    // body lit aggregate
    auto b1(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), condvec1()));
    auto b2(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), condvec2()));
    auto b3(bdaggr(NAF::NOT, AggregateFunction::SUM, bound1(), condvec1()));
    auto b4(bdaggr(NAF::POS, AggregateFunction::COUNT, boundvec(), condvec1()));
    CPPUNIT_ASSERT(*b1 == *get_clone(b1));
    CPPUNIT_ASSERT(*b2 == *get_clone(b2));
    CPPUNIT_ASSERT(*b3 == *get_clone(b3));
    CPPUNIT_ASSERT(*b4 == *get_clone(b4));
    CPPUNIT_ASSERT(!(*b1 == *b2));
    CPPUNIT_ASSERT(!(*b1 == *b3));
    CPPUNIT_ASSERT(!(*b1 == *b4));
    CPPUNIT_ASSERT(!(*b2 == *b3));
    CPPUNIT_ASSERT(!(*b2 == *b4));
    CPPUNIT_ASSERT(!(*b3 == *b4));
    // conjunction
    auto c1(bdaggr(lit("p"), litvec()));
    auto c2(bdaggr(lit("p"), litvec(lit("q"))));
    CPPUNIT_ASSERT(*c1 == *get_clone(c1));
    CPPUNIT_ASSERT(*c2 == *get_clone(c2));
    CPPUNIT_ASSERT(!(*c1 == *c2));
    // head tuple aggregate
    auto d1(hdaggr(AggregateFunction::SUM, bound1(), hdvec1()));
    auto d2(hdaggr(AggregateFunction::SUM, bound1(), hdvec2()));
    auto d3(hdaggr(AggregateFunction::COUNT, boundvec(), hdvec1()));
    CPPUNIT_ASSERT(*b1 == *get_clone(b1));
    CPPUNIT_ASSERT(*b2 == *get_clone(b2));
    CPPUNIT_ASSERT(*b3 == *get_clone(b3));
    CPPUNIT_ASSERT(!(*b1 == *b2));
    CPPUNIT_ASSERT(!(*b1 == *b3));
    CPPUNIT_ASSERT(!(*b2 == *b3));
    // head lit aggregate
    auto e1(hdaggr(AggregateFunction::SUM, bound1(), condvec1()));
    auto e2(hdaggr(AggregateFunction::SUM, bound1(), condvec2()));
    auto e3(hdaggr(AggregateFunction::COUNT, boundvec(), condvec1()));
    CPPUNIT_ASSERT(*e1 == *get_clone(e1));
    CPPUNIT_ASSERT(*e2 == *get_clone(e2));
    CPPUNIT_ASSERT(*e3 == *get_clone(e3));
    CPPUNIT_ASSERT(!(*e1 == *e2));
    CPPUNIT_ASSERT(!(*e1 == *e3));
    CPPUNIT_ASSERT(!(*e2 == *e3));
    // disjunction
    auto f1(hdaggr(condvec1()));
    auto f2(hdaggr(condvec2()));
    CPPUNIT_ASSERT(*f1 == *get_clone(f1));
    CPPUNIT_ASSERT(*f2 == *get_clone(f2));
    CPPUNIT_ASSERT(!(*f1 == *f2));
}

void TestAggregate::test_unpool() {
    // body tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("[3>#sum{1,3:;2,3:;1,4:;2,4:}>1]"), to_string(unpool(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec7()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[1>#sum{}>3,2>#sum{}>3,1>#sum{}>4,2>#sum{}>4]"), to_string(unpool(bdaggr(NAF::POS, AggregateFunction::SUM, bound2(), bdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[not 1>#min{}>3,not 2>#min{}>3,not 1>#min{}>4,not 2>#min{}>4]"), to_string(unpool(bdaggr(NAF::NOT, AggregateFunction::MIN, bound2(), bdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[3>#sum{1:p;1:q;2:p;2:q;:}>1]"), to_string(unpool(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), bdvec6()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[1>#sum{1:p;1:q;2:p;2:q;:},2>#sum{1:p;1:q;2:p;2:q;:}]"), to_string(unpool(bdaggr(NAF::POS, AggregateFunction::SUM, bound3(), bdvec6()))));
    // body lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("[1>#sum{}>3,2>#sum{}>3,1>#sum{}>4,2>#sum{}>4]"), to_string(unpool(bdaggr(NAF::POS, AggregateFunction::SUM, bound2(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[not 1>#min{}>3,not 2>#min{}>3,not 1>#min{}>4,not 2>#min{}>4]"), to_string(unpool(bdaggr(NAF::NOT, AggregateFunction::MIN, bound2(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[3>#sum{p:r;p:s;q:r;q:s;t:}>1]"), to_string(unpool(bdaggr(NAF::POS, AggregateFunction::SUM, bound1(), condvec6()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[1>#sum{p:r;p:s;q:r;q:s;t:},2>#sum{p:r;p:s;q:r;q:s;t:}]"), to_string(unpool(bdaggr(NAF::POS, AggregateFunction::SUM, bound3(), condvec6()))));
    // conjunction
    CPPUNIT_ASSERT_EQUAL(std::string("[p|q:r]"), to_string(unpool(bdaggr(lit("p", "q"), litvec(lit("r"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("[p:q;p:r]"), to_string(unpool(bdaggr(lit("p"), litvec(lit("q", "r"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("[p|q:r;p|q:s]"), to_string(unpool(bdaggr(lit("p", "q"), litvec(lit("r", "s"))))));
    // head tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("[3>#sum{1,3:p:;2,3:p:;1,4:p:;2,4:p:}>1]"), to_string(unpool(hdaggr(AggregateFunction::SUM, bound1(), hdvec7()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[1>#sum{}>3,2>#sum{}>3,1>#sum{}>4,2>#sum{}>4]"), to_string(unpool(hdaggr(AggregateFunction::SUM, bound2(), hdvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[3>#sum{1:p:q;1:p:r;2:p:q;2:p:r;:s:}>1]"), to_string(unpool(hdaggr(AggregateFunction::SUM, bound1(), hdvec6()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[3>#sum{1:p:;1:q:}>1]"), to_string(unpool(hdaggr(AggregateFunction::SUM, bound1(), hdvec8()))));
    // head lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("[1>#sum{}>3,2>#sum{}>3,1>#sum{}>4,2>#sum{}>4]"), to_string(unpool(hdaggr(AggregateFunction::SUM, bound2(), condvec1()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[3>#sum{p:r;p:s;q:r;q:s;t:}>1]"), to_string(unpool(hdaggr(AggregateFunction::SUM, bound1(), condvec6()))));
    CPPUNIT_ASSERT_EQUAL(std::string("[1>#sum{p:r;p:s;q:r;q:s;t:},2>#sum{p:r;p:s;q:r;q:s;t:}]"), to_string(unpool(hdaggr(AggregateFunction::SUM, bound3(), condvec6()))));
    // disjunction
    CPPUNIT_ASSERT_EQUAL(std::string("[p:&q::r;p:&q::s;t::]"), to_string(unpool(hdaggr(condvec6()))));
}

void TestAggregate::test_simplify() {
    // body tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Range0>#sum{#Range1:#Range2,#range(#Range1,3,4),#range(#Range2,5,6)},[(#Range0,1,2)])"), simplify(bdaggr(NAF::POS, AggregateFunction::SUM, boundvec(Relation::LT, dterm(1,2)), bdelemvec(termvec(dterm(3,4)), litvec(dlit(5,6))))));
    // body lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Range0>#sum{#Range1:#Range2,#range(#Range1,3,4),#range(#Range2,5,6)},[(#Range0,1,2)])"), simplify(bdaggr(NAF::POS, AggregateFunction::SUM, boundvec(Relation::LT, dterm(1,2)), condlitvec(dlit(3,4), litvec(dlit(5,6))))));
    // conjunction
    CPPUNIT_ASSERT_EQUAL(std::string("(#Range0&#range(#Range0,1,2):#Range1,#range(#Range1,3,4),[])"), simplify(bdaggr(dlit(1,2), litvec(dlit(3,4)))));
    // head tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Range0>#sum{#Range1:#Range2:#Range3,#range(#Range1,3,4),#range(#Range2,5,6),#range(#Range3,7,8)},[(#Range0,1,2)])"), simplify(hdaggr(AggregateFunction::SUM, boundvec(Relation::LT, dterm(1,2)), hdelemvec(termvec(dterm(3,4)), dlit(5,6), litvec(dlit(7,8))))));
    // head lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Range0>#sum{#Range1:#Range2,#range(#Range1,3,4),#range(#Range2,5,6)},[(#Range0,1,2)])"), simplify(hdaggr(AggregateFunction::SUM, boundvec(Relation::LT, dterm(1,2)), condlitvec(dlit(3,4), litvec(dlit(5,6))))));
    // disjunction
    CPPUNIT_ASSERT_EQUAL(std::string("(#Range0:#range(#Range0,1,2):#Range1,#range(#Range1,3,4),[])"), simplify(hdaggr(condlitvec(dlit(1,2), litvec(dlit(3,4))))));
}

void TestAggregate::test_rewriteArithmetics() {
    // body tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Arith0>#sum{(C+D):#Arith1,#Arith1=(E+F)},[{(A+B):#Arith0}],[])"), rewrite(bdaggr(NAF::POS, AggregateFunction::SUM, boundvec(Relation::LT, aterm("A","B")), bdelemvec(termvec(aterm("C","D")), litvec(alit("E","F"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("(#Arith0=#sum{(C+D):#Arith0},[{(A+B):#Arith0}],[])"), rewrite(bdaggr(NAF::POS, AggregateFunction::SUM, boundvec(Relation::EQ, aterm("A","B")), bdelemvec(termvec(aterm("C","D")), litvec(alit("A","B"))))));
    // body lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Arith0>#sum{(C+D):#Arith1,#Arith1=(E+F)},[{(A+B):#Arith0}],[])"), rewrite(bdaggr(NAF::POS, AggregateFunction::SUM, boundvec(Relation::LT, aterm("A","B")), condlitvec(alit("C","D"), litvec(alit("E","F"))))));
    CPPUNIT_ASSERT_EQUAL(std::string("(#Arith0=#sum{(C+D):#Arith0},[{(A+B):#Arith0}],[])"), rewrite(bdaggr(NAF::POS, AggregateFunction::SUM, boundvec(Relation::EQ, aterm("A","B")), condlitvec(alit("C","D"), litvec(alit("A","B"))))));
    // conjunction
    CPPUNIT_ASSERT_EQUAL(std::string("(#Arith0&#Arith0=(C+D):#Arith1,#Arith1=(E+F),[{}],[])"), rewrite(bdaggr(alit("C","D"), litvec(alit("E","F")))));
    // head tuple aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Arith0>#sum{(C+D):(E+F):#Arith1,#Arith1=(G+H)},[{(A+B):#Arith0}])"), rewrite(hdaggr(AggregateFunction::SUM, boundvec(Relation::LT, aterm("A","B")), hdelemvec(termvec(aterm("C","D")), alit("E", "F"), litvec(alit("G","H"))))));
    // head lit aggregate
    CPPUNIT_ASSERT_EQUAL(std::string("(#Arith0>#sum{(C+D):#Arith1,#Arith1=(E+F)},[{(A+B):#Arith0}])"), rewrite(hdaggr(AggregateFunction::SUM, boundvec(Relation::LT, aterm("A","B")), condlitvec(alit("C","D"), litvec(alit("E","F"))))));
    // disjunction
    CPPUNIT_ASSERT_EQUAL(std::string("((C+D)::#Arith0,#Arith0=(E+F),#Arith1=(E+F),#Arith1=#Arith0,[{}])"), rewrite(hdaggr(condlitvec(alit("C","D"), litvec(alit("E","F"))))));
}

TestAggregate::~TestAggregate() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestAggregate);

} } } // namespace Test Input Gringo

