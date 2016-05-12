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

namespace Gringo { namespace Input {

GroundTermParser::GroundTermParser() { }
Value GroundTermParser::parse(std::string const &str) {
    undefined_ = false;
    while (!empty()) { pop(); }
    push(gringo_make_unique<std::stringstream>(str), 0);
    GroundTermGrammar::parser parser(this);
    parser.parse();
    return undefined_ ? Value() : value;
}

Value GroundTermParser::term(BinOp op, Value a, Value b) {
    if (a.type() == Value::NUM && b.type() == Value::NUM && (op != BinOp::DIV || b.num() != 0)) {
        return Value::createNum(Gringo::eval(op, a.num(), b.num()));
    }
    undefined_ = true;
    return Value::createNum(0);
}

Value GroundTermParser::term(UnOp op, Value a) {
    if (a.type() == Value::NUM) {
        int num = a.num();
        switch (op) {
            case UnOp::NEG: { return Value::createNum(-num); }
            case UnOp::ABS: { return Value::createNum(std::abs(num)); }
            case UnOp::NOT: { return Value::createNum(~num); }
        }
        assert(false);
    }
    else if (op == UnOp::NEG && (a.type() == Value::ID || a.type() == Value::FUNC)) {
        return a.flipSign();
    }
    undefined_ = true;
    return Value::createNum(0);
}

unsigned GroundTermParser::terms() {
    return terms_.emplace();
}

Value GroundTermParser::tuple(unsigned uid, bool forceTuple) {
    FWValVec args(terms_.erase(uid));
    if (!forceTuple && args.size() == 1) {
        return args.front();
    }
    else {
        return Value::createTuple(args);
    }
}

unsigned GroundTermParser::terms(unsigned uid, Value a) {
    terms_[uid].emplace_back(a);
    return uid;
}

FWValVec GroundTermParser::terms(unsigned uid) {
    return terms_.erase(uid);
}

void GroundTermParser::parseError(std::string const &message) {
    std::ostringstream out;
    out << "parse error in line " << line() << " column " << column() << ": " << message;
    throw std::runtime_error(out.str());
}

void GroundTermParser::lexerError(std::string const &token) {
    std::ostringstream out;
    out << "lexer error in line " << line() << " column " << column() << ": unexpected token " << token;
    throw std::runtime_error(out.str());
}

int GroundTermParser::lex(void *pValue) {
    return lex_impl(pValue);
}

GroundTermParser::~GroundTermParser() { }

} }

#include "input/groundtermlexer.hh"
