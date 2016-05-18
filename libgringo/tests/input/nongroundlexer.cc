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

#include "gringo/input/nongroundparser.hh"
#include "gringo/input/programbuilder.hh"
#include "gringo/input/program.hh"
#include "gringo/output/output.hh"
#include "input/nongroundgrammar/grammar.hh"
#include "gringo/value.hh"
#include "gringo/scripts.hh"
#include "tests/gringo_module.hh"

#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include <climits>
#include <sstream>

namespace Gringo { namespace Input { namespace Test {

// {{{ declaration of TestNongroundLexer

class TestNongroundLexer : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestNongroundLexer);
        CPPUNIT_TEST(test_lexer);
    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

    void test_lexer();

    virtual ~TestNongroundLexer();
};

// }}}

// {{{ definition of TestNongroundLexer

void TestNongroundLexer::setUp() {
}

void TestNongroundLexer::tearDown() {
}

void TestNongroundLexer::test_lexer() {
    std::ostringstream oss;
    Potassco::TheoryData td;
    Output::OutputBase out(td, {}, oss);
    Program prg;
    Defines defs;
    Scripts scripts(Gringo::Test::getTestModule());
    NongroundProgramBuilder pb(scripts, prg, out, defs);
    NonGroundParser ngp(pb);
    std::string in =
        "#script (python) #end "
        "%*xyz\nxyz\n*%"
        "%xyz\n"
        "#minimize "
        "#minimise "
        "#infimum "
        "#inf "
        ";* "
        ";\n * "
        "not "
        "xyz "
        "_xyz "
        "__xyz "
        "___xyz "
        // TODO: check errors too: "# "
        ;
    ngp.pushStream("-", std::unique_ptr<std::istream>(new std::stringstream(in)));

    Location loc("<undef>", 0, 0, "<undef>", 0, 0);
    NonGroundGrammar::parser::semantic_type val;

    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::PYTHON), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::MINIMIZE), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::MINIMIZE), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::INFIMUM), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::INFIMUM), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::SEM), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::MUL), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::SEM), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::MUL), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::NOT), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::IDENTIFIER), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(String("xyz"), String::fromRep(val.str));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::IDENTIFIER), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(String("_xyz"), String::fromRep(val.str));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::IDENTIFIER), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(String("__xyz"), String::fromRep(val.str));
    CPPUNIT_ASSERT_EQUAL(int(NonGroundGrammar::parser::token::IDENTIFIER), ngp.lex(&val, loc));
    CPPUNIT_ASSERT_EQUAL(String("___xyz"), String::fromRep(val.str));
    CPPUNIT_ASSERT_EQUAL(5u, loc.beginLine);
    CPPUNIT_ASSERT_EQUAL(23u, loc.beginColumn);
    CPPUNIT_ASSERT_EQUAL(0, ngp.lex(&val, loc));
}

TestNongroundLexer::~TestNongroundLexer() { }

CPPUNIT_TEST_SUITE_REGISTRATION(TestNongroundLexer);

// }}}

} } } // namespace Test Input Gringo

