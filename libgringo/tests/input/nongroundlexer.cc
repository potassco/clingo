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
#include "gringo/symbol.hh"
#include "gringo/scripts.hh"

#include "tests/tests.hh"

namespace Gringo { namespace Input { namespace Test {

TEST_CASE("input-nongroundlexer", "[input]") {
    Gringo::Test::TestGringoModule module;
    std::ostringstream oss;
    Potassco::TheoryData td;
    Output::OutputBase out(td, {}, oss);
    Program prg;
    Defines defs;
    Scripts scripts;
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
    ngp.pushStream("-", std::unique_ptr<std::istream>(new std::stringstream(in)), module.logger);

    Location loc("<undef>", 0, 0, "<undef>", 0, 0);
    NonGroundGrammar::parser::semantic_type val;

    REQUIRE(int(NonGroundGrammar::parser::token::PYTHON) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::MINIMIZE) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::MINIMIZE) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::INFIMUM) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::INFIMUM) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::SEM) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::MUL) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::SEM) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::MUL) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::NOT) == ngp.lex(&val, loc));
    REQUIRE(int(NonGroundGrammar::parser::token::IDENTIFIER) == ngp.lex(&val, loc));
    REQUIRE(String("xyz") == String::fromRep(val.str));
    REQUIRE(int(NonGroundGrammar::parser::token::IDENTIFIER) == ngp.lex(&val, loc));
    REQUIRE(String("_xyz") == String::fromRep(val.str));
    REQUIRE(int(NonGroundGrammar::parser::token::IDENTIFIER) == ngp.lex(&val, loc));
    REQUIRE(String("__xyz") == String::fromRep(val.str));
    REQUIRE(int(NonGroundGrammar::parser::token::IDENTIFIER) == ngp.lex(&val, loc));
    REQUIRE(String("___xyz") == String::fromRep(val.str));
    REQUIRE(5 == loc.beginLine);
    REQUIRE(23 == loc.beginColumn);
    REQUIRE(0 == ngp.lex(&val, loc));
}

// }}}

} } } // namespace Test Input Gringo

