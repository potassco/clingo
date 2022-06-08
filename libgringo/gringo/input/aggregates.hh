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

#ifndef GRINGO_INPUT_AGGREGATES_HH
#define GRINGO_INPUT_AGGREGATES_HH

#include <gringo/input/aggregate.hh>
#include <gringo/terms.hh>
#include <ostream>

namespace Gringo { namespace Input {

// {{{1 declaration of AggregateElement

class BodyAggrElem : public IEContext {
public:
    BodyAggrElem(UTermVec tuple, ULitVec condition)
    : tuple_{std::move(tuple)}
    , condition_{std::move(condition)} { }

    bool hasPool() const;
    void unpool(BodyAggrElemVec &pool);
    bool hasUnpoolComparison() const;
    void unpoolComparison(BodyAggrElemVec &elems) const;
    void collect(VarTermBoundVec &vars, bool tupleOnly = false) const;
    bool simplify(Projections &project, SimplifyState &state, Logger &log);
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen);
    void check(ChkLvlVec &levels) const;
    void replace(Defines &defs);
    template <class T, class C>
    std::unique_ptr<T> toGround(ToGroundArg &x, C &completeRef, Ground::ULitVec &&lits) const;

    void gatherIEs(IESolver &solver) const override;
    void addIEBounds(IESolver const &solver, IEBoundMap const &bounds) override;

    friend std::ostream &operator<<(std::ostream &out, BodyAggrElem const &elem);
    friend bool operator==(BodyAggrElem const &a, BodyAggrElem const &b);
    friend size_t get_value_hash(BodyAggrElem const &elem);
    friend BodyAggrElem get_clone(BodyAggrElem const &elem);

    UTermVec tuple_;
    ULitVec condition_;
};

// {{{1 declaration of TupleBodyAggregate

class TupleBodyAggregate : public BodyAggregate {
public:
    TupleBodyAggregate(NAF naf, bool removedAssignment, bool translated, AggregateFunction fun, BoundVec &&bounds, BodyAggrElemVec &&elems); // NOTE: private
    TupleBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, BodyAggrElemVec &&elems);

    void addToSolver(IESolver &solver) override;
    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    bool isAssignment() const override;
    void removeAssignment() override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    TupleBodyAggregate *clone() const override;
    bool hasPool() const override;
    void unpool(UBodyAggrVec &x) override;
    bool hasUnpoolComparison() const override;
    UBodyAggrVecVec unpoolComparison() const override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    NAF naf_;
    bool removedAssignment_ = false;
    bool translated_ = false;
    AggregateFunction fun_;
    BoundVec bounds_;
    BodyAggrElemVec elems_;
};

// {{{1 declaration of LitBodyAggregate

class LitBodyAggregate : public BodyAggregate {
public:
    LitBodyAggregate(NAF naf, AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems);
    LitBodyAggregate(LitBodyAggregate const &other) = delete;
    LitBodyAggregate(LitBodyAggregate &&other) noexcept = default;
    LitBodyAggregate &operator=(LitBodyAggregate const &other) = delete;
    LitBodyAggregate &operator=(LitBodyAggregate &&other) noexcept = default;
    ~LitBodyAggregate() noexcept override;

    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    bool isAssignment() const override;
    void removeAssignment() override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    LitBodyAggregate *clone() const override;
    bool hasPool() const override;
    void unpool(UBodyAggrVec &x) override;
    bool hasUnpoolComparison() const override;
    UBodyAggrVecVec unpoolComparison() const override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    NAF naf_;
    AggregateFunction fun_;
    BoundVec bounds_;
    CondLitVec elems_;
};

// {{{1 declaration of Conjunction

class Conjunction : public BodyAggregate {
public:
    using ULitVecVec = std::vector<ULitVec>;
    using Elem = std::pair<ULitVecVec, ULitVec>;
    using ElemVec = std::vector<Elem>;

    Conjunction(ULit &&head, ULitVec &&cond);
    Conjunction(ElemVec &&elems);
    Conjunction(Conjunction const &other) = delete;
    Conjunction(Conjunction &&other) noexcept = default;
    Conjunction &operator=(Conjunction const &other) = delete;
    Conjunction &operator=(Conjunction &&other) noexcept = default;
    ~Conjunction() noexcept override;

    bool rewriteAggregates(UBodyAggrVec &aggr) override;
    bool isAssignment() const override;
    void removeAssignment() override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(BodyAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    Conjunction *clone() const override;
    bool hasPool() const override;
    void unpool(UBodyAggrVec &x) override;
    bool hasUnpoolComparison() const override;
    UBodyAggrVecVec unpoolComparison() const override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    ElemVec elems_;
};

// {{{1 declaration of SimpleBodyLiteral

class SimpleBodyLiteral : public BodyAggregate {
public:
    SimpleBodyLiteral(ULit &&lit);
    SimpleBodyLiteral(SimpleBodyLiteral const &other) = delete;
    SimpleBodyLiteral(SimpleBodyLiteral &&other) noexcept = default;
    SimpleBodyLiteral &operator=(SimpleBodyLiteral const &other) = delete;
    SimpleBodyLiteral &operator=(SimpleBodyLiteral &&other) noexcept = default;
    ~SimpleBodyLiteral() noexcept override;

    void addToSolver(IESolver &solver) override;
    unsigned projectScore() const override { return lit_->projectScore(); }
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
    bool hasPool() const override;
    void unpool(UBodyAggrVec &x) override;
    bool hasUnpoolComparison() const override;
    UBodyAggrVecVec unpoolComparison() const override;
    bool simplify(Projections &project, SimplifyState &state, bool singleton, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::RelationVec &assign, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    ULit lit_;
};

// }}}1

// {{{1 declaration of TupleHeadAggregate

class TupleHeadAggregate : public HeadAggregate {
public:
    TupleHeadAggregate(AggregateFunction fun, bool translated, BoundVec &&bounds, HeadAggrElemVec &&elems);
    TupleHeadAggregate(AggregateFunction fun, BoundVec &&bounds, HeadAggrElemVec &&elems);
    TupleHeadAggregate(TupleHeadAggregate const &other) = delete;
    TupleHeadAggregate(TupleHeadAggregate &&other) noexcept = default;
    TupleHeadAggregate &operator=(TupleHeadAggregate const &other) = delete;
    TupleHeadAggregate &operator=(TupleHeadAggregate &&other) noexcept = default;
    ~TupleHeadAggregate() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    TupleHeadAggregate *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    AggregateFunction fun_;
    bool translated_;
    BoundVec bounds_;
    HeadAggrElemVec elems_;
};

// {{{1 declaration of LitHeadAggregate

class LitHeadAggregate : public HeadAggregate {
public:
    LitHeadAggregate(AggregateFunction fun, BoundVec &&bounds, CondLitVec &&elems);
    LitHeadAggregate(LitHeadAggregate const &other) = delete;
    LitHeadAggregate(LitHeadAggregate &&other) noexcept = default;
    LitHeadAggregate &operator=(LitHeadAggregate const &other) = delete;
    LitHeadAggregate &operator=(LitHeadAggregate &&other) noexcept = default;
    ~LitHeadAggregate() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    LitHeadAggregate *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    AggregateFunction fun_;
    BoundVec bounds_;
    CondLitVec elems_;
};

// {{{1 declaration of Disjunction

class Disjunction : public HeadAggregate {
public:
    using Head = std::pair<ULit, ULitVec>;
    using HeadVec = std::vector<Head>;
    using Elem = std::pair<HeadVec, ULitVec>;
    using ElemVec = std::vector<Elem>;

    Disjunction(CondLitVec &&elems);
    Disjunction(ElemVec &&elems);
    Disjunction(Disjunction const &other) = delete;
    Disjunction(Disjunction &&other) noexcept = default;
    Disjunction &operator=(Disjunction const &other) = delete;
    Disjunction &operator=(Disjunction &&other) noexcept = default;
    ~Disjunction() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    Disjunction *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    ElemVec elems_;
};

// {{{1 declaration of SimpleHeadLiteral

class SimpleHeadLiteral : public HeadAggregate {
public:
    SimpleHeadLiteral(ULit &&lit);
    SimpleHeadLiteral(SimpleHeadLiteral const &other) = delete;
    SimpleHeadLiteral(SimpleHeadLiteral &&other) noexcept = default;
    SimpleHeadLiteral &operator=(SimpleHeadLiteral const &other) = delete;
    SimpleHeadLiteral &operator=(SimpleHeadLiteral &&other) noexcept = default;
    ~SimpleHeadLiteral() noexcept override;

    void addToSolver(IESolver &solver) override;
    bool isPredicate() const override { return true; }
    Location const &loc() const override;
    void loc(Location const &loc) override;
    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    SimpleHeadLiteral *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &dx) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;
    Symbol isEDB() const override;

private:
    ULit lit_;
};

// }}}1
// {{{1 declaration of MinimizeHeadLiteral

class MinimizeHeadLiteral : public HeadAggregate {
public:
    MinimizeHeadLiteral(UTerm &&weight, UTerm &&priority, UTermVec &&tuple);
    MinimizeHeadLiteral(UTermVec &&tuple);
    MinimizeHeadLiteral(MinimizeHeadLiteral const &other) = delete;
    MinimizeHeadLiteral(MinimizeHeadLiteral &&other) noexcept = default;
    MinimizeHeadLiteral &operator=(MinimizeHeadLiteral const &other) = delete;
    MinimizeHeadLiteral &operator=(MinimizeHeadLiteral &&other) noexcept = default;
    ~MinimizeHeadLiteral() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    void printWithCondition(std::ostream &out, UBodyAggrVec const &condition) const override;
    size_t hash() const override;
    MinimizeHeadLiteral *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    Term &weight() const;
    Term &priority() const;

    UTermVec tuple_;
};

// }}}1
// {{{1 declaration of EdgeHeadAtom

class EdgeHeadAtom : public HeadAggregate {
public:
    EdgeHeadAtom(UTerm &&u, UTerm &&v);
    EdgeHeadAtom(EdgeHeadAtom const &other) = delete;
    EdgeHeadAtom(EdgeHeadAtom &&other) noexcept = default;
    EdgeHeadAtom &operator=(EdgeHeadAtom const &other) = delete;
    EdgeHeadAtom &operator=(EdgeHeadAtom &&other) noexcept = default;
    ~EdgeHeadAtom() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    EdgeHeadAtom *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    UTerm u_;
    UTerm v_;
};

// {{{1 declaration of ProjectHeadAtom

class ProjectHeadAtom : public HeadAggregate {
public:
    ProjectHeadAtom(UTerm &&atom);
    ProjectHeadAtom(ProjectHeadAtom const &other) = delete;
    ProjectHeadAtom(ProjectHeadAtom &&other) noexcept = default;
    ProjectHeadAtom &operator=(ProjectHeadAtom const &other) = delete;
    ProjectHeadAtom &operator=(ProjectHeadAtom &&other) noexcept = default;
    ~ProjectHeadAtom() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    ProjectHeadAtom *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    UTerm atom_;
};

// {{{1 declaration of ExternalHeadAtom

class ExternalHeadAtom : public HeadAggregate {
public:
    ExternalHeadAtom(UTerm &&atom, UTerm &&type);
    ExternalHeadAtom(ExternalHeadAtom const &other) = delete;
    ExternalHeadAtom(ExternalHeadAtom &&other) noexcept = default;
    ExternalHeadAtom &operator=(ExternalHeadAtom const &other) = delete;
    ExternalHeadAtom &operator=(ExternalHeadAtom &&other) noexcept = default;
    ~ExternalHeadAtom() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    void printWithCondition(std::ostream &out, UBodyAggrVec const &condition) const override;
    size_t hash() const override;
    ExternalHeadAtom *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    UTerm atom_;
    UTerm type_;
};

// {{{1 declaration of HeuristicHeadAtom

class HeuristicHeadAtom : public HeadAggregate {
public:
    HeuristicHeadAtom(UTerm &&atom, UTerm &&bias, UTerm &&priority, UTerm &&mod);
    HeuristicHeadAtom(HeuristicHeadAtom const &other) = delete;
    HeuristicHeadAtom(HeuristicHeadAtom &&other) noexcept = default;
    HeuristicHeadAtom &operator=(HeuristicHeadAtom const &other) = delete;
    HeuristicHeadAtom &operator=(HeuristicHeadAtom &&other) noexcept = default;
    ~HeuristicHeadAtom() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    HeuristicHeadAtom *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    UTerm atom_;
    UTerm value_;
    UTerm priority_;
    UTerm mod_;
};

// {{{1 declaration of ShowHeadLiteral

class ShowHeadLiteral : public HeadAggregate {
public:
    ShowHeadLiteral(UTerm &&term);
    ShowHeadLiteral(ShowHeadLiteral const &other) = delete;
    ShowHeadLiteral(ShowHeadLiteral &&other) noexcept = default;
    ShowHeadLiteral &operator=(ShowHeadLiteral const &other) = delete;
    ShowHeadLiteral &operator=(ShowHeadLiteral &&other) noexcept = default;
    ~ShowHeadLiteral() noexcept override;

    UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) override;
    void collect(VarTermBoundVec &vars) const override;
    bool operator==(HeadAggregate const &other) const override;
    void print(std::ostream &out) const override;
    size_t hash() const override;
    ShowHeadLiteral *clone() const override;
    bool hasPool() const override;
    void unpool(UHeadAggrVec &x) override;
    UHeadAggr unpoolComparison(UBodyAggrVec &body) override;
    bool simplify(Projections &project, SimplifyState &state, Logger &log) override;
    void assignLevels(AssignLevel &lvl) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) override;
    void check(ChkLvlVec &levels, Logger &log) const override;
    void replace(Defines &x) override;
    CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms) const override;

private:
    UTerm term_;
};

// }}}1

} } // namespace Input Gringo

#endif // GRINGO_INPUT_AGGREGATES_HH
