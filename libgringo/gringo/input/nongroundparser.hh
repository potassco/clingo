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

#ifndef _GRINGO_INPUT_NONGROUNDPARSER_HH
#define _GRINGO_INPUT_NONGROUNDPARSER_HH

#include <gringo/input/programbuilder.hh>
#include <gringo/lexerstate.hh>
#include <memory>
#include <iosfwd>
#include <set>

namespace Gringo { namespace Input {

// {{{ declaration of NonGroundParser

using StringVec   = std::vector<std::string>;
using ProgramVec  = std::vector<std::tuple<String, IdVec, std::string>>;

enum class TheoryLexing { Disabled, Theory, Definition };

class NonGroundParser : private LexerState<std::pair<String, std::pair<String, IdVec>>> {
private:
    enum Condition { yyccomment, yycblockcomment, yycpython, yyclua, yycnormal, yyctheory, yycdefinition };
public:
    NonGroundParser(INongroundProgramBuilder &pb);
    void parseError(Location const &loc, std::string const &token);
    void pushFile(std::string &&filename, MessagePrinter &log);
    void pushStream(std::string &&name, std::unique_ptr<std::istream>, MessagePrinter &log);
    void pushBlock(std::string const &name, IdVec const &vec, std::string const &block, MessagePrinter &log);
    int lex(void *pValue, Location &loc);
    bool parseDefine(std::string const &define, MessagePrinter &log);
    bool parse(MessagePrinter &log);
    bool empty() { return LexerState::empty(); }
    void include(String file, Location const &loc, bool include, MessagePrinter &log);
    void theoryLexing(TheoryLexing mode);
    INongroundProgramBuilder &builder();
    // Note: only to be used during parsing
    MessagePrinter &logger() { assert(log_); return *log_; }
    // {{{ aggregate helper functions
    BoundVecUid boundvec(Relation ra, TermUid ta, Relation rb, TermUid tb);
    unsigned aggregate(AggregateFunction fun, bool choice, unsigned elems, BoundVecUid bounds);
    unsigned aggregate(TheoryAtomUid atom);
    HdLitUid headaggregate(Location const &loc, unsigned hdaggr);
    BdLitVecUid bodyaggregate(BdLitVecUid body, Location const &loc, NAF naf, unsigned bdaggr);
    // }}}
    ~NonGroundParser();

private:
    int lex_impl(void *pValue, Location &loc);
    void lexerError(StringSpan token);
    bool push(std::string const &filename, bool include = false);
    bool push(std::string const &file, std::unique_ptr<std::istream> in);
    void pop();
    void _init();
    void condition(Condition cond);
    using LexerState<std::pair<String, std::pair<String, IdVec>>>::start;
    void start(Location &loc);
    Condition condition() const;
    String filename() const;

private:
    std::set<std::string> filenames_;
    bool incmodeIncluded_ = false;
    TheoryLexing theoryLexing_ = TheoryLexing::Disabled;
    String not_;
    INongroundProgramBuilder &pb_;
    struct Aggr
    {
        AggregateFunction fun;
        unsigned choice;
        unsigned elems;
        BoundVecUid bounds;
    };
    Indexed<Aggr> _aggregates;
    int           _startSymbol;
    Condition     condition_ = yycnormal;
    String        _filename;
    MessagePrinter *log_ = nullptr;
};

// }}}

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_NONGROUNDPARSER_HH
