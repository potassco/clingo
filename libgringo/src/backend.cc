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
    TheoryVisitor(Potassco::AbstractProgram &out, GetTheoryAtomCondition &cond)
    : out_(out)
    , cond_(cond) { }

private:
    void visit(Potassco::TheoryData const &data, Potassco::Id_t termId, Potassco::TheoryTerm const &t) override {
        if (addSeen(tSeen_, termId)) { // only visit once
            // visit any subterms then print
            data.accept(t, *this);
            Potassco::print(out_, termId, t);
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
        Potassco::print(out_, a);
    }
    bool addSeen(std::vector<bool>& vec, Potassco::Id_t id) const {
        if (vec.size() <= id) { vec.resize(id + 1, false); }
        bool seen = vec[id];
        if (!seen) { vec[id] = true; }
        return !seen;
    }
    Potassco::AbstractProgram &out_;
    GetTheoryAtomCondition &cond_;
    std::vector<bool> tSeen_;
    std::vector<bool> eSeen_;
};

} // namespace

void output(Potassco::TheoryData const &data, Potassco::AbstractProgram &out, GetTheoryAtomCondition cond) {
    TheoryVisitor visitor(out, cond);
    data.accept(visitor);
}

} // namespace Output Gringo
