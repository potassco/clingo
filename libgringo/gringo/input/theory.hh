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
    void check(Location const &loc, Printable const &p, ChkLvlVec &levels, Logger &log) const;
    bool simplify(Projections &project, SimplifyState &state, Logger &log);
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    void initTheory(Output::TheoryParser &p, Logger &log);
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
    TheoryAtom(UTerm &&name, TheoryElementVec &&elems, String op, Output::UTheoryTerm &&guard, TheoryAtomType type = TheoryAtomType::Any);
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
    void check(Location const &loc, Printable const &p, ChkLvlVec &levels, Logger &log) const;
    bool simplify(Projections &project, SimplifyState &state, Logger &log);
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen);
    void initTheory(Location const &loc, TheoryDefs &def, bool inBody, bool hasBody, Logger &log);
    TheoryAtomType type() const { return type_; }
    CreateBody toGroundBody(ToGroundArg &x, Ground::UStmVec &stms, NAF naf, UTerm &&id) const;
    CreateHead toGroundHead() const;

private:
    UTerm               name_;
    TheoryElementVec    elems_;
    String              op_;
    Output::UTheoryTerm guard_;
    TheoryAtomType      type_ = TheoryAtomType::Any;
};
inline std::ostream &operator<<(std::ostream &out, TheoryAtom const &atom) { atom.print(out); return out; }

// {{{1 declaration of BodyTheoryLiteral

class BodyTheoryLiteral : public BodyAggregate {
public:
    BodyTheoryLiteral(NAF naf, TheoryAtom &&atom, bool rewritten = false);
    virtual ~BodyTheoryLiteral() noexcept;
    // {{{2 BodyAggregate interface
    void unpool(UBodyAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) override;
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    void removeAssignment() override;
    bool isAssignment() const override;
    void collect(VarTermBoundVec &vars) const override;
    bool hasPool(bool beforeRewrite) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    void initTheory(TheoryDefs &def, Logger &log) override;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    void getNeg(std::function<void (Sig)> f) const override;
    void initTheory(TheoryDefs &def, bool hasBody, Logger &log) override;
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
