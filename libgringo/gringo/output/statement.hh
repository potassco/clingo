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

#ifndef _GRINGO_OUTPUT_STATEMENT_HH
#define _GRINGO_OUTPUT_STATEMENT_HH

#include <gringo/output/literal.hh>
#include <gringo/locatable.hh>
#include <gringo/value.hh>
#include <gringo/domain.hh>

namespace Gringo { namespace Output {

// {{{ declaration of Statement

struct LparseOutputter;
struct LparseTranslator;
struct Statement;
struct AuxAtom;
struct DisjointElem;

using SAuxAtom         = std::shared_ptr<AuxAtom>;
using UStm             = std::unique_ptr<Statement>;
using UStmVec          = std::vector<UStm>;
using MinimizeList     = std::vector<std::pair<FWValVec, ULitVec>>;
using CoefVarVec       = std::vector<std::pair<int, Value>>;
using DisjointCons     = std::vector<std::pair<FWValVec, std::vector<DisjointElem>>>;
using OutputPredicates = std::vector<std::tuple<Location, FWSignature, bool>>;

struct LparseTranslator {
    ULit negate(ULit &lit);
    virtual bool isAtomFromPreviousStep(ULit const &lit) = 0;
    virtual void addMinimize(MinimizeList &&x) = 0;
    virtual void addBounds(Value value, std::vector<CSPBound> bounds) = 0;
    virtual void addLinearConstraint(SAuxAtom head, CoefVarVec &&vars, int bound) = 0;
    virtual void addDisjointConstraint(SAuxAtom head, DisjointCons &&elem) = 0;
    virtual unsigned auxAtom() = 0;
    virtual void translate() = 0;
    virtual void outputSymbols(LparseOutputter &out, OutputPredicates const &outPreds) = 0;
    virtual void operator()(Statement &x) = 0;
    virtual bool minimizeChanged() const = 0;
    virtual ULit makeAux(NAF naf=NAF::POS) = 0;
    virtual ULit getTrueLit() = 0;
    virtual void simplify(AssignmentLookup assignment) = 0;
    virtual ~LparseTranslator() { }
};

struct Statement : Clonable<Statement> {
    virtual void toLparse(LparseTranslator &trans) = 0;
    virtual void printPlain(std::ostream &out) const = 0;
    virtual void printLparse(LparseOutputter &out) const = 0;
    virtual bool isIncomplete() const = 0;
    virtual ~Statement() { }
};

// }}}

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_STATEMENT_HH

