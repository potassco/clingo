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

#ifndef _GRINGO_OUTPUT_LITERAL_HH
#define _GRINGO_OUTPUT_LITERAL_HH

#include <gringo/domain.hh>

namespace Gringo { namespace Output {

struct LparseOutputter;
struct LparseTranslator;
struct Statement;
struct AuxAtom;
using UStm     = std::unique_ptr<Statement>;
using UStmVec  = std::vector<UStm>;
using CSPBound = std::pair<int, int>;
using SAuxAtom         = std::shared_ptr<AuxAtom>;
using AssignmentLookup = std::function<std::pair<bool, TruthValue>(unsigned)>; // (isExternal, truthValue)

// {{{ declaration of Literal

struct Literal;
using ULit = std::unique_ptr<Literal>;
struct Literal : Clonable<Literal>, Hashable, Comparable<Literal> {
    virtual ULit negateLit(LparseTranslator &x) const;
    virtual PredicateDomain::element_type *isAtom() const { return nullptr; };
    virtual SAuxAtom isAuxAtom() const { return nullptr; };
    virtual ULit toLparse(LparseTranslator &x) = 0;
    virtual void printPlain(std::ostream &out) const = 0;
    virtual bool isIncomplete() const = 0;
    virtual int lparseUid(LparseOutputter &out) const = 0;
    virtual bool isBound(Value &value, bool negate) const { (void)value; (void)negate; return false; }
    virtual void updateBound(CSPBound &bounds, bool negate) const { (void)bounds; (void)negate; }
    virtual bool invertible() const { return false; }
    virtual void invert() { assert(false); }
    virtual bool needsSemicolon() const { return false; }
    virtual bool isPositive() const { return true; }
    virtual std::pair<bool, TruthValue> getTruth(AssignmentLookup) { return {false, TruthValue::Open}; };
    virtual ~Literal() { }
};
using LitVec  = std::vector<std::reference_wrapper<Literal>>;
using ULitVec = std::vector<ULit>;

// }}}

} } // namespace Output Gringo

GRINGO_HASH(Gringo::Output::Literal)

#endif // _GRINGO_OUTPUT_LITERAL_HH

