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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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
    size_t hash() const override;
    MinimizeHeadLiteral *clone() const override;
    void unpool(UHeadAggrVec &x, bool beforeRewrite) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    bool hasPool(bool beforeRewrite) const override;
    void check(ChkLvlVec &lvl, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
    virtual ~ProjectHeadAtom();

private:
    UTerm atom_;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const override;
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

