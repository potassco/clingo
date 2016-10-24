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

#include "gringo/backend.hh"
#include "potassco/theory_data.h"
#include <cstring>

namespace Gringo {

namespace {

class TheoryVisitor : public Potassco::TheoryData::Visitor {
public:
    TheoryVisitor(Backend &out, GetTheoryAtomCondition &cond)
    : out_(out)
    , cond_(cond) { }

private:
    void print(Potassco::Id_t termId, const Potassco::TheoryTerm& term) {
        switch (term.type()) {
            case Potassco::Theory_t::Number  : out_.theoryTerm(termId, term.number()); break;
            case Potassco::Theory_t::Symbol  : out_.theoryTerm(termId, Potassco::toSpan(term.symbol())); break;
            case Potassco::Theory_t::Compound: out_.theoryTerm(termId, term.compound(), term.terms()); break;
        }
    }
    void print(const Potassco::TheoryAtom& a) {
        if (a.guard()) { out_.theoryAtom(a.atom(), a.term(), a.elements(), *a.guard(), *a.rhs()); }
        else           { out_.theoryAtom(a.atom(), a.term(), a.elements()); }
    }
    void visit(Potassco::TheoryData const &data, Potassco::Id_t termId, Potassco::TheoryTerm const &t) override {
        if (addSeen(tSeen_, termId)) { // only visit once
            // visit any subterms then print
            data.accept(t, *this);
            print(termId, t);
        }
    }
    void visit(Potassco::TheoryData const &data, Potassco::Id_t elemId, Potassco::TheoryElement const &e) override {
        if (addSeen(eSeen_, elemId)) { // only visit once
            // visit terms then print element
            data.accept(e, *this);
            out_.theoryElement(elemId, e.terms(), Potassco::toSpan(cond_(elemId)));
        }
    }
    void visit(Potassco::TheoryData const &data, Potassco::TheoryAtom const &a) override {
        // visit elements then print atom
        data.accept(a, *this);
        print(a);
    }
    bool addSeen(std::vector<bool>& vec, Potassco::Id_t id) const {
        if (vec.size() <= id) { vec.resize(id + 1, false); }
        bool seen = vec[id];
        if (!seen) { vec[id] = true; }
        return !seen;
    }
    Backend &out_;
    GetTheoryAtomCondition &cond_;
    std::vector<bool> tSeen_;
    std::vector<bool> eSeen_;
};

} // namespace

void output(Potassco::TheoryData const &data, Backend &out, GetTheoryAtomCondition cond) {
    TheoryVisitor visitor(out, cond);
    data.accept(visitor);
}

} // namespace Output Gringo
