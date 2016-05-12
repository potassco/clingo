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

#ifndef _GRINGO_INPUT_THEORY_HH
#define _GRINGO_INPUT_THEORY_HH

#include <gringo/terms.hh>
#include <gringo/output/theory.hh>
#include <gringo/input/aggregate.hh>

namespace Gringo {

namespace Ground {

class TheoryAccumulate;
class TheoryComplete;
class TheoryLiteral;

} // namespace Ground

namespace Input {

// {{{1 declaration of TheoryElement

class TheoryElement;
using TheoryElementVec = std::vector<TheoryElement>;
class TheoryElement {
public:
    TheoryElement(TheoryElement &&);
    TheoryElement &operator=(TheoryElement &&);
    TheoryElement(Output::UTheoryTermVec &&tuple, ULitVec &&cond);
    ~TheoryElement() noexcept;
    TheoryElement clone() const;
    void print(std::ostream &out) const;
    bool operator==(TheoryElement const &other) const;
    size_t hash() const;
    void unpool(TheoryElementVec &elems, bool beforeRewrite);
    bool hasPool(bool beforeRewrite) const;
    void replace(Defines &x);
    void collect(VarTermBoundVec &vars) const;
    void assignLevels(AssignLevel &lvl);
    void check(Location const &loc, Printable const &p, ChkLvlVec &levels) const;
    bool simplify(Projections &project, SimplifyState &state);
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    void initTheory(Output::TheoryParser &p);
    std::unique_ptr<Ground::TheoryAccumulate> toGround(ToGroundArg &x, Ground::TheoryComplete &completeRef, Ground::ULitVec &&lits) const;

private:
    Output::UTheoryTermVec tuple_;
    ULitVec cond_;
};
inline std::ostream &operator<<(std::ostream &out, TheoryElement const &elem) { elem.print(out); return out; }

// {{{1 declaration of TheoryAtom

class TheoryAtom {
public:
    TheoryAtom(TheoryAtom &&);
    TheoryAtom &operator=(TheoryAtom &&);
    // Note: name must be a term that (after unpooling) has a signature
    TheoryAtom(UTerm &&name, TheoryElementVec &&elems);
    TheoryAtom(UTerm &&name, TheoryElementVec &&elems, FWString op, Output::UTheoryTerm &&guard, TheoryAtomType type = TheoryAtomType::Any);
    ~TheoryAtom() noexcept;
    TheoryAtom clone() const;
    bool operator==(TheoryAtom const &other) const;
    void print(std::ostream &out) const;
    bool hasGuard() const;
    size_t hash() const;
    template <class T>
    void unpool(T f, bool beforeRewrite);
    bool hasPool(bool beforeRewrite) const;
    void replace(Defines &x);
    void collect(VarTermBoundVec &vars) const;
    void assignLevels(AssignLevel &lvl);
    void check(Location const &loc, Printable const &p, ChkLvlVec &levels) const;
    bool simplify(Projections &project, SimplifyState &state);
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    void initTheory(Location const &loc, TheoryDefs &def, bool inBody, bool hasBody);
    TheoryAtomType type() const { return type_; }
    CreateBody toGroundBody(ToGroundArg &x, Ground::UStmVec &stms, NAF naf, UTerm &&id) const;
    CreateHead toGroundHead() const;

private:
    UTerm            name_;
    TheoryElementVec elems_;
    FWString         op_;
    Output::UTheoryTerm guard_;
    TheoryAtomType   type_ = TheoryAtomType::Any;
};
inline std::ostream &operator<<(std::ostream &out, TheoryAtom const &atom) { atom.print(out); return out; }

// {{{1 declaration of BodyTheoryLiteral

class BodyTheoryLiteral : public BodyAggregate {
public:
    BodyTheoryLiteral(NAF naf, TheoryAtom &&atom, bool rewritten = false);
    virtual ~BodyTheoryLiteral() noexcept;
    // {{{2 BodyAggregate interface
    void unpool(UBodyAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton) override;
    void assignLevels(AssignLevel &lvl) override;
    void check(ChkLvlVec &lvl) const override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) override;
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    void removeAssignment() override;
    bool isAssignment() const override;
    void collect(VarTermBoundVec &vars) const override;
    bool hasPool(bool beforeRewrite) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    void initTheory(TheoryDefs &def) override;
    // {{{2 Hashable
    size_t hash() const override;
    // {{{2 Printable
    void print(std::ostream &out) const override;
    // {{{2 Clonable
    BodyTheoryLiteral *clone() const override;
    // {{{2 Comparable interface
    bool operator==(BodyAggregate const &other) const override;
    // }}}2

private:
    TheoryAtom atom_;
    NAF naf_;
    bool rewritten_;
};

// }}}1

// {{{1 declaration of HeadTheoryLiteral

class HeadTheoryLiteral : public HeadAggregate {
public:
    HeadTheoryLiteral(TheoryAtom &&atom, bool rewritten = false);
    virtual ~HeadTheoryLiteral() noexcept;
    // {{{2 HeadAggregate interface
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl) const override;
    void replace(Defines &x) override;
    void initTheory(TheoryDefs &def, bool hasBody) override;
    // {{{2 Hashable
    size_t hash() const override;
    // {{{2 Printable
    void print(std::ostream &out) const override;
    // {{{2 Clonable
    // precondition: must not be called after rewriteAggregates
    HeadTheoryLiteral *clone() const override;
    // {{{2 Comparable interface
    bool operator==(HeadAggregate const &other) const override;
    // }}}2

private:
    TheoryAtom atom_;
    bool rewritten_;
};

} } // namespace Gringo Input

GRINGO_CALL_HASH(Gringo::Input::TheoryAtom)
GRINGO_CALL_HASH(Gringo::Input::TheoryElement)
GRINGO_CALL_CLONE(Gringo::Input::TheoryElement)
GRINGO_CALL_CLONE(Gringo::Input::TheoryAtom)

#endif // _GRINGO_INPUT_THEORY_HH
