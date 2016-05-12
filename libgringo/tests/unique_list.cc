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

#include "gringo/unique_list.hh"
#include "gringo/hash_set.hh"

#include "tests/tests.hh"

namespace Gringo { namespace Test {

// {{{ declaration of TestUniqueList

class TestUniqueList : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestUniqueList);
        CPPUNIT_TEST(test_emplace);
        CPPUNIT_TEST(test_push);
        CPPUNIT_TEST(test_erase);
    CPPUNIT_TEST_SUITE_END();

public:
    using S = std::string;

    virtual void setUp();
    virtual void tearDown();

    S str(unique_list<std::pair<int, int>, extract_first<int>> const &x) {
        auto f = [](std::ostream &out, unique_list<std::pair<int, int>, extract_first<int>>::value_type const &x) { out << "(" << x.first << "," << x.second << ")"; };
        std::ostringstream oss;
        print_comma(oss, x, ",", f);
        return oss.str();
    }
    void test_emplace();
    void test_push();
    void test_erase();

    virtual ~TestUniqueList();
};

// }}}

// {{{ definition of TestUniqueList

void TestUniqueList::setUp() {
}

void TestUniqueList::tearDown() {
}

void TestUniqueList::test_emplace() {
    unique_list<std::pair<int, int>, extract_first<int>> x;
    x.emplace_back(1,1);
    x.emplace_back(2,2);
    x.emplace_back(1,3);
    x.emplace_back(4,4);
    x.emplace_back(5,5);
    x.emplace_back(1,6);
    x.emplace_back(4,7);
    x.emplace_back(1,8);
    CPPUNIT_ASSERT_EQUAL(S("(1,1),(2,2),(4,4),(5,5)"), str(x));
}

void TestUniqueList::test_push() {
    UniqueVec<unsigned> vec;
    CPPUNIT_ASSERT( vec.push(1).second);
    CPPUNIT_ASSERT( vec.push(2).second);
    CPPUNIT_ASSERT( vec.push(3).second);
    CPPUNIT_ASSERT( vec.push(4).second);
    vec.pop();
    vec.pop();
    CPPUNIT_ASSERT(!vec.push(1).second);
    CPPUNIT_ASSERT(!vec.push(2).second);
    CPPUNIT_ASSERT( vec.push(3).second);
    CPPUNIT_ASSERT( vec.push(4).second);
    CPPUNIT_ASSERT( vec.push(5).second);
    CPPUNIT_ASSERT( vec.push(6).second);
    CPPUNIT_ASSERT( vec.push(7).second);
    vec.erase([](unsigned val) { return 2 < val && val < 6; });
    CPPUNIT_ASSERT(!vec.push(1).second);
    CPPUNIT_ASSERT(!vec.push(2).second);
    CPPUNIT_ASSERT( vec.push(3).second);
    CPPUNIT_ASSERT( vec.push(4).second);
    CPPUNIT_ASSERT( vec.push(5).second);
    CPPUNIT_ASSERT(!vec.push(6).second);
    CPPUNIT_ASSERT(!vec.push(7).second);
}

void TestUniqueList::test_erase() {
    unique_list<std::pair<int, int>, extract_first<int>> x;
    for (int i = 0; i < 10; i++) { x.emplace_back(i, i); }
    x.erase(1);
    x.erase(0);
    x.erase(5);
    x.erase(9);
    x.erase(10);
    CPPUNIT_ASSERT_EQUAL(S("(2,2),(3,3),(4,4),(6,6),(7,7),(8,8)"), str(x));
    x.erase(x.find(4), x.find(7));
    CPPUNIT_ASSERT_EQUAL(S("(2,2),(3,3),(7,7),(8,8)"), str(x));
    x.erase(x.begin(), x.find(7));
    CPPUNIT_ASSERT_EQUAL(S("(7,7),(8,8)"), str(x));
    x.emplace_back(9,9);
    x.erase(x.find(7), x.find(9));
    CPPUNIT_ASSERT_EQUAL(S("(9,9)"), str(x));
    x.erase(x.begin(), x.end());
    CPPUNIT_ASSERT_EQUAL(S(""), str(x));
}

TestUniqueList::~TestUniqueList() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestUniqueList);

} } // namespace Test Gringo


