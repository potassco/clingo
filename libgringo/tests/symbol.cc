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

// {{{ declaration of TestSymbol

class TestSymbol : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestSymbol);
        CPPUNIT_TEST(test_types);
        CPPUNIT_TEST(test_symbols);
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
    void test_symbols();
    void test_op();
    void test_cmp_num();
    void test_cmp_type();
    void test_cmp_other();
    void test_print();
    void test_sig();

    virtual ~TestSymbol();

    SymVec args;
    SymVec symbols;
};

// }}}

// {{{ definition of TestSymbol

void TestSymbol::setUp() {
    args = { Symbol::createNum(42), Symbol::createId("a") };
    symbols = {
        Symbol::createNum(INT_MIN),
        Symbol::createNum(INT_MAX),
        Symbol::createNum(0),
        Symbol::createNum(42),

        Symbol::createId("x"),
        Symbol::createId("abc"),

        Symbol::createStr(""),
        Symbol::createStr("xyz"),

        Symbol::createInf(),
        Symbol::createSup(),

        Symbol::createTuple(Potassco::toSpan(args)),
        Symbol::createFun("f", Potassco::toSpan(args))
    };

}

void TestSymbol::tearDown() {
    FWString::clear();
    FWValVec::clear();
}

void TestSymbol::test_types() {
    CPPUNIT_ASSERT_EQUAL(SymbolType::Num, symbols[0].type());
    CPPUNIT_ASSERT_EQUAL(SymbolType::Num, symbols[1].type());
    CPPUNIT_ASSERT_EQUAL(SymbolType::Num, symbols[2].type());
    CPPUNIT_ASSERT_EQUAL(SymbolType::Num, symbols[3].type());

    CPPUNIT_ASSERT_EQUAL(SymbolType::Fun, symbols[4].type());
    CPPUNIT_ASSERT_EQUAL(SymbolType::Fun, symbols[5].type());

    CPPUNIT_ASSERT_EQUAL(SymbolType::Str, symbols[6].type());
    CPPUNIT_ASSERT_EQUAL(SymbolType::Str, symbols[7].type());

    CPPUNIT_ASSERT_EQUAL(SymbolType::Inf, symbols[8].type());
    CPPUNIT_ASSERT_EQUAL(symbols[9].type(), SymbolType::Sup);

    CPPUNIT_ASSERT_EQUAL(SymbolType::Fun, symbols[11].type());
}

void TestSymbol::test_symbols() {
    CPPUNIT_ASSERT_EQUAL(INT_MIN, symbols[0].num());
    CPPUNIT_ASSERT_EQUAL(INT_MAX, symbols[1].num());
    CPPUNIT_ASSERT_EQUAL( 0, symbols[2].num());
    CPPUNIT_ASSERT_EQUAL(42, symbols[3].num());

    CPPUNIT_ASSERT_EQUAL(String("x"), symbols[4].name());
    CPPUNIT_ASSERT_EQUAL(String("abc"), symbols[5].name());

    CPPUNIT_ASSERT_EQUAL(String(""), symbols[6].string());
    CPPUNIT_ASSERT_EQUAL(String("xyz"), symbols[7].string());

    CPPUNIT_ASSERT_EQUAL(size_t(2u), symbols[10].args().size);
    CPPUNIT_ASSERT_EQUAL(42, symbols[10].args()[0].num());
    CPPUNIT_ASSERT_EQUAL(String("a"), symbols[10].args()[1].name());

    CPPUNIT_ASSERT_EQUAL(String("f"), symbols[11].name());
    CPPUNIT_ASSERT_EQUAL(42, symbols[11].args()[0].num());
    CPPUNIT_ASSERT_EQUAL(String("a"), symbols[11].args()[1].name());
}

void TestSymbol::test_cmp_num() {
    CPPUNIT_ASSERT(!(Symbol::createNum(0) < Symbol::createNum(0)));
    CPPUNIT_ASSERT( (Symbol::createNum(0) < Symbol::createNum(1)));
    CPPUNIT_ASSERT(!(Symbol::createNum(1) < Symbol::createNum(0)));

    CPPUNIT_ASSERT(!(Symbol::createNum(0) > Symbol::createNum(0)));
    CPPUNIT_ASSERT(!(Symbol::createNum(0) > Symbol::createNum(1)));
    CPPUNIT_ASSERT( (Symbol::createNum(1) > Symbol::createNum(0)));

    CPPUNIT_ASSERT( (Symbol::createNum(0) <= Symbol::createNum(0)));
    CPPUNIT_ASSERT( (Symbol::createNum(0) <= Symbol::createNum(1)));
    CPPUNIT_ASSERT(!(Symbol::createNum(1) <= Symbol::createNum(0)));

    CPPUNIT_ASSERT( (Symbol::createNum(0) >= Symbol::createNum(0)));
    CPPUNIT_ASSERT(!(Symbol::createNum(0) >= Symbol::createNum(1)));
    CPPUNIT_ASSERT( (Symbol::createNum(1) >= Symbol::createNum(0)));

    CPPUNIT_ASSERT( (Symbol::createNum(0) == Symbol::createNum(0)));
    CPPUNIT_ASSERT(!(Symbol::createNum(0) == Symbol::createNum(1)));
    CPPUNIT_ASSERT(!(Symbol::createNum(1) == Symbol::createNum(0)));

    CPPUNIT_ASSERT(!(Symbol::createNum(0) != Symbol::createNum(0)));
    CPPUNIT_ASSERT( (Symbol::createNum(0) != Symbol::createNum(1)));
    CPPUNIT_ASSERT( (Symbol::createNum(1) != Symbol::createNum(0)));
}

void TestSymbol::test_cmp_type() {
    CPPUNIT_ASSERT(symbols[8] < symbols[0]);
    CPPUNIT_ASSERT(symbols[0] < symbols[4]);
    CPPUNIT_ASSERT(symbols[4] < symbols[6]);
    CPPUNIT_ASSERT(symbols[6] < symbols[10]);
    CPPUNIT_ASSERT(symbols[11] < symbols[9]);
}

void TestSymbol::test_cmp_other() {
    CPPUNIT_ASSERT(!(Symbol::createId("a") == Symbol::createId("a").flipSign()));
    CPPUNIT_ASSERT( (Symbol::createId("a") != Symbol::createId("a").flipSign()));

    CPPUNIT_ASSERT(!(Symbol::createId("a") < Symbol::createId("a")));
    CPPUNIT_ASSERT( (Symbol::createId("a") < Symbol::createId("a").flipSign()));
    CPPUNIT_ASSERT( (Symbol::createId("b") < Symbol::createId("a").flipSign()));
    CPPUNIT_ASSERT( (Symbol::createId("aaa") < Symbol::createId("aab")));
    CPPUNIT_ASSERT( (Symbol::createId("a") < Symbol::createId("aa")));
    CPPUNIT_ASSERT(!(Symbol::createId("aa") < Symbol::createId("a")));

    CPPUNIT_ASSERT(!(Symbol::createStr("a") < Symbol::createStr("a")));
    CPPUNIT_ASSERT( (Symbol::createStr("aaa") < Symbol::createStr("aab")));
    CPPUNIT_ASSERT( (Symbol::createStr("a") < Symbol::createStr("aa")));
    CPPUNIT_ASSERT(!(Symbol::createStr("aa") < Symbol::createStr("a")));

    Symbol a = Symbol::createTuple( Potassco::toSpan(SymVec{ Symbol::createNum(1), Symbol::createNum(1) }) );
    Symbol b = Symbol::createTuple( Potassco::toSpan(SymVec{ Symbol::createNum(1) }) );
    Symbol c = Symbol::createTuple( Potassco::toSpan(SymVec{ Symbol::createNum(1), Symbol::createNum(2) }) );
    Symbol d = Symbol::createTuple( Potassco::toSpan(SymVec{ Symbol::createNum(2) }) );
    CPPUNIT_ASSERT((b < a) && !(a < b));
    CPPUNIT_ASSERT((a < c) && !(c < a));
    CPPUNIT_ASSERT((b < d) && !(d < b));
    CPPUNIT_ASSERT((d < a) && !(a < d));

    Symbol fa = Symbol::createFun( "f", Potassco::toSpan(SymVec{ Symbol::createNum(1), Symbol::createNum(1) }) );
    Symbol ga = Symbol::createFun( "g", Potassco::toSpan(SymVec{ Symbol::createNum(1), Symbol::createNum(1) }));
    Symbol fb = Symbol::createFun( "f", Potassco::toSpan(SymVec{ Symbol::createNum(1) }) );
    Symbol fc = Symbol::createFun( "f", Potassco::toSpan(SymVec{ Symbol::createNum(1), Symbol::createNum(2) }) );
    Symbol fd = Symbol::createFun( "f", Potassco::toSpan(SymVec{ Symbol::createNum(2) }) );
    Symbol gd = Symbol::createFun( "g", Potassco::toSpan(SymVec{ Symbol::createNum(2) }) );
    CPPUNIT_ASSERT((fb < fa) && !(fa < fb));
    CPPUNIT_ASSERT((fa < fc) && !(fc < fa));
    CPPUNIT_ASSERT((fb < fd) && !(fd < fb));
    CPPUNIT_ASSERT((fd < fa) && !(fa < fd));
    CPPUNIT_ASSERT((fa < ga) && !(ga < fa));
    CPPUNIT_ASSERT((gd < fa) && !(fa < gd));
    CPPUNIT_ASSERT((fa < fa.flipSign()) && !(fa.flipSign() < fa));
    CPPUNIT_ASSERT((fa < ga.flipSign()) && !(ga.flipSign() < fa));
}

void TestSymbol::test_print() {
    std::ostringstream oss;
    auto toString = [&oss](Symbol const &val) -> std::string {
        oss << val;
        std::string str = oss.str();
        oss.str("");
        return str;

    };
    CPPUNIT_ASSERT_EQUAL(std::string("0"), toString(symbols[2]));
    CPPUNIT_ASSERT_EQUAL(std::string("42"), toString(symbols[3]));
    CPPUNIT_ASSERT_EQUAL(std::string("x"), toString(symbols[4]));
    CPPUNIT_ASSERT_EQUAL(std::string("-x"), toString(symbols[4].flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("x"), toString(symbols[4].flipSign().flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("abc"), toString(symbols[5]));
    CPPUNIT_ASSERT_EQUAL(std::string("\"\""), toString(symbols[6]));
    CPPUNIT_ASSERT_EQUAL(std::string("\"xyz\""), toString(symbols[7]));
    CPPUNIT_ASSERT_EQUAL(std::string("#inf"), toString(symbols[8]));
    CPPUNIT_ASSERT_EQUAL(std::string("#sup"), toString(symbols[9]));
    CPPUNIT_ASSERT_EQUAL(std::string("(42,a)"), toString(symbols[10]));
    CPPUNIT_ASSERT_EQUAL(std::string("f(42,a)"), toString(symbols[11]));
    CPPUNIT_ASSERT_EQUAL(std::string("-f(42,a)"), toString(symbols[11].flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("f(42,a)"), toString(symbols[11].flipSign().flipSign()));
    CPPUNIT_ASSERT_EQUAL(std::string("()"), toString(Symbol::createTuple(SymSpan{nullptr, 0})));

    std::string comp = toString(Symbol::createFun("g", SymSpan{symbols.data() + 2, symbols.size() - 2}));
    CPPUNIT_ASSERT_EQUAL(std::string("g(0,42,x,abc,\"\",\"xyz\",#inf,#sup,(42,a),f(42,a))"), comp);
}

void TestSymbol::test_sig() {
    std::vector<std::string> names { "a", "b", "c", "d" };
    for (unsigned i = 0; i < 42; i++) {
        auto name = names[i % names.size()];
        Signature sig{ name, i };
        CPPUNIT_ASSERT_EQUAL(name, *sig.name());
        CPPUNIT_ASSERT_EQUAL(i, sig.length());
    }
}

TestSymbol::~TestSymbol() { }

CPPUNIT_TEST_SUITE_REGISTRATION(TestSymbol);

// }}}

} } // namespace Test Gringo

