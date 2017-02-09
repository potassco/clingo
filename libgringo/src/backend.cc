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
