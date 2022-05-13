// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

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
    enum Condition { yyccomment, yycblockcomment, yycscript, yycscript_body, yycnormal, yyctheory, yycdefinition };
public:
    NonGroundParser(INongroundProgramBuilder &pb, bool &incmode);
    void parseError(Location const &loc, std::string const &token);
    void pushFile(std::string &&filename, Logger &log);
    void pushStream(std::string &&name, std::unique_ptr<std::istream>, Logger &log);
    void pushBlock(std::string const &name, IdVec const &vec, std::string const &block, Logger &log);
    int lex(void *pValue, Location &loc);
    bool parseDefine(std::string const &define, Logger &log);
    bool parse(Logger &log);
    bool empty() { return LexerState::empty(); }
    void include(String file, Location const &loc, bool include, Logger &log);
    void theoryLexing(TheoryLexing mode);
    INongroundProgramBuilder &builder();
    // Note: only to be used during parsing
    Logger &logger() { assert(log_); return *log_; }
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
    void lexerError(Location const &loc, StringSpan token);
    bool push(std::string const &filename, bool include = false);
    bool push(std::string const &file, std::unique_ptr<std::istream> in);
    void pop();
    void init_();
    void condition(Condition cond);
    using LexerState<std::pair<String, std::pair<String, IdVec>>>::start;
    void start(Location &loc);
    Location &end(Location &loc);
    using LexerState<std::pair<String, std::pair<String, IdVec>>>::eof;
    Location &eof(Location &loc);
    Condition condition() const;
    String filename() const;

private:
    std::set<std::string> filenames_;
    bool &incmode_;
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
    Indexed<Aggr> aggregates_;
    int           injectSymbol_;
    Condition     condition_ = yycnormal;
    String        filename_;
    Logger *log_ = nullptr;
};

// }}}

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_NONGROUNDPARSER_HH
