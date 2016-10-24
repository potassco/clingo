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
    virtual void output(Symbol sym, int value, Potassco::LitSpan const& condition) = 0;
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

void output(Potassco::TheoryData const &data, Backend &out, GetTheoryAtomCondition cond);

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
