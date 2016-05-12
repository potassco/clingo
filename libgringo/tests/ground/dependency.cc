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

#include "gringo/ground/dependency.hh"

#include "tests/tests.hh"
#include "tests/term_helper.hh"

#include <functional>

namespace Gringo { namespace Ground { namespace Test {

// {{{ declaration of TestDependency

class TestDependency : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestDependency);
        CPPUNIT_TEST(test_match);
        CPPUNIT_TEST(test_unify);
		CPPUNIT_TEST(test_dep1);
        CPPUNIT_TEST(test_dep2);
        CPPUNIT_TEST(test_dep3);
        CPPUNIT_TEST(test_dep4);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();
    
    void test_match();
    void test_unify();
    void test_dep1();
    void test_dep2();
    void test_dep3();
    void test_dep4();

    virtual ~TestDependency();
};

// }}}

// {{{ definition of auxiliary functions

namespace {

using namespace Gringo::Test;
using namespace Gringo::IO;

typedef std::set<std::string> StringSet;
typedef Value V;
typedef std::string S;

struct TestLookup {
    typedef Lookup<UGTerm> L;
    void add(UTerm const &x) {
        auto occ   = x->gterm();
        auto &term = *occ;
        l.add(term, std::move(occ));
    }
    void matchTerm(L::iterator begin, L::iterator end) {
        for (auto it = begin; it != end; ++it){ str.insert(to_string(*it->first)); }
    }
    std::string match(Value const &x) {
        str.clear();
        using namespace std::placeholders;
        l.match(x, std::bind(&TestLookup::matchTerm, this, _1, _2));
        return to_string(str);
    }
    std::string unify(UTerm const &x) {
        str.clear();
        auto term(x->gterm());
        using namespace std::placeholders;
        l.unify(*term, std::bind(&TestLookup::matchTerm, this, _1, _2));
        return to_string(str);
    }
    StringSet str;
    L l;
};

struct TestBodyOccurrence : BodyOccurrence<S> {
    TestBodyOccurrence(unsigned num, UTerm &&term, bool positive = true) : num(num), term(std::move(term)), positive(positive) { }
    virtual UGTerm getRepr() const  { return term->gterm(); }
    virtual bool isPositive() const { return positive; }
    virtual bool isNegative() const { return !positive; }
    virtual void setType(OccurrenceType x)    { type = x; }
    virtual OccurrenceType getType() const { return type; }
    void print(std::ostream &out) const {
        out << *term << "@" << num << ":";
        switch (type) {
            case OccurrenceType::POSITIVELY_STRATIFIED: { out << "!"; break; }
            case OccurrenceType::STRATIFIED:            { out << "."; break; }
            case OccurrenceType::UNSTRATIFIED:          { out << "?"; break; }
        }
    }
    virtual DefinedBy &definedBy() { return defs; }
    virtual void checkDefined(LocSet &, SigSet const &, UndefVec &) const { }
    virtual ~TestBodyOccurrence() { }
    unsigned num;
    UTerm term;
    OccurrenceType type = OccurrenceType::UNSTRATIFIED;
    bool positive;
    DefinedBy defs;
};

struct TestDep {
    Dependency<S,S>::Node &add(S const &name, bool normal) { return dep.add(S(name), normal); }
    void provides(Dependency<S,S>::Node &x, UTerm &&y) {
        heads.emplace_front(to_string(*y));
        dep.provides(x, heads.front(), y->gterm());
    }
    void depends(Dependency<S,S>::Node &x, unsigned num, UTerm &&y, bool positive = true) {
        occs.emplace_front(num, std::move(y), positive);
        dep.depends(x, occs.front());
    }
    S analyze() {
        auto sccs(dep.analyze());
        for (auto &component : sccs) { std::sort(component.first.begin(), component.first.end()); }
        std::ostringstream ss;
        ss << "(" << sccs << ",[";
        print_comma(ss, occs, ",", [](std::ostream &out, TestBodyOccurrence const &x) { x.print(out); });
        ss << "])";
        return ss.str();
    }

    std::forward_list<S> heads;
    std::forward_list<TestBodyOccurrence> occs;
    Dependency<S,S> dep;
};

} // namespace

// }}}
// {{{ definition of TestDependency

void TestDependency::setUp() {
}

void TestDependency::tearDown() {
}

void TestDependency::test_match() {
    TestLookup l;
    l.add(fun("f", val(NUM(2))));
    CPPUNIT_ASSERT_EQUAL(S("{f(2)}"), l.match(V::createFun("f", { NUM(2) })));
    l.add(fun("f", var("X")));
    CPPUNIT_ASSERT_EQUAL(S("{f(Y0)}"), l.match(V::createFun("f", { NUM(1) })));
    CPPUNIT_ASSERT_EQUAL(S("{}"), l.match(V::createFun("g", { NUM(1) })));
    l.add(fun("f", var("X"), var("X")));
    l.add(fun("f", var("X"), var("Y")));
    CPPUNIT_ASSERT_EQUAL(S("{f(Y0,Y0),f(Y0,Y1)}"), l.match(V::createFun("f", { NUM(1), NUM(1) })));
    CPPUNIT_ASSERT_EQUAL(S("{f(Y0,Y1)}"), l.match(V::createFun("f", { NUM(1), NUM(2) })));
    l.add(fun("g", lin("X", 3, 7)));
    CPPUNIT_ASSERT_EQUAL(S("{}"), l.match(V::createFun("g", { NUM(2) })));
    CPPUNIT_ASSERT_EQUAL(S("{g((3*Y0+7))}"), l.match(V::createFun("g", { NUM(10) })));
}

void TestDependency::test_unify() {
    TestLookup l;
    l.add(fun("f", var("X")));
    l.add(fun("f", fun("g", var("X"))));
    CPPUNIT_ASSERT_EQUAL(S("{f(Y0),f(g(Y0))}"), l.unify(fun("f", var("Y"))));
    CPPUNIT_ASSERT_EQUAL(S("{f(Y0),f(g(Y0))}"), l.unify(fun("f", fun("g", var("X")))));
    l.add(fun("f", var("X"), var("X")));
    CPPUNIT_ASSERT_EQUAL(S("{}"), l.unify(fun("f", var("A"), fun("f", var("A")))));
    CPPUNIT_ASSERT_EQUAL(S("{f(Y0,Y0)}"), l.unify(fun("f", var("A"), fun("f", var("B")))));
    l.add(fun("f", val(V::createFun("g", { NUM(1) })), var("X")));
    CPPUNIT_ASSERT_EQUAL(S("{f(Y0,Y0),f(g(1),Y0)}"), l.unify(fun("f", var("A"), val(V::createFun("g", { NUM(2) })))));
}

void TestDependency::test_dep1() {
    TestDep dep;
    auto &x(dep.add("x.", true));
    dep.provides(x, val(ID("x")));
    auto &y(dep.add("a:-b@1,x@1,y@1.", true));
    dep.provides(y, val(ID("a")));
    dep.depends(y, 1, val(ID("b")));
    dep.depends(y, 1, val(ID("x")));
    dep.depends(y, 1, val(ID("y")));
    auto &z(dep.add("b:-a@1.", true));
    dep.provides(z, val(ID("b")));
    dep.depends(z, 1, val(ID("a")));
    auto &u(dep.add("c:-a@2,b@2.", true));
    dep.provides(u, val(ID("c")));
    dep.depends(u, 2, val(ID("a")));
    dep.depends(u, 2, val(ID("b")));
	CPPUNIT_ASSERT_EQUAL(S("([([x.],1),([a:-b@1,x@1,y@1.,b:-a@1.],1),([c:-a@2,b@2.],1)],[b@2:!,a@2:!,a@1:?,y@1:!,x@1:!,b@1:?])"), dep.analyze());
}

void TestDependency::test_dep2() {
    TestDep dep;
    auto &x(dep.add("a:-~b@1.", true));
    dep.provides(x, val(ID("a")));
    dep.depends(x, 1, val(ID("b")), false);
    auto &y(dep.add("b:-~a@1,a@2.", true));
    dep.provides(y, val(ID("b")));
    dep.depends(y, 1, val(ID("a")), false);
    dep.depends(y, 2, val(ID("a")));
    CPPUNIT_ASSERT_EQUAL(S("([([a:-~b@1.],0),([b:-~a@1,a@2.],0)],[a@2:.,a@1:.,b@1:?])"), dep.analyze());
}

void TestDependency::test_dep3() {
    TestDep dep;
    auto &x(dep.add("{a}.", false));
    dep.provides(x, val(ID("a")));
    auto &y(dep.add("b:-a@1.", true));
    dep.provides(y, val(ID("b")));
    dep.depends(y, 1, val(ID("a")));
    CPPUNIT_ASSERT_EQUAL(S("([([{a}.],0),([b:-a@1.],0)],[a@1:.])"), dep.analyze());
}

void TestDependency::test_dep4() {
    TestDep dep;
    auto &x(dep.add("a:-~b@1.", true));
    dep.provides(x, val(ID("a")));
    dep.depends(x, 1, val(ID("b")), false);
    auto &y(dep.add("a:-~c@1.", true));
    dep.provides(y, val(ID("a")));
    dep.depends(y, 1, val(ID("c")), false);

    auto &b(dep.add("b:-~a@1.", true));
    dep.provides(b, val(ID("b")));
    dep.depends(b, 1, val(ID("a")));
    auto &c(dep.add("c:-~a@2.", true));
    dep.provides(c, val(ID("c")));
    dep.depends(c, 2, val(ID("a")));
    CPPUNIT_ASSERT_EQUAL(S("([([a:-~b@1.],0),([a:-~c@1.],0),([b:-~a@1.],0),([c:-~a@2.],0)],[a@2:.,a@1:.,c@1:?,b@1:?])"), dep.analyze());
}

TestDependency::~TestDependency() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestDependency);

} } } // namespace Test Ground Gringo

