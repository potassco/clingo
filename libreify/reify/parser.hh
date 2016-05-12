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

#ifndef REIFY_PARSER_HH
#define REIFY_PARSER_HH

#include <gringo/lexerstate.hh>

namespace Reify {

using Location = std::pair<std::pair<unsigned, unsigned>, std::pair<unsigned, unsigned>>;

class Program;
class Parser : private Gringo::LexerState<int> {
public:
    Parser(Program &prg);
    void parse(std::string const &file);
    void parse(std::string const &file, std::unique_ptr<std::istream> &&in);
private:
    void parse();
    void parseProgram();
    void parseSymbolTable();
    void parseCompute();
    void error(std::string const &msg);
    int lexNumber();
    std::string lexStringNL();
    void skipWS();
    void skipBP();
    void skipBM();
    void skipEOF();
    void beginLoc();
    void endLoc();
private:
    Program &prg_;
    std::string file_;
    Location loc_;

};

} // namespace Reify

#endif // REIFY_PARSER_HH

