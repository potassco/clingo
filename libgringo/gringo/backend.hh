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

#include <gringo/value.hh>
#include <potassco/theory_data.h>

namespace Gringo {

// {{{1 declaration of Backend

class Backend {
public:
    using AtomVec      = std::vector<Potassco::Atom_t>;
    using LitVec       = std::vector<Potassco::Lit_t>;
    using LitWeightVec = std::vector<Potassco::WeightLit_t>;
    using TId          = Potassco::Id_t;
    using GetCond      = std::function<LitVec(Potassco::Id_t)>;
    virtual void init(bool incremental) = 0;
    virtual void beginStep() = 0;
    virtual void printTheoryAtom(Potassco::TheoryAtom const &atom, GetCond getCond) = 0;
    virtual void printHead(bool choice, AtomVec const &atoms) = 0;
    virtual void printNormalBody(LitVec const &body) = 0;
    virtual void printWeightBody(Potassco::Weight_t lower, LitWeightVec const &body) = 0;
    virtual void printProject(AtomVec const &atoms) = 0;
    virtual void printOutput(char const *symbol, LitVec const &body) = 0;
    virtual void printEdge(unsigned u, unsigned v, LitVec const &body) = 0;
    virtual void printHeuristic(Potassco::Heuristic_t modifier, Potassco::Atom_t atom, int value, unsigned priority, LitVec const &body) = 0;
    virtual void printExternal(Potassco::Atom_t atom, Potassco::Value_t value) = 0;
    virtual void printAssume(LitVec const &lits) = 0;
    virtual void printMinimize(int priority, LitWeightVec const &body) = 0;
    virtual void endStep() = 0;

    AtomVec &tempAtoms() {
        hd_.clear();
        return hd_;
    }
    LitVec &tempLits() {
        bd_.clear();
        return bd_;
    }
    LitWeightVec &tempWLits() {
        wb_.clear();
        return wb_;
    }

    virtual ~Backend() noexcept { }

public:
    AtomVec hd_;
    LitVec  bd_;
    LitWeightVec wb_;
};
using UBackend = std::unique_ptr<Backend>;

// }}}1

} // namespace Gringo

#endif // _GRINGO_BACKEND_HH
