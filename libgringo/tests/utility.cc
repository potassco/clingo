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

#include "gringo/utility.hh"
#include "tests/tests.hh"

namespace Gringo { namespace Test {

// {{{ declaration of TestUtility

class TestUtility : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestUtility);
        CPPUNIT_TEST(test_to_string);
        CPPUNIT_TEST(test_cross_product);
        CPPUNIT_TEST(test_clone);
        CPPUNIT_TEST(test_hash);
        CPPUNIT_TEST(test_equal_to);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

    void test_to_string();
    void test_cross_product();
    void test_clone();
    void test_hash();
    void test_equal_to();

    virtual ~TestUtility();
};

// }}}

using Gringo::IO::to_string;

// {{{ declaration of helper classes

namespace {
    struct CloneMe {
        CloneMe(int x)
            : x(x) { }
        CloneMe(CloneMe const &) = delete;
        CloneMe(CloneMe &&) = default;
        CloneMe *clone() const {
            return new CloneMe(x);
        }
        int x;
    };

    std::ostream &operator<<(std::ostream &out, CloneMe const &x) {
        out << "c" << x.x;
        return out;
    }
}

// }}}
// {{{ definition of TestUtility

void TestUtility::setUp() { }

void TestUtility::tearDown() {
}

void TestUtility::test_to_string() {
    int x1 { 1 };
    std::unique_ptr<int> x2 { new int(1) };
    std::pair<int, int> x3 { 1, 2 };
    std::tuple<> x4 { };
    std::tuple<int> x5 { 1 };
    std::tuple<int,int,int> x6 { 1, 2, 3 };
    std::vector<int> x7 { 1, 2, 3 };
    std::vector<std::vector<int>> x8 { {1,2}, {3,4}, {5,6,7}, {8} };

    CPPUNIT_ASSERT_EQUAL(std::string("1"), to_string(x1));
    CPPUNIT_ASSERT_EQUAL(std::string("1"), to_string(x2));
    CPPUNIT_ASSERT_EQUAL(std::string("(1,2)"), to_string(x3));
    CPPUNIT_ASSERT_EQUAL(std::string("()"), to_string(x4));
    CPPUNIT_ASSERT_EQUAL(std::string("(1)"), to_string(x5));
    CPPUNIT_ASSERT_EQUAL(std::string("(1,2,3)"), to_string(x6));
    CPPUNIT_ASSERT_EQUAL(std::string("[1,2,3]"), to_string(x7));
    CPPUNIT_ASSERT_EQUAL(std::string("[[1,2],[3,4],[5,6,7],[8]]"), to_string(x8));
}

void TestUtility::test_cross_product() {
    std::vector<std::vector<int>> x1 { };
    std::vector<std::vector<int>> x2 { {} };
    std::vector<std::vector<int>> x3 { {1} };
    std::vector<std::vector<int>> x4 { {1,2} };
    std::vector<std::vector<int>> x5 { {1,2}, {3} };
    std::vector<std::vector<int>> x6 { {1,2}, {3,4} };
    std::vector<std::vector<int>> x7 { {1,2}, {3,4}, {} };
    std::vector<std::vector<int>> x8 { {0,1}, {0,1}, {0,1} };
    std::vector<std::vector<int>> x9 { {1,2}, {3,4}, {5,6,7}, {8} };
    cross_product(x1);
    cross_product(x2);
    cross_product(x3);
    cross_product(x4);
    cross_product(x5);
    cross_product(x6);
    cross_product(x7);
    cross_product(x8);
    cross_product(x9);
    CPPUNIT_ASSERT_EQUAL(std::string("[[]]"), to_string(x1));
    CPPUNIT_ASSERT_EQUAL(std::string("[]"), to_string(x2));
    CPPUNIT_ASSERT_EQUAL(std::string("[[1]]"), to_string(x3));
    CPPUNIT_ASSERT_EQUAL(std::string("[[1],[2]]"), to_string(x4));
    CPPUNIT_ASSERT_EQUAL(std::string("[[1,3],[2,3]]"), to_string(x5));
    CPPUNIT_ASSERT_EQUAL(std::string("[[1,3],[2,3],[1,4],[2,4]]"), to_string(x6));
    CPPUNIT_ASSERT_EQUAL(std::string("[]"), to_string(x7));
    CPPUNIT_ASSERT_EQUAL(std::string("[[0,0,0],[1,0,0],[0,1,0],[1,1,0],[0,0,1],[1,0,1],[0,1,1],[1,1,1]]"), to_string(x8));
    CPPUNIT_ASSERT_EQUAL(std::string("[[1,3,5,8],[2,3,5,8],[1,4,5,8],[2,4,5,8],[1,3,6,8],[2,3,6,8],[1,4,6,8],[2,4,6,8],[1,3,7,8],[2,3,7,8],[1,4,7,8],[2,4,7,8]]"), to_string(x9));
}

void TestUtility::test_clone() {
    typedef std::unique_ptr<CloneMe> U;
    CloneMe x1 { 1 };
    auto x2(gringo_make_unique<CloneMe>(1));
    auto x3(std::make_pair(gringo_make_unique<CloneMe>(1), gringo_make_unique<CloneMe>(2)));
    std::vector<U> x4;
    x4.emplace_back(gringo_make_unique<CloneMe>(1));
    x4.emplace_back(gringo_make_unique<CloneMe>(2));
    x4.emplace_back(gringo_make_unique<CloneMe>(3));
    std::tuple<U, U, U> x5 { gringo_make_unique<CloneMe>(1), gringo_make_unique<CloneMe>(2), gringo_make_unique<CloneMe>(3) };
    std::tuple<> x6 { };
    CPPUNIT_ASSERT_EQUAL(to_string(1), to_string(get_clone(1)));
    CPPUNIT_ASSERT_EQUAL(to_string(x1), to_string(U(get_clone(&x1))));
    CPPUNIT_ASSERT_EQUAL(to_string(x2), to_string(get_clone(x2)));
    CPPUNIT_ASSERT_EQUAL(to_string(x3), to_string(get_clone(x3)));
    CPPUNIT_ASSERT_EQUAL(to_string(x4), to_string(get_clone(x4)));
    CPPUNIT_ASSERT_EQUAL(to_string(x5), to_string(get_clone(x5)));
    CPPUNIT_ASSERT_EQUAL(to_string(x6), to_string(get_clone(x6)));
}

void TestUtility::test_hash() {
    std::vector<int> x1 { };
    std::vector<int> x1e { };
    std::vector<int> x2 { 1, 2, 3 };
    std::vector<int> x2e { 1, 2, 3 };
    std::vector<int> x3 { 1, 2, 3, 4 };
    std::vector<int> x3e { 1, 2, 3, 4 };
    std::pair<int, int> x4 { 1, 2 };
    std::pair<int, int> x4e { 1, 2 };
    std::pair<int, int> x5 { 2, 3 };
    std::pair<int, int> x5e { 2, 3 };
    std::tuple<> x6 { };
    std::tuple<> x6e { };
    std::tuple<int, int, int> x7 { 1, 2, 4 };
    std::tuple<int, int, int> x7e { 1, 2, 4 };
    std::tuple<int, int, int> x8 { 1, 2, 3 };
    std::tuple<int, int, int> x8e { 1, 2, 3 };
    std::unique_ptr<int> x9 { new int(1) };
    std::unique_ptr<int> x9e { new int(1) };
    std::unique_ptr<int> x10 { new int(2) };
    std::unique_ptr<int> x10e { new int(2) };
    std::tuple<std::pair<std::vector<int>, std::unique_ptr<int>>> x11 { std::make_pair(std::vector<int> {1, 2, 3}, gringo_make_unique<int>(4)) };
    std::tuple<std::pair<std::vector<int>, std::unique_ptr<int>>> x11e { std::make_pair(std::vector<int> {1, 2, 3}, gringo_make_unique<int>(4)) };
    std::tuple<std::pair<std::vector<int>, std::unique_ptr<int>>> x12 { std::make_pair(std::vector<int> {1, 2, 4}, gringo_make_unique<int>(4)) };


    CPPUNIT_ASSERT_EQUAL(get_value_hash(1), get_value_hash(1));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x1), get_value_hash(x1e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x2), get_value_hash(x2e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x3), get_value_hash(x3e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x4), get_value_hash(x4e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x5), get_value_hash(x5e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x6), get_value_hash(x6e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x7), get_value_hash(x7e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x8), get_value_hash(x8e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x9), get_value_hash(x9e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x10), get_value_hash(x10e));
    CPPUNIT_ASSERT_EQUAL(get_value_hash(x11), get_value_hash(x11e));

    char const *msg = "warning: hashes are very unlikely to collide";
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x1) != get_value_hash(x2));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x1) != get_value_hash(x3));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x2) != get_value_hash(x3));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x4) != get_value_hash(x5));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x6) != get_value_hash(x7));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x6) != get_value_hash(x8));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x7) != get_value_hash(x8));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x9) != get_value_hash(x10));
    CPPUNIT_ASSERT_MESSAGE(msg, get_value_hash(x11) != get_value_hash(x12));
}

void TestUtility::test_equal_to() {
    std::vector<int> x1 { };
    std::vector<int> x1e { };
    std::vector<int> x2 { 1, 2, 3 };
    std::vector<int> x2e { 1, 2, 3 };
    std::vector<int> x3 { 1, 2, 3, 4 };
    std::vector<int> x3e { 1, 2, 3, 4 };
    std::pair<int, int> x4 { 1, 2 };
    std::pair<int, int> x4e { 1, 2 };
    std::pair<int, int> x5 { 2, 3 };
    std::pair<int, int> x5e { 2, 3 };
    std::tuple<> x6 { };
    std::tuple<> x6e { };
    std::tuple<int, int, int> x7 { 1, 2, 4 };
    std::tuple<int, int, int> x7e { 1, 2, 4 };
    std::tuple<int, int, int> x8 { 1, 2, 3 };
    std::tuple<int, int, int> x8e { 1, 2, 3 };
    std::unique_ptr<int> x9 { new int(1) };
    std::unique_ptr<int> x9e { new int(1) };
    std::unique_ptr<int> x10 { new int(2) };
    std::unique_ptr<int> x10e { new int(2) };
    std::tuple<std::pair<std::vector<int>, std::unique_ptr<int>>> x11 { std::make_pair(std::vector<int> {1, 2, 3}, gringo_make_unique<int>(4)) };
    std::tuple<std::pair<std::vector<int>, std::unique_ptr<int>>> x11e { std::make_pair(std::vector<int> {1, 2, 3}, gringo_make_unique<int>(4)) };
    std::tuple<std::pair<std::vector<int>, std::unique_ptr<int>>> x12 { std::make_pair(std::vector<int> {1, 2, 4}, gringo_make_unique<int>(4)) };

    CPPUNIT_ASSERT(is_value_equal_to(x1, x1e));
    CPPUNIT_ASSERT(is_value_equal_to(x2, x2e));
    CPPUNIT_ASSERT(is_value_equal_to(x3, x3e));
    CPPUNIT_ASSERT(is_value_equal_to(x4, x4e));
    CPPUNIT_ASSERT(is_value_equal_to(x5, x5e));
    CPPUNIT_ASSERT(is_value_equal_to(x6, x6e));
    CPPUNIT_ASSERT(is_value_equal_to(x7, x7e));
    CPPUNIT_ASSERT(is_value_equal_to(x8, x8e));
    CPPUNIT_ASSERT(is_value_equal_to(x9, x9e));
    CPPUNIT_ASSERT(is_value_equal_to(x10, x10e));
    CPPUNIT_ASSERT(is_value_equal_to(x11, x11e));

    CPPUNIT_ASSERT(!is_value_equal_to(x1, x2));
    CPPUNIT_ASSERT(!is_value_equal_to(x1, x3));
    CPPUNIT_ASSERT(!is_value_equal_to(x2, x3));
    CPPUNIT_ASSERT(!is_value_equal_to(x4, x5));
    CPPUNIT_ASSERT(!is_value_equal_to(x7, x8));
    CPPUNIT_ASSERT(!is_value_equal_to(x9, x10));
    CPPUNIT_ASSERT(!is_value_equal_to(x11, x12));
}

TestUtility::~TestUtility() { }

// }}}

CPPUNIT_TEST_SUITE_REGISTRATION(TestUtility);

} } // namespace Test Gringo

