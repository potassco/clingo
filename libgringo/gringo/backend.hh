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

namespace Gringo {

// {{{1 declaration of Backend

using GetTheoryAtomCondition = std::function<std::vector<Potassco::Lit_t> (Potassco::Id_t)>;
using BackendAtomVec = std::vector<Potassco::Atom_t>;
using BackendLitVec = std::vector<Potassco::Lit_t>;
using BackendLitWeightVec = std::vector<Potassco::WeightLit_t>;
using Backend = Potassco::AbstractProgram;
using UBackend = std::unique_ptr<Backend>;

void output(Potassco::TheoryData const &data, Potassco::AbstractProgram &out, GetTheoryAtomCondition cond);

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
