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

#include "gringo/bug.hh"
#include "reify/parser.hh"
#include "reify/program.hh"
#include "reify/util.hh"
#include <sstream>
#include <stdexcept>

Reify::Parser::Parser(Program &prg)
: prg_(prg) { }

void Reify::Parser::parseProgram() {
    AtomVec head;
    LitVec body;
    LitWeightVec weightedBody;
    for (;;) {
        head.clear();
        body.clear();
        weightedBody.clear();
        Size type = lexNumber();
        switch (type) {
            case 1: {
                head.emplace_back(lexNumber());
                Size size = lexNumber();
                Size negative = lexNumber();
                for (Size i = 0; i < size; ++i) {
                    Lit lit = lexNumber();
                    if (i < negative) { lit = -lit; }
                    body.emplace_back(lit);
                }
                prg_.addRule(Rule(head, body));
                break;
            }
            case 2: {
                head.emplace_back(lexNumber());
                Size size = lexNumber();
                Size negative = lexNumber();
                Size bound = lexNumber();
                for (Size i = 0; i < size; ++i) {
                    Lit lit = lexNumber();
                    if (i < negative) { lit = -lit; }
                    weightedBody.emplace_back(lit, 1);
                }
                prg_.addRule(Rule(head, weightedBody, bound));
                break;
            }
            case 3: {
                Size heads = lexNumber();
                for (Size i = 0; i < heads; ++i) {
                    head.emplace_back(lexNumber());
                }
                Size size = lexNumber();
                Size negative = lexNumber();
                for (Size i = 0; i < size; ++i) {
                    Lit lit = lexNumber();
                    if (i < negative) { lit = -lit; }
                    body.emplace_back(lit);
                }
                prg_.addRule(Rule(head, body, true));
                break;
            }
            case 5: {
                head.emplace_back(lexNumber());
                Size bound = lexNumber();
                Size size = lexNumber();
                Size negative = lexNumber();
                for (Size i = 0; i < size; ++i) {
                    Lit lit = lexNumber();
                    if (i < negative) { lit = -lit; }
                    weightedBody.emplace_back(lit, 0);
                }
                for (Size i = 0; i < size; ++i) {
                    weightedBody[i].second = lexNumber();
                }
                prg_.addRule(Rule(head, weightedBody, bound));
                break;
            }
            case 6: {
                lexNumber();
                Size size = lexNumber();
                Size negative = lexNumber();
                for (Size i = 0; i < size; ++i) {
                    Lit lit = lexNumber();
                    if (i < negative) { lit = -lit; }
                    weightedBody.emplace_back(lit, 0);
                }
                for (Size i = 0; i < size; ++i) {
                    weightedBody[i].second = lexNumber();
                }
                prg_.addMinimize(weightedBody);
                break;
            }
            case 8: {
                Size heads = lexNumber();
                for (Size i = 0; i < heads; ++i) {
                    head.emplace_back(lexNumber());
                }
                Size size = lexNumber();
                Size negative = lexNumber();
                for (Size i = 0; i < size; ++i) {
                    Lit lit = lexNumber();
                    if (i < negative) { lit = -lit; }
                    body.emplace_back(lit);
                }
                prg_.addRule(Rule(head, body, false));
                break;
            }
            case 0: {
                return;
            }
            default: {
                error("unsupported rule type: " + std::to_string(type));
            }
        }
    }
}

void Reify::Parser::parseSymbolTable() {
    for (;;) {
        Atom atom = lexNumber();
        if (atom) {
            skipWS();
            std::string name = lexStringNL();
            prg_.showAtom(atom, std::move(name));
        }
        else { return; }
    }
}

void Reify::Parser::parseCompute() {
    skipBP();
    while (Atom atom = lexNumber()) { prg_.addCompute(atom); }
    skipBM();
    while (Atom atom = lexNumber()) { prg_.addCompute(-atom); }
    Size models = lexNumber();
    prg_.setModels(models);
}

void Reify::Parser::parse() {
    parseProgram();
    parseSymbolTable();
    parseCompute();
    skipEOF();
}

void Reify::Parser::parse(std::string const &file) {
    file_ = file;
    push(file_, 0);
    parse();
    pop();
}

void Reify::Parser::parse(std::string const &file, std::unique_ptr<std::istream> &&in) {
    file_ = file;
    push(std::move(in), 0);
    parse();
    pop();
}

void Reify::Parser::beginLoc() {
    loc_.first.first  = line();
    loc_.first.second = column();
}

void Reify::Parser::endLoc() {
    loc_.second.first  = line();
    loc_.second.second = column();
}

void Reify::Parser::error(std::string const &msg) {
    std::ostringstream oss;
    oss << file_ << ":"
        << loc_.first.first << ":" << loc_.first.second;
    if (loc_.first.first == loc_.second.first) {
        if (loc_.first.second != loc_.second.second) {
            oss << "-" << loc_.second.second;
        }
    }
    else {
        oss << loc_.second.first << ":" << loc_.second.second;
    }
    oss << ": " << msg;
    throw std::runtime_error(oss.str());
}

#include "lexer.hh"

