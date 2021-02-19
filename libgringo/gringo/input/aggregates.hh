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

#ifndef _GRINGO_INPUT_AGGREGATES_HH
#define _GRINGO_INPUT_AGGREGATES_HH

#include <gringo/input/aggregate.hh>
#include <gringo/terms.hh>

namespace Gringo { namespace Input {

// {{{1 declaration of TupleBodyAggregate

struct TupleBodyAggregate : BodyAggregate {
    TupleBodyAggregate(NAF naf, bool removedAssignment, bool translated, AggregateFunction fun, BoundVec &&bounds, BodyAggrElemVec &&elems); // NOTE: private
    TupleBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, BodyAggrElemVec &&elems);
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    bool isAssignment() const override;
    void removeAssignment() override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    TupleBodyAggregate *clone() const override;
    void unpool(UBodyAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~TupleBodyAggregate();

    NAF naf;
    bool removedAssignment = false;
    bool translated = false;
    AggregateFunction fun;
    BoundVec bounds;
    BodyAggrElemVec elems;
};

// {{{1 declaration of LitBodyAggregate

struct LitBodyAggregate : BodyAggregate {
    LitBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems);
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    bool isAssignment() const override;
    void removeAssignment() override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    LitBodyAggregate *clone() const override;
    void unpool(UBodyAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~LitBodyAggregate();

    NAF naf;
    AggregateFunction fun;
    BoundVec bounds;
    CondLitVec elems;
};

// {{{1 declaration of Conjunction

struct Conjunction : BodyAggregate {
    using ULitVecVec = std::vector<ULitVec>;
    using Elem = std::pair<ULitVecVec, ULitVec>;
    using ElemVec = std::vector<Elem>;

    Conjunction(ULit &&head, ULitVec &&cond);
    Conjunction(ElemVec &&elems);
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    bool isAssignment() const override;
    void removeAssignment() override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    Conjunction *clone() const override;
    void unpool(UBodyAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~Conjunction();

    ElemVec elems;
};

// {{{1 declaration of SimpleBodyLiteral

struct SimpleBodyLiteral : BodyAggregate {
    SimpleBodyLiteral(ULit &&lit);
    unsigned projectScore() const override { return lit->projectScore(); }
    Location const &loc() const override;
    void loc(Location const &loc) override;
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    void removeAssignment() override;
    bool isAssignment() const override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    SimpleBodyLiteral *clone() const override;
    void unpool(UBodyAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~SimpleBodyLiteral();

    ULit lit;
};

// {{{1 declaration of DisjointAggregate

struct CSPElem {
    CSPElem(Location const &loc, UTermVec &&tuple, CSPAddTerm &&value, ULitVec &&cond);
    CSPElem(CSPElem &&x);
    CSPElem &operator=(CSPElem &&x);
    ~CSPElem();
    void print(std::ostream &out) const;
    CSPElem clone() const;
    size_t hash() const; bool operator==(CSPElem const &other) const;

    Location   loc;
    UTermVec   tuple;
    CSPAddTerm value;
    ULitVec    cond;
};

inline std::ostream &operator<<(std::ostream &out, CSPElem const &x) {
    x.print(out);
    return out;
}

using CSPElemVec = std::vector<CSPElem>;

struct DisjointAggregate : BodyAggregate {
    DisjointAggregate(NAF naf, CSPElemVec &&elems);
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    bool isAssignment() const override;
    void removeAssignment() override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    DisjointAggregate *clone() const override;
    void unpool(UBodyAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~DisjointAggregate();

    NAF        naf;
    CSPElemVec elems;
};

// }}}1

// {{{1 declaration of TupleHeadAggregate

struct TupleHeadAggregate : HeadAggregate {
    TupleHeadAggregate(AggregateFunction fun, bool translated, BoundVec &&bounds, HeadAggrElemVec &&elems); // NOTE: private
    TupleHeadAggregate(AggregateFunction fun, BoundVec &&bounds, HeadAggrElemVec &&elems);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    TupleHeadAggregate *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~TupleHeadAggregate();

    AggregateFunction fun;
    bool translated;
    BoundVec bounds;
    HeadAggrElemVec elems;
};

// {{{1 declaration of LitHeadAggregate

struct LitHeadAggregate : HeadAggregate {
    LitHeadAggregate(AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    LitHeadAggregate *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~LitHeadAggregate();

    AggregateFunction fun;
    BoundVec bounds;
    CondLitVec elems;
};

// {{{1 declaration of Disjunction

struct Disjunction : HeadAggregate {
    using Head = std::pair<ULit, ULitVec>;
    using HeadVec = std::vector<Head>;
    using Elem = std::pair<HeadVec, ULitVec>;
    using ElemVec = std::vector<Elem>;

    Disjunction(CondLitVec &&elems);
    Disjunction(ElemVec &&elems);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    Disjunction *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~Disjunction();

    ElemVec elems;
};

// {{{1 declaration of SimpleHeadLiteral

struct SimpleHeadLiteral : HeadAggregate {
    SimpleHeadLiteral(ULit &&lit);
    virtual bool isPredicate() const override { return true; }
    Location const &loc() const override;
    void loc(Location const &loc) override;
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    SimpleHeadLiteral *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    Symbol isEDB() const override;
    virtual ~SimpleHeadLiteral();

    ULit lit;
};

// }}}1
// {{{1 declaration of MinimizeHeadLiteral

struct MinimizeHeadLiteral : HeadAggregate {
    MinimizeHeadLiteral(UTerm &&weight, UTerm &&priority, UTermVec &&tuple);
    MinimizeHeadLiteral(UTermVec &&tuple);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    void printWithCondition(std::ostream &out, UBodyAggrVec const &condition) const override;
    size_t hash() const override;
    MinimizeHeadLiteral *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~MinimizeHeadLiteral();

private:
    Term &weight() const;
    Term &priority() const;

    UTermVec tuple_;
};

// }}}1
// {{{1 declaration of EdgeHeadAtom

struct EdgeHeadAtom : HeadAggregate {
    EdgeHeadAtom(UTerm &&u, UTerm &&v);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    EdgeHeadAtom *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~EdgeHeadAtom();

private:
    UTerm u_;
    UTerm v_;
};

// {{{1 declaration of ProjectHeadAtom

struct ProjectHeadAtom : HeadAggregate {
    ProjectHeadAtom(UTerm &&atom);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    ProjectHeadAtom *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~ProjectHeadAtom();

private:
    UTerm atom_;
};

// {{{1 declaration of ExternalHeadAtom

struct ExternalHeadAtom : HeadAggregate {
    ExternalHeadAtom(UTerm &&atom, UTerm &&type);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    void printWithCondition(std::ostream &out, UBodyAggrVec const &condition) const override;
    size_t hash() const override;
    ExternalHeadAtom *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~ExternalHeadAtom();

private:
    UTerm atom_;
    UTerm type_;
};

// {{{1 declaration of HeuristicHeadAtom

struct HeuristicHeadAtom : HeadAggregate {
    HeuristicHeadAtom(UTerm &&atom, UTerm &&bias, UTerm &&priority, UTerm &&mod);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    HeuristicHeadAtom *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~HeuristicHeadAtom();

private:
    UTerm atom_;
    UTerm value_;
    UTerm priority_;
    UTerm mod_;
};

// {{{1 declaration of ShowHeadLiteral

struct ShowHeadLiteral : HeadAggregate {
    ShowHeadLiteral(UTerm &&term, bool csp);
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    ShowHeadLiteral *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    virtual ~ShowHeadLiteral();

private:
    UTerm term_;
    bool csp_;
};

// }}}1

} } // namespace Input Gringo

GRINGO_CALL_HASH(Gringo::Input::CSPElem)
GRINGO_CALL_CLONE(Gringo::Input::CSPElem)

#endif // _GRINGO_INPUT_AGGREGATES_HH

