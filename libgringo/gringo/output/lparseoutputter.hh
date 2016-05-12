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

#ifndef _GRINGO_OUTPUT_INTERFACES_HH
#define _GRINGO_OUTPUT_INTERFACES_HH

#include <gringo/value.hh>
#include <gringo/clonable.hh>
#include <gringo/comparable.hh>
#include <gringo/hashable.hh>
#include <gringo/domain.hh>

namespace Gringo { namespace Output {

// {{{ declaration of LparseOutputter

using OutputPredicates = std::vector<std::tuple<Location, FWSignature, bool>>;

struct LparseOutputter {
    using AtomVec          = std::vector<unsigned>;
    using LitVec           = std::vector<int>;
    using LitWeightVec     = std::vector<std::pair<int, unsigned>>;

    virtual unsigned falseUid() = 0;
    virtual unsigned newUid() = 0;
    virtual void incremental() { }
    virtual void printBasicRule(unsigned head, LitVec const &body) = 0;
    virtual void printChoiceRule(AtomVec const &atoms, LitVec const &body) = 0;
    virtual void printCardinalityRule(unsigned head, unsigned lower, LitVec const &body) = 0;
    virtual void printWeightRule(unsigned head, unsigned lower, LitWeightVec const &body) = 0;
    virtual void printMinimize(LitWeightVec const &body) = 0;
    virtual void printDisjunctiveRule(AtomVec const &atoms, LitVec const &body) = 0;
    virtual void finishRules() = 0;
    virtual void printSymbol(unsigned atomUid, Value v) = 0;
    virtual void printExternal(unsigned atomUid, TruthValue type) = 0;
    virtual void finishSymbols() = 0;
    virtual bool &disposeMinimize() = 0;
    virtual ~LparseOutputter() { }
};

// }}}

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_INTERFACES_HH
