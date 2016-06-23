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

#ifndef _GRINGO_INPUT_GROUNDTERMPARSER_HH
#define _GRINGO_INPUT_GROUNDTERMPARSER_HH

#include <gringo/lexerstate.hh>
#include <gringo/symbol.hh>
#include <gringo/term.hh>
#include <gringo/indexed.hh>

namespace Gringo { namespace Input {

// {{{ declaration of GroundTermParser

class GroundTermParser : private LexerState<int> {
    using IndexedTerms = Indexed<SymVec, unsigned>;
public:
    GroundTermParser();
    Symbol parse(std::string const &str, Logger &log);
    ~GroundTermParser();
    // NOTE: only to be used durning parsing (actually it would be better to hide this behind a private interface)
    Logger &logger() { assert(log_); return *log_; }
    void parseError(std::string const &message, Logger &log);
    void lexerError(StringSpan token, Logger &log);
    int lex(void *pValue, Logger &log);

    Symbol term(BinOp op, Symbol a, Symbol b);
    Symbol term(UnOp op, Symbol a);
    unsigned terms();
    unsigned terms(unsigned uid, Symbol a);
    SymVec terms(unsigned uid);
    Symbol tuple(unsigned uid, bool forceTuple);

    Symbol        value;
private:
    int lex_impl(void *pValue, Logger &log);

    IndexedTerms terms_;
    Logger *log_ = nullptr;
    bool         undefined_;
};

// }}}

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_GROUNDTERMPARSER_HH
