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

#include "gringo/value.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <climits>
#include <sstream>

namespace Gringo { namespace Test {

// {{{ declaration of TestValue

class TestValue : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestValue);
        CPPUNIT_TEST(test_types);
        CPPUNIT_TEST(test_values);
        CPPUNIT_TEST(test_cmp_num);
        CPPUNIT_TEST(test_cmp_type);
        CPPUNIT_TEST(test_cmp_other);
        CPPUNIT_TEST(test_print);
        CPPUNIT_TEST(test_sig);
    CPPUNIT_TEST_SUITE_END();
public:
    virtual void setUp();
    virtual void tearDown();

    void test_types();
    void test_values();
    void test_op();
    void test_cmp_num();
    void test_cmp_type();
    void test_cmp_other();
    void test_print(); 
    void test_sig(); 

    virtual ~TestValue();

    ValVec args;
    ValVec values;
};

// }}}

// {{{ definition of TestValue

void TestValue::setUp() {
    args = { Value::createNum(42), Value::createId("a") };
    values = {
        Value::createNum(INT_MIN), 
        Value::createNum(INT_MAX), 
        Value::createNum(0), 
        Value::createNum(42), 

        Value::createId("x"), 
        Value::createId("abc"), 

        Value::createStr(""), 
        Value::createStr("xyz"),

        Value::createInf(),
        Value::createSup(),

        Value::createTuple(args),
        Value::createFun(std::string("f"), args)
    };

}

void TestValue::tearDown() {
    FWString::clear();
    FWValVec::clear();
}

void TestValue::test_types() {
    CPPUNIT_ASSERT_EQUAL(Value::NUM, values[0].type());
    CPPUNIT_ASSERT_EQUAL(Value::NUM, values[1].type());
    CPPUNIT_ASSERT_EQUAL(Value::NUM, values[2].type());
    CPPUNIT_ASSERT_EQUAL(Value::NUM, values[3].type());

    CPPUNIT_ASSERT_EQUAL(Value::ID, values[4].type());
    CPPUNIT_ASSERT_EQUAL(Value::ID, values[5].type());

    CPPUNIT_ASSERT_EQUAL(Value::STRING, values[6].type());
    CPPUNIT_ASSERT_EQUAL(Value::STRING, values[7].type());

    CPPUNIT_ASSERT_EQUAL(Value::INF, values[8].type());
    CPPUNIT_ASSERT_EQUAL(values[9].type(), Value::SUP);

    CPPUNIT_ASSERT_EQUAL(Value::FUNC, values[11].type());
}

void TestValue::test_values() {
    CPPUNIT_ASSERT_EQUAL(INT_MIN, values[0].num());
    CPPUNIT_ASSERT_EQUAL(INT_MAX, values[1].num());
    CPPUNIT_ASSERT_EQUAL( 0, values[2].num());
    CPPUNIT_ASSERT_EQUAL(42, values[3].num());

    CPPUNIT_ASSERT_EQUAL(std::string("x"), *(values[4].string()));
    CPPUNIT_ASSERT_EQUAL(std::string("abc"), *(values[5].string()));

    CPPUNIT_ASSERT_EQUAL(std::string(""), *(values[6].string()));
    CPPUNIT_ASSERT_EQUAL(std::string("xyz"), *(values[7].string()));

    CPPUNIT_ASSERT_EQUAL(2u, values[10].args().size());
    CPPUNIT_ASSERT_EQUAL(42, values[10].args()[0].num());
    CPPUNIT_ASSERT_EQUAL(std::string("a"), *(values[10].args()[1].string()));

    CPPUNIT_ASSERT_EQUAL(std::string("f"), *(values[11].name()));
    CPPUNIT_ASSERT_EQUAL(42, values[11].args()[0].num());
    CPPUNIT_ASSERT_EQUAL(std::string("a"), *(values[11].args()[1].string()));
}

void TestValue::test_cmp_num() {
    CPPUNIT_ASSERT(!(Value::createNum(0) < Value::createNum(0)));
    CPPUNIT_ASSERT( (Value::createNum(0) < Value::createNum(1)));
    CPPUNIT_ASSERT(!(Value::createNum(1) < Value::createNum(0)));

    CPPUNIT_ASSERT(!(Value::createNum(0) > Value::createNum(0)));
    CPPUNIT_ASSERT(!(Value::createNum(0) > Value::createNum(1)));
    CPPUNIT_ASSERT( (Value::createNum(1) > Value::createNum(0)));

    CPPUNIT_ASSERT( (Value::createNum(0) <= Value::createNum(0)));
    CPPUNIT_ASSERT( (Value::createNum(0) <= Value::createNum(1)));
    CPPUNIT_ASSERT(!(Value::createNum(1) <= Value::createNum(0)));

    CPPUNIT_ASSERT( (Value::createNum(0) >= Value::createNum(0)));
    CPPUNIT_ASSERT(!(Value::createNum(0) >= Value::createNum(1)));
    CPPUNIT_ASSERT( (Value::createNum(1) >= Value::createNum(0)));

    CPPUNIT_ASSERT( (Value::createNum(0) == Value::createNum(0)));
    CPPUNIT_ASSERT(!(Value::createNum(0) == Value::createNum(1)));
    CPPUNIT_ASSERT(!(Value::createNum(1) == Value::createNum(0)));

    CPPUNIT_ASSERT(!(Value::createNum(0) != Value::createNum(0)));
    CPPUNIT_ASSERT( (Value::createNum(0) != Value::createNum(1)));
    CPPUNIT_ASSERT( (Value::createNum(1) != Value::createNum(0)));
}

void TestValue::test_cmp_type() {
    CPPUNIT_ASSERT(values[8] < values[0]);
    CPPUNIT_ASSERT(values[0] < values[4]);
    CPPUNIT_ASSERT(values[4] < values[6]);
    CPPUNIT_ASSERT(values[6] < values[10]);
    CPPUNIT_ASSERT(values[11] < values[9]);
}

void TestValue::test_cmp_other() {
    CPPUNIT_ASSERT(!(Value::createId("a") == Value::createId("a").flipSign()));
    CPPUNIT_ASSERT( (Value::createId("a") != Value::createId("a").flipSign()));

    CPPUNIT_ASSERT(!(Value::createId("a") < Value::createId("a")));
    CPPUNIT_ASSERT( (Value::createId("a") < Value::createId("a").flipSign()));
    CPPUNIT_ASSERT( (Value::createId("b") < Value::createId("a").flipSign()));
    CPPUNIT_ASSERT( (Value::createId("aaa") < Value::createId("aab")));
    CPPUNIT_ASSERT( (Value::createId("a") < Value::createId("aa")));
    CPPUNIT_ASSERT(!(Value::createId("aa") < Value::createId("a")));

    CPPUNIT_ASSERT(!(Value::createStr("a") < Value::createStr("a")));
    CPPUNIT_ASSERT( (Value::createStr("aaa") < Value::createStr("aab")));
    CPPUNIT_ASSERT( (Value::createStr("a") < Value::createStr("aa")));
    CPPUNIT_ASSERT(!(Value::createStr("aa") < Value::createStr("a")));

    Value a = Value::createTuple( ValVec{ Value::createNum(1), Value::createNum(1) } );
    Value b = Value::createTuple( ValVec{ Value::createNum(1) } );
    Value c = Value::createTuple( ValVec{ Value::createNum(1), Value::createNum(2) } );
    Value d = Value::createTuple( ValVec{ Value::createNum(2) } );
    CPPUNIT_ASSERT((b < a) && !(a < b));
    CPPUNIT_ASSERT((a < c) && !(c < a));
    CPPUNIT_ASSERT((b < d) && !(d < b));
    CPPUNIT_ASSERT((d < a) && !(a < d));

    Value fa = Value::createFun( "f", ValVec{ Value::createNum(1), Value::createNum(1) } );
    Value ga = Value::createFun( "g", ValVec{ Value::createNum(1), Value::createNum(1) } );
    Value fb = Value::createFun( "f", ValVec{ Value::createNum(1) } );
    Value fc = Value::createFun( "f", ValVec{ Value::createNum(1), Value::createNum(2) } );
    Value fd = Value::createFun( "f", ValVec{ Value::createNum(2) } );
    Value gd = Value::createFun( "g", ValVec{ Value::createNum(2) } );
    CPPUNIT_ASSERT((fb < fa) && !(fa < fb));
    CPPUNIT_ASSERT((fa < fc) && !(fc < fa));
    CPPUNIT_ASSERT((fb < fd) && !(fd < fb));
    CPPUNIT_ASSERT((fd < fa) && !(fa < fd));
    CPPUNIT_ASSERT((fa < ga) && !(ga < fa));
    CPPUNIT_ASSERT((gd < fa) && !(fa < gd));
    CPPUNIT_ASSERT((fa < fa.flipSign()) && !(fa.flipSign() < fa));
    CPPUNIT_ASSERT((fa < ga.flipSign()) && !(ga.flipSign() < fa));
}

void TestValue::test_print() {
    std::ostringstream oss;
    auto toString = [&oss](Value const &val) -> std::string { 
        oss << val;
        std::string str = oss.str();
        oss.str("");
        return str;

    };
    CPPUNIT_ASSERT_EQUAL(std::string("0"), toString(values[2]));
    CPPUNIT_ASSERT_EQUAL(std::string("42"), toString(values[3]));
    CPPUNIT_ASSERT_EQUAL(std::string("x"), toString(values[4]));
    CPPUNIT_ASSERT_EQUAL(std::string("-x"), toString(values[4].flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("x"), toString(values[4].flipSign().flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("abc"), toString(values[5]));
    CPPUNIT_ASSERT_EQUAL(std::string("\"\""), toString(values[6]));
    CPPUNIT_ASSERT_EQUAL(std::string("\"xyz\""), toString(values[7]));
    CPPUNIT_ASSERT_EQUAL(std::string("#inf"), toString(values[8]));
    CPPUNIT_ASSERT_EQUAL(std::string("#sup"), toString(values[9]));
    CPPUNIT_ASSERT_EQUAL(std::string("(42,a)"), toString(values[10]));
    CPPUNIT_ASSERT_EQUAL(std::string("f(42,a)"), toString(values[11]));
    CPPUNIT_ASSERT_EQUAL(std::string("-f(42,a)"), toString(values[11].flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("f(42,a)"), toString(values[11].flipSign().flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("()"), toString(Value::createTuple(ValVec{})));

    std::string comp = toString(Value::createFun(std::string("g"), ValVec(values.begin() + 2, values.end())));
    CPPUNIT_ASSERT_EQUAL(std::string("g(0,42,x,abc,\"\",\"xyz\",#inf,#sup,(42,a),f(42,a))"), comp);
}

void TestValue::test_sig() {
    std::vector<std::string> names { "a", "b", "c", "d" };
    for (unsigned i = 0; i < 42; i++) {
        auto name = names[i % names.size()];
        Signature sig{ name, i };
        CPPUNIT_ASSERT_EQUAL(name, *sig.name());
        CPPUNIT_ASSERT_EQUAL(i, sig.length());
    }
}

TestValue::~TestValue() { }

CPPUNIT_TEST_SUITE_REGISTRATION(TestValue);

// }}}

} } // namespace Test Gringo

