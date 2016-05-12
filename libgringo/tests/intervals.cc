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

#include "gringo/intervals.hh"
#include "gringo/utility.hh"

#include "tests/tests.hh"

namespace Gringo { namespace Test {

// {{{ declaration of TestInterval

class TestInterval : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestInterval);
        CPPUNIT_TEST(test_boundLess);
        CPPUNIT_TEST(test_boundLessEq);
        CPPUNIT_TEST(test_boundBefore);
        CPPUNIT_TEST(test_add);
        CPPUNIT_TEST(test_remove);
        CPPUNIT_TEST(test_contains);
        CPPUNIT_TEST(test_intersects);
        CPPUNIT_TEST(test_intersect);
        CPPUNIT_TEST(test_difference);
        CPPUNIT_TEST(test_enum_empty);
        CPPUNIT_TEST(test_enum_add);
        CPPUNIT_TEST(test_enum_contains);
        CPPUNIT_TEST(test_enum_remove);
        CPPUNIT_TEST(test_enum_intersects);
    CPPUNIT_TEST_SUITE_END();
    using IS = IntervalSet<int>;
    using ES = enum_interval_set<int>;
    using LB = IS::LBound;
    using RB = IS::RBound;
    using IV = IS::Interval;
    using S = std::string;

public:
    virtual void setUp();
    virtual void tearDown();

    LB lb(int x, bool y) { return {x, y}; };
    RB rb(int x, bool y) { return {x, y}; };
    std::string print(IS const &x);
    std::string print(ES const &x);

    void test_boundLess();
    void test_boundLessEq();
    void test_boundBefore();
    void test_add();
    void test_remove();
    void test_contains();
    void test_intersects();
    void test_intersect();
    void test_difference();
    void test_enum_empty();
    void test_enum_add();
    void test_enum_contains();
    void test_enum_remove();
    void test_enum_intersects();

    virtual ~TestInterval();
};

// }}}


// {{{ definition of TestInterval

void TestInterval::setUp() {
}

void TestInterval::tearDown() {
}

void TestInterval::test_boundLess() {
    CPPUNIT_ASSERT(!(lb(2,true)  < lb(1,true)));
    CPPUNIT_ASSERT(!(lb(2,true)  < lb(1,false)));
    CPPUNIT_ASSERT(!(lb(2,false) < lb(1,true)));
    CPPUNIT_ASSERT(!(lb(2,false) < lb(1,false)));

    CPPUNIT_ASSERT( (lb(1,true)  < lb(2,true)));
    CPPUNIT_ASSERT( (lb(1,true)  < lb(2,false)));
    CPPUNIT_ASSERT( (lb(1,false) < lb(2,true)));
    CPPUNIT_ASSERT( (lb(1,false) < lb(2,false)));

    CPPUNIT_ASSERT(!(lb(1,true)  < lb(1,true)));
    CPPUNIT_ASSERT( (lb(1,true)  < lb(1,false)));
    CPPUNIT_ASSERT(!(lb(1,false) < lb(1,true)));
    CPPUNIT_ASSERT(!(lb(1,false) < lb(1,false)));
    // same for RB
    CPPUNIT_ASSERT(!(rb(2,true)  < rb(1,true)));
    CPPUNIT_ASSERT(!(rb(2,true)  < rb(1,false)));
    CPPUNIT_ASSERT(!(rb(2,false) < rb(1,true)));
    CPPUNIT_ASSERT(!(rb(2,false) < rb(1,false)));

    CPPUNIT_ASSERT( (rb(1,true)  < rb(2,true)));
    CPPUNIT_ASSERT( (rb(1,true)  < rb(2,false)));
    CPPUNIT_ASSERT( (rb(1,false) < rb(2,true)));
    CPPUNIT_ASSERT( (rb(1,false) < rb(2,false)));

    CPPUNIT_ASSERT(!(rb(1,true)  < rb(1,true)));
    CPPUNIT_ASSERT(!(rb(1,true)  < rb(1,false)));
    CPPUNIT_ASSERT( (rb(1,false) < rb(1,true)));
    CPPUNIT_ASSERT(!(rb(1,false) < rb(1,false)));
}

void TestInterval::test_boundLessEq() {
    CPPUNIT_ASSERT(!(lb(2,true)  <= lb(1,true)));
    CPPUNIT_ASSERT(!(lb(2,true)  <= lb(1,false)));
    CPPUNIT_ASSERT(!(lb(2,false) <= lb(1,true)));
    CPPUNIT_ASSERT(!(lb(2,false) <= lb(1,false)));

    CPPUNIT_ASSERT( (lb(1,true)  <= lb(2,true)));
    CPPUNIT_ASSERT( (lb(1,true)  <= lb(2,false)));
    CPPUNIT_ASSERT( (lb(1,false) <= lb(2,true)));
    CPPUNIT_ASSERT( (lb(1,false) <= lb(2,false)));

    CPPUNIT_ASSERT( (lb(1,true)  <= lb(1,true)));
    CPPUNIT_ASSERT( (lb(1,true)  <= lb(1,false)));
    CPPUNIT_ASSERT(!(lb(1,false) <= lb(1,true)));
    CPPUNIT_ASSERT( (lb(1,false) <= lb(1,false)));
    // same for RB
    CPPUNIT_ASSERT(!(rb(2,true)  <= rb(1,true)));
    CPPUNIT_ASSERT(!(rb(2,true)  <= rb(1,false)));
    CPPUNIT_ASSERT(!(rb(2,false) <= rb(1,true)));
    CPPUNIT_ASSERT(!(rb(2,false) <= rb(1,false)));

    CPPUNIT_ASSERT( (rb(1,true)  <= rb(2,true)));
    CPPUNIT_ASSERT( (rb(1,true)  <= rb(2,false)));
    CPPUNIT_ASSERT( (rb(1,false) <= rb(2,true)));
    CPPUNIT_ASSERT( (rb(1,false) <= rb(2,false)));

    CPPUNIT_ASSERT( (rb(1,true)  <= rb(1,true)));
    CPPUNIT_ASSERT(!(rb(1,true)  <= rb(1,false)));
    CPPUNIT_ASSERT( (rb(1,false) <= rb(1,true)));
    CPPUNIT_ASSERT( (rb(1,false) <= rb(1,false)));
}

void TestInterval::test_boundBefore() {
    // before with gap
    // {
    //  }
    CPPUNIT_ASSERT((lb(1,true)  < rb(2,true)));
    CPPUNIT_ASSERT((lb(1,true)  < rb(2,false)));
    CPPUNIT_ASSERT((lb(1,false) < rb(2,true)));
    CPPUNIT_ASSERT((lb(1,false) < rb(2,false)));
    // [        {           {
    // ] - gap  ) - no gap  ) - no gap
    CPPUNIT_ASSERT( (lb(1,true)  < rb(1,true)));
    CPPUNIT_ASSERT(!(lb(1,true)  < rb(1,false)));
    CPPUNIT_ASSERT(!(lb(1,false) < rb(1,true)));
    CPPUNIT_ASSERT(!(lb(1,false) < rb(1,false)));
    //  {
    // }
    CPPUNIT_ASSERT(!(lb(2,true)  < rb(1,true)));
    CPPUNIT_ASSERT(!(lb(2,true)  < rb(1,false)));
    CPPUNIT_ASSERT(!(lb(2,false) < rb(1,true)));
    CPPUNIT_ASSERT(!(lb(2,false) < rb(1,false)));
    // the other way round
    // }
    //  {
    CPPUNIT_ASSERT((rb(1,true)  < lb(2,true)));
    CPPUNIT_ASSERT((rb(1,true)  < lb(2,false)));
    CPPUNIT_ASSERT((rb(1,false) < lb(2,true)));
    CPPUNIT_ASSERT((rb(1,false) < lb(2,false)));
    // }           ]           )
    // [ - no gap  { - no gap  ( - gap
    CPPUNIT_ASSERT(!(rb(1,true)  < lb(1,true)));
    CPPUNIT_ASSERT(!(rb(1,true)  < lb(1,false)));
    CPPUNIT_ASSERT(!(rb(1,false) < lb(1,true)));
    CPPUNIT_ASSERT( (rb(1,false) < lb(1,false)));
    //  }
    // {
    CPPUNIT_ASSERT(!(rb(2,true)  < lb(1,true)));
    CPPUNIT_ASSERT(!(rb(2,true)  < lb(1,false)));
    CPPUNIT_ASSERT(!(rb(2,false) < lb(1,true)));
    CPPUNIT_ASSERT(!(rb(2,false) < lb(1,false)));
}

std::string TestInterval::print(IS const &x) {
    auto f = [](std::ostream &out, IV const &x) {
        out
            << (x.left.inclusive ? "[" : "(")
            << x.left.bound
            << ","
            << x.right.bound
            << (x.right.inclusive ? "]" : ")");
    };
    std::ostringstream out;
    out << "{";
    print_comma(out, x.vec, ",", f);
    out << "}";
    return out.str();
}

std::string TestInterval::print(ES const &x) {
    std::ostringstream out;
    out << "{";
    if (!x.empty()) {
        int a = *x.begin();
        int b = *x.begin() - 1;
        for (auto y : x) {
            if (b+1 != y) {
                if (a <= b) { out << "[" << a << "," << b << "],"; }
                a = y;
            }
            b = y;
        }
        out << "[" << a << "," << b << "]";
    }
    out << "}";
    return out.str();
}

void TestInterval::test_add() {
    IS x;
    x.add(1,true,1,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,1]}"), print(x));
    x.add(3,true,2,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,1]}"), print(x));
    x.add(3,true,4,false);
    CPPUNIT_ASSERT_EQUAL(S("{[1,1],[3,4)}"), print(x));
    x.add(2,false,3,false);
    CPPUNIT_ASSERT_EQUAL(S("{[1,1],(2,4)}"), print(x));
    x.add(1,false,2,false);
    CPPUNIT_ASSERT_EQUAL(S("{[1,2),(2,4)}"), print(x));
    x.add(2,true,2,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,4)}"), print(x));
    x.add(4,true,5,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5]}"), print(x));
    x.add(8,false,9,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],(8,9]}"), print(x));
    x.add(11,false,12,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],(8,9],(11,12]}"), print(x));
    x.add(13,false,14,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],(8,9],(11,12],(13,14]}"), print(x));
    x.add(10,true,11,false);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],(8,9],[10,11),(11,12],(13,14]}"), print(x));
    x.add(9,true,11,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],(8,12],(13,14]}"), print(x));
    x.add(0,true,13,false);
    CPPUNIT_ASSERT_EQUAL(S("{[0,13),(13,14]}"), print(x));
    x.add(-1,true,42,false);
    CPPUNIT_ASSERT_EQUAL(S("{[-1,42)}"), print(x));
    x.add(42,true,43,true);
    CPPUNIT_ASSERT_EQUAL(S("{[-1,43]}"), print(x));
}

void TestInterval::test_remove() {
    IS x;
    x.add(1,true,50,true);
    CPPUNIT_ASSERT_EQUAL(S("{[1,50]}"), print(x));
    x.remove(1,true,2,true);
    CPPUNIT_ASSERT_EQUAL(S("{(2,50]}"), print(x));
    x.remove(49,false,50,true);
    CPPUNIT_ASSERT_EQUAL(S("{(2,49]}"), print(x));
    x.remove(5,true,6,false);
    CPPUNIT_ASSERT_EQUAL(S("{(2,5),[6,49]}"), print(x));
    x.remove(8,false,9,true);
    CPPUNIT_ASSERT_EQUAL(S("{(2,5),[6,8],(9,49]}"), print(x));
    IS a(x), b(x), c(x);
    a.remove(2,false,13,true);
    CPPUNIT_ASSERT_EQUAL(S("{(13,49]}"), print(a));
    b.remove(4,false,8,true);
    CPPUNIT_ASSERT_EQUAL(S("{(2,4],(9,49]}"), print(b));
    c.remove(4,false,13,true);
    CPPUNIT_ASSERT_EQUAL(S("{(2,4],(13,49]}"), print(c));
}

void TestInterval::test_contains() {
    IS x;
    x.add(5,true,10,false);
    x.add(1,true,4,false);
    CPPUNIT_ASSERT( x.contains({{5,true},{10,false}}));
    CPPUNIT_ASSERT(!x.contains({{5,true},{10,true}}));
    CPPUNIT_ASSERT( x.contains({{7,true},{8,true}}));
    CPPUNIT_ASSERT( x.contains({{5,false},{6,false}}));
}

void TestInterval::test_intersects() {
    IS x;
    x.add(1,true,4,false);
    x.add(5,true,10,false);
    x.add(11,false,12,false);
    CPPUNIT_ASSERT( x.intersects({{5,true},{10,false}}));
    CPPUNIT_ASSERT( x.intersects({{5,true},{10,true}}));
    CPPUNIT_ASSERT( x.intersects({{7,true},{8,true}}));
    CPPUNIT_ASSERT(!x.intersects({{10,true},{11,true}}));
    CPPUNIT_ASSERT( x.intersects({{10,true},{12,true}}));
    CPPUNIT_ASSERT( x.intersects({{2,true},{7,true}}));
    CPPUNIT_ASSERT( x.intersects({{4,true},{7,true}}));
    CPPUNIT_ASSERT(!x.intersects({{0,true},{1,false}}));
    CPPUNIT_ASSERT( x.intersects({{0,true},{1,true}}));
    CPPUNIT_ASSERT(!x.intersects({{12,true},{13,true}}));
}

void TestInterval::test_intersect() {
    IS x, y;
    x.add({{2,true},{8,false}});
    x.add({{9,false},{13,true}});

    y.add({{1,true},{3,true}});
    y.add({{4,false},{5,true}});
    y.add({{7,true},{10,false}});
    y.add({{11,true},{12,false}});
    CPPUNIT_ASSERT_EQUAL(S("{[2,3],(4,5],[7,8),(9,10),[11,12)}"), print(x.intersect(y)));
    CPPUNIT_ASSERT_EQUAL(S("{[2,3],(4,5],[7,8),(9,10),[11,12)}"), print(y.intersect(x)));
}

void TestInterval::test_difference() {
    IS x, y;
    x.add({{2,true},{8,false}});
    x.add({{9,false},{13,true}});

    y.add({{1,true},{3,true}});
    y.add({{4,false},{5,true}});
    y.add({{7,true},{10,false}});
    y.add({{11,true},{12,false}});
    CPPUNIT_ASSERT_EQUAL(S("{(3,4],(5,7),[10,11),[12,13]}"), print(x.difference(y)));
    CPPUNIT_ASSERT_EQUAL(S("{[1,2),[8,9]}"), print(y.difference(x)));
}

void TestInterval::test_enum_empty() {
    ES x;
    CPPUNIT_ASSERT(x.empty());
    CPPUNIT_ASSERT(x.begin() == x.end());
}

void TestInterval::test_enum_add() {
    ES x;
    x.add(1,2);
    CPPUNIT_ASSERT_EQUAL(S("{[1,1]}"), print(x));
    x.add(3,3);
    CPPUNIT_ASSERT_EQUAL(S("{[1,1]}"), print(x));
    x.add(3,4);
    CPPUNIT_ASSERT_EQUAL(S("{[1,1],[3,3]}"), print(x));
    x.add(2,3);
    CPPUNIT_ASSERT_EQUAL(S("{[1,3]}"), print(x));
    x.add(4,6);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5]}"), print(x));
    x.add(9,10);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],[9,9]}"), print(x));
    x.add(12,13);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],[9,9],[12,12]}"), print(x));
    x.add(14,16);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],[9,9],[12,12],[14,15]}"), print(x));
    x.add(10,11);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],[9,10],[12,12],[14,15]}"), print(x));
    x.add(9,12);
    CPPUNIT_ASSERT_EQUAL(S("{[1,5],[9,12],[14,15]}"), print(x));
    x.add(0,14);
    CPPUNIT_ASSERT_EQUAL(S("{[0,15]}"), print(x));
    x.add(-1,42);
    CPPUNIT_ASSERT_EQUAL(S("{[-1,41]}"), print(x));
    x.add(42,44);
    CPPUNIT_ASSERT_EQUAL(S("{[-1,43]}"), print(x));
}

void TestInterval::test_enum_contains() {
    ES x;
    x.add(5,10);
    x.add(1,4);
    CPPUNIT_ASSERT( x.contains(5,10));
    CPPUNIT_ASSERT(!x.contains(5,11));
    CPPUNIT_ASSERT( x.contains(7,9));
    CPPUNIT_ASSERT( x.contains(6,6));
}

void TestInterval::test_enum_remove() {
    ES x;
    x.add(1,51);
    CPPUNIT_ASSERT_EQUAL(S("{[1,50]}"), print(x));
    x.remove(1,3);
    CPPUNIT_ASSERT_EQUAL(S("{[3,50]}"), print(x));
    x.remove(50,51);
    CPPUNIT_ASSERT_EQUAL(S("{[3,49]}"), print(x));
    x.remove(5,6);
    CPPUNIT_ASSERT_EQUAL(S("{[3,4],[6,49]}"), print(x));
    x.remove(9,10);
    CPPUNIT_ASSERT_EQUAL(S("{[3,4],[6,8],[10,49]}"), print(x));
    ES a(x), b(x), c(x);
    a.remove(3,14);
    CPPUNIT_ASSERT_EQUAL(S("{[14,49]}"), print(a));
    b.remove(5,9);
    CPPUNIT_ASSERT_EQUAL(S("{[3,4],[10,49]}"), print(b));
    c.remove(5,14);
    CPPUNIT_ASSERT_EQUAL(S("{[3,4],[14,49]}"), print(c));
}

void TestInterval::test_enum_intersects() {
    ES x;
    x.add(1,3);
    x.add(5,9);
    x.add(12,13);
    CPPUNIT_ASSERT( x.intersects(5,9));
    CPPUNIT_ASSERT( x.intersects(5,10));
    CPPUNIT_ASSERT( x.intersects(7,8));
    CPPUNIT_ASSERT(!x.intersects(10,12));
    CPPUNIT_ASSERT( x.intersects(10,13));
    CPPUNIT_ASSERT( x.intersects(2,7));
    CPPUNIT_ASSERT( x.intersects(4,7));
    CPPUNIT_ASSERT(!x.intersects(0,0));
    CPPUNIT_ASSERT( x.intersects(0,2));
    CPPUNIT_ASSERT(!x.intersects(13,14));
}

TestInterval::~TestInterval() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestInterval);

} } // namespace Test Gringo

