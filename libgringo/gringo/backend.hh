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

#ifndef _GRINGO_BACKEND_HH
#define _GRINGO_BACKEND_HH

#include <gringo/symbol.hh>
#include <potassco/theory_data.h>
#include <sstream>

namespace Gringo {

// {{{1 declaration of Backend

using GetTheoryAtomCondition = std::function<std::vector<Potassco::Lit_t> (Potassco::Id_t)>;
using BackendAtomVec = std::vector<Potassco::Atom_t>;
using BackendLitVec = std::vector<Potassco::Lit_t>;
using BackendLitWeightVec = std::vector<Potassco::WeightLit_t>;

class Backend {
public:
    using Head_t = Potassco::Head_t;
    using Weight_t = Potassco::Weight_t;
    using Value_t = Potassco::Value_t;
    using Atom_t = Potassco::Atom_t;
    using Heuristic_t = Potassco::Heuristic_t;
    using Id_t = Potassco::Id_t;
    using IdSpan = Potassco::IdSpan;
    using AtomSpan = Potassco::AtomSpan;
    using LitSpan = Potassco::LitSpan;
    using WeightLitSpan = Potassco::WeightLitSpan;

    virtual void initProgram(bool incremental) = 0;
    virtual void beginStep() = 0;

    virtual void rule(Head_t ht, const AtomSpan& head, const LitSpan& body) = 0;
    virtual void rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& body) = 0;
    virtual void minimize(Weight_t prio, const WeightLitSpan& lits) = 0;

    virtual void project(const AtomSpan& atoms) = 0;
    virtual void output(Symbol sym, Potassco::Atom_t atom) = 0;
    virtual void output(Symbol sym, Potassco::LitSpan const& condition) = 0;
    virtual void external(Atom_t a, Value_t v) = 0;
    virtual void assume(const LitSpan& lits) = 0;
    virtual void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition) = 0;
    virtual void acycEdge(int s, int t, const LitSpan& condition) = 0;

    virtual void theoryTerm(Id_t termId, int number) = 0;
    virtual void theoryTerm(Id_t termId, const StringSpan& name) = 0;
    virtual void theoryTerm(Id_t termId, int cId, const IdSpan& args) = 0;
    virtual void theoryElement(Id_t elementId, const IdSpan& terms, const LitSpan& cond) = 0;
    virtual void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements) = 0;
    virtual void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements, Id_t op, Id_t rhs) = 0;

    virtual void endStep() = 0;
    virtual ~Backend() = default;
};
using UBackend = std::unique_ptr<Backend>;

inline void outputRule(Backend &out, bool choice, Potassco::AtomSpan head, Potassco::LitSpan body) {
    out.rule(choice ? Potassco::Head_t::Choice : Potassco::Head_t::Disjunctive, head, body);
}

inline void outputRule(Backend &out, bool choice, BackendAtomVec const &head, BackendLitVec const &body) {
    outputRule(out, choice, Potassco::toSpan(head), Potassco::toSpan(body));
}

inline void outputRule(Backend &out, bool choice, Potassco::AtomSpan head, Potassco::Weight_t lower, Potassco::WeightLitSpan body) {
    out.rule(choice ? Potassco::Head_t::Choice : Potassco::Head_t::Disjunctive, head, lower, body);
}

inline void outputRule(Backend &out, bool choice, BackendAtomVec const &head, Potassco::Weight_t lower, BackendLitWeightVec const &body) {
    outputRule(out, choice, Potassco::toSpan(head), lower, Potassco::toSpan(body));
}

// }}}1

} // namespace Gringo

#endif // _GRINGO_BACKEND_HH
