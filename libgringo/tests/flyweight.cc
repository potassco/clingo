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

#include "gringo/flyweight.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <climits>
#include <sstream>

namespace Gringo { namespace Test {

// {{{ declaration of TestFlyweight

class TestFlyweight : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestFlyweight);
        CPPUNIT_TEST(test_uid);
        CPPUNIT_TEST(test_erase_uid);
        CPPUNIT_TEST(test_offset);
        CPPUNIT_TEST(test_erase_offset);
    CPPUNIT_TEST_SUITE_END();

public:
    struct Dummy;

    typedef Flyweight<Dummy> FWDummy;
    typedef FlyweightVec<std::string> FWStringVec;

    virtual void setUp();
    virtual void tearDown();

    void test_uid();
    void test_uid_value();
    void test_erase_uid();
    void test_offset();
    void test_erase_offset();

    virtual ~TestFlyweight();
};

// }}}
// {{{ declaration of TestFlyweight::Dummy

struct TestFlyweight::Dummy {
    Dummy(std::string const &name);
    Dummy(Dummy &&other);
    Dummy &operator=(Dummy &&other);
    bool operator==(Dummy const &other) const;
    std::string name;
};

std::ostream &operator<<(std::ostream &out, TestFlyweight::Dummy const &dummy);

// }}}

} } // namespace Test Gringo

namespace std {

// {{{ declaration of hash<TestFlyweight>

template <>
struct hash<Gringo::Test::TestFlyweight::Dummy> {
    size_t operator()(Gringo::Test::TestFlyweight::Dummy const &dummy) const;
};

// }}}

} // namespace std

namespace Gringo { namespace Test {

// {{{ defintion of TestFlyweight::Dummy

TestFlyweight::Dummy::Dummy(std::string const &name)
    : name(name) { }

TestFlyweight::Dummy::Dummy(Dummy &&other)
    : name(std::move(other.name)) { }

TestFlyweight::Dummy &TestFlyweight::Dummy::operator=(Dummy &&other) {
    name = std::move(other.name);
    return *this;
}

bool TestFlyweight::Dummy::operator==(Dummy const &other) const {
    return name == other.name;
}

// }}}
// {{{ defintion of TestFlyweight

void TestFlyweight::setUp() { }

void TestFlyweight::tearDown() {
    FWDummy::clear();
    FWStringVec::clear();
}

void TestFlyweight::test_uid() {
    FWDummy::clear();

    FWDummy a("a");
    FWDummy aa("a");
    FWDummy b("b");

    CPPUNIT_ASSERT_EQUAL(0u, a.uid());
    CPPUNIT_ASSERT_EQUAL(1u, b.uid());
    CPPUNIT_ASSERT_EQUAL(a.uid(), aa.uid());
    CPPUNIT_ASSERT(a.uid() != b.uid());

    CPPUNIT_ASSERT_EQUAL(std::string("a"), (*a).name);
    CPPUNIT_ASSERT_EQUAL(std::string("a"), (*aa).name);
    CPPUNIT_ASSERT_EQUAL(std::string("b"), (*b).name);
}

void TestFlyweight::test_erase_uid() {
    // NOTE: erasing in flyweights will probably never be used in real code
    //       given the current implementation visiting the datastructure
    //       and copying all flyweights used is probably a better idea
    //       the tests are just kept for now
    FWDummy::clear();

    FWDummy a("a");
    FWDummy b("b");

    CPPUNIT_ASSERT_EQUAL(0u, a.uid());
    CPPUNIT_ASSERT_EQUAL(1u, b.uid());

    FWDummy::erase([](Dummy const &d) { return d.name == "b"; });
    FWDummy c("c");
    CPPUNIT_ASSERT_EQUAL(1u, c.uid());

    FWDummy bb("b");

    CPPUNIT_ASSERT_EQUAL(2u, bb.uid());

    FWDummy::erase([](Dummy const &d) { return d.name == "a" || d.name == "c"; });

    FWDummy x("x");
    FWDummy y("y");
    FWDummy z("z");

    CPPUNIT_ASSERT_EQUAL(1u, x.uid());
    CPPUNIT_ASSERT_EQUAL(2u, y.uid());
    CPPUNIT_ASSERT_EQUAL(3u, z.uid());
}

void TestFlyweight::test_offset() {
    FWStringVec::clear();
    FWStringVec a = {"a", "b", "c"};
    FWStringVec b = {"a", "b"};
    FWStringVec c = {"a", "b", "c"};
    FWStringVec d = {"a", "b", "d"};

    CPPUNIT_ASSERT_EQUAL(3u, a.size());
    CPPUNIT_ASSERT_EQUAL(0u, a.offset());
    CPPUNIT_ASSERT_EQUAL(std::string("a"), a[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("b"), a[1]);
    CPPUNIT_ASSERT_EQUAL(std::string("c"), a[2]);

    CPPUNIT_ASSERT_EQUAL(2u, b.size());
    CPPUNIT_ASSERT_EQUAL(0u, b.offset());
    CPPUNIT_ASSERT_EQUAL(std::string("a"), b[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("b"), b[1]);

    CPPUNIT_ASSERT_EQUAL(3u, c.size());
    CPPUNIT_ASSERT_EQUAL(0u, c.offset());
    CPPUNIT_ASSERT_EQUAL(1u, d.offset());
}

template <class It, class Jt>
bool range_equal(It it, It ie, Jt jt, Jt je) {
    for (; it != ie && jt != je; ++it, ++jt) {
        if (!(*it == *jt)) { return false; }
    }
    return it == ie && jt == je;
}

void TestFlyweight::test_erase_offset() {
    // NOTE: erasing in flyweights will probably never be used in real code
    //       the tests are just kept for now
    FWStringVec::clear();

    FWStringVec a{"a", "b", "c"};
    FWStringVec b{"a", "b"};
    FWStringVec d{"a", "b", "d"};
    CPPUNIT_ASSERT_EQUAL(0u, a.offset());
    CPPUNIT_ASSERT_EQUAL(0u, b.offset());
    CPPUNIT_ASSERT_EQUAL(1u, d.offset());

    FWStringVec::erase([](FWStringVec::ConstIterator it, FWStringVec::ConstIterator ie) {
        auto ab = {"a", "b"};
        return range_equal(it, ie, ab.begin(), ab.end());
    });
    FWStringVec c{"x", "y"};
    CPPUNIT_ASSERT_EQUAL(0u, c.offset());

    FWStringVec cc{"x", "y"};

    CPPUNIT_ASSERT_EQUAL(0u, cc.offset());

    FWStringVec::erase([](FWStringVec::ConstIterator it, FWStringVec::ConstIterator ie) {
        auto xy = {"x", "y"};
        auto abc = {"a", "b", "c"};
        return range_equal(it, ie, xy.begin(), xy.end()) || range_equal(it, ie, abc.begin(), abc.end());
    });

    FWStringVec x{"x", "y"};
    FWStringVec y{"x", "y", "z"};

    CPPUNIT_ASSERT_EQUAL(0u, x.offset());
    CPPUNIT_ASSERT_EQUAL(0u, FWStringVec({"a", "b", "d"}).offset());
    CPPUNIT_ASSERT_EQUAL(1u, y.offset());
}

TestFlyweight::~TestFlyweight() { }

std::ostream &operator<<(std::ostream &out, TestFlyweight::Dummy const &dummy) {
    out << dummy.name;
    return out;
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestFlyweight);

// }}}

} } // namespace Test Gringo

namespace std {

// {{{ defintion of std::hash<TestFlyweight::Dummy>

size_t hash<Gringo::Test::TestFlyweight::Dummy>::operator()(Gringo::Test::TestFlyweight::Dummy const &dummy) const {
    return std::hash<std::string>()(dummy.name);
}

// }}}

} // namespace std

