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

#include "gringo/input/groundtermparser.hh"
#include "input/groundtermgrammar/grammar.hh"
#include "gringo/logger.hh"

namespace Gringo { namespace Input {

GroundTermParser::GroundTermParser() { }
Symbol GroundTermParser::parse(std::string const &str) {
    undefined_ = false;
    while (!empty()) { pop(); }
    push(gringo_make_unique<std::stringstream>(str), 0);
    GroundTermGrammar::parser parser(this);
    parser.parse();
    return undefined_ ? Symbol() : value;
}

Symbol GroundTermParser::term(BinOp op, Symbol a, Symbol b) {
    if (a.type() == SymbolType::Num && b.type() == SymbolType::Num && (op != BinOp::DIV || b.num() != 0)) {
        return Symbol::createNum(Gringo::eval(op, a.num(), b.num()));
    }
    undefined_ = true;
    return Symbol::createNum(0);
}

Symbol GroundTermParser::term(UnOp op, Symbol a) {
    if (a.type() == SymbolType::Num) {
        int num = a.num();
        switch (op) {
            case UnOp::NEG: { return Symbol::createNum(-num); }
            case UnOp::ABS: { return Symbol::createNum(std::abs(num)); }
            case UnOp::NOT: { return Symbol::createNum(~num); }
        }
        assert(false);
    }
    else if (op == UnOp::NEG && a.type() == SymbolType::Fun) {
        return a.flipSign();
    }
    undefined_ = true;
    return Symbol::createNum(0);
}

unsigned GroundTermParser::terms() {
    return terms_.emplace();
}

Symbol GroundTermParser::tuple(unsigned uid, bool forceTuple) {
    SymVec args(terms_.erase(uid));
    if (!forceTuple && args.size() == 1) {
        return args.front();
    }
    else {
        return Symbol::createTuple(Potassco::toSpan(args));
    }
}

unsigned GroundTermParser::terms(unsigned uid, Symbol a) {
    terms_[uid].emplace_back(a);
    return uid;
}

SymVec GroundTermParser::terms(unsigned uid) {
    return terms_.erase(uid);
}

void GroundTermParser::parseError(std::string const &message, MessagePrinter &log) {
    Location loc("<string>", line(), column(), "<string>", line(), column());
    GRINGO_REPORT(log, W_OPERATION_UNDEFINED)
        << loc << ": " << "error: " << message << "\n";
    throw std::runtime_error("term parsing failed");
}

void GroundTermParser::lexerError(StringSpan token, MessagePrinter &log) {
    Location loc("<string>", line(), column(), "<string>", line(), column());
    GRINGO_REPORT(log, W_OPERATION_UNDEFINED)
        << loc << ": " << "error: unexpected token:\n"
        << std::string(token.first, token.size) << "\n";
    throw std::runtime_error("term parsing failed");
}

int GroundTermParser::lex(void *pValue, MessagePrinter &log) {
    return lex_impl(pValue, log);
}

GroundTermParser::~GroundTermParser() { }

} }

#include "input/groundtermlexer.hh"
