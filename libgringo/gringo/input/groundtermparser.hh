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
#include <gringo/value.hh>
#include <gringo/term.hh>
#include <memory>
#include <iosfwd>
#include <set>

namespace Gringo { namespace Input {

// {{{ declaration of GroundTermParser

class GroundTermParser : private LexerState<int> {
    using IndexedTerms = Indexed<ValVec, unsigned>;
public:
    GroundTermParser();
    Value parse(std::string const &str);
    ~GroundTermParser();
    void parseError(std::string const &message);
    void lexerError(std::string const &token);
    int lex(void *pValue);

    Value term(BinOp op, Value a, Value b);
    Value term(UnOp op, Value a);
    unsigned terms();
    unsigned terms(unsigned uid, Value a);
    FWValVec terms(unsigned uid);
    Value tuple(unsigned uid, bool forceTuple);

    Value        value;
private:
    int lex_impl(void *pValue);

    IndexedTerms terms_;
    bool         undefined_;
};

// }}}

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_GROUNDTERMPARSER_HH
