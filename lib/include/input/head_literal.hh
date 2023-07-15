#pragma once

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

class HeadLiteral;
class HeadLiteralVisitor;
using SHeadLiteral = Util::shared_ptr<HeadLiteral>;
using SHeadLiteralVec = std::vector<SHeadLiteral>;

class HeadLiteral {
  public:
    virtual ~HeadLiteral() = default;

    [[nodiscard]] virtual auto print_empty() const -> bool;
    virtual void visit_variables(VarVisitFun const &fun, VariableContext ctx) const = 0;
    [[nodiscard]] virtual auto project(Projection project) const -> std::optional<SHeadLiteral> = 0;
    [[nodiscard]] virtual auto project_anonymous() const -> std::optional<SHeadLiteral> = 0;
    [[nodiscard]] virtual auto is_atom() const -> bool;
    [[nodiscard]] virtual auto is_test() const -> bool;
    [[nodiscard]] virtual auto is_classical() const -> bool;
    [[nodiscard]] virtual auto unpool() const -> std::optional<SHeadLiteralVec> = 0;

    //! Visit head literals with the given visitor.
    virtual void accept(HeadLiteralVisitor const &visitor) const = 0;

  private:
    friend void inc_ref_count(HeadLiteral &lit) { ++lit.refs_; }
    friend void dec_ref_count(HeadLiteral &lit) { ++lit.refs_; }
    [[nodiscard]] friend auto get_ref_count(HeadLiteral const &lit) -> size_t { return lit.refs_; }

    size_t refs_ = 0;
};

class Disjunction : public HeadLiteral {
  public:
    using Element = std::pair<SLiteralVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit Disjunction(ElementVec elems) : elems_{std::move(elems)} {}

    //! Get the elements of the disjunction.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }

    [[nodiscard]] auto print_empty() const -> bool override;
    [[nodiscard]] auto unpool() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto is_atom() const -> bool override;
    [[nodiscard]] auto is_test() const -> bool override;
    [[nodiscard]] auto is_classical() const -> bool override;

    void accept(HeadLiteralVisitor const &visitor) const override;

  private:
    ElementVec elems_;
};

class HeadTheoryAtom : public HeadLiteral {
  public:
    explicit HeadTheoryAtom(TheoryAtom atom) : atom_{std::move(atom)} {}

    //! Get the theory atom.
    [[nodiscard]] auto atom() const -> TheoryAtom const & { return atom_; }

    [[nodiscard]] auto unpool() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SHeadLiteral> override;

    void accept(HeadLiteralVisitor const &visitor) const override;

  private:
    TheoryAtom atom_;
};

class HeadAggregate : public HeadLiteral {
  public:
    using Element = std::tuple<TermVec, SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit HeadAggregate(LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    explicit HeadAggregate(AggregateFunction fun, ElementVec elems) : fun_(fun), elems_(std::move(elems)) {}
    explicit HeadAggregate(AggregateFunction fun, ElementVec elems, Relation rel, TermV2 rhs)
        : fun_(fun), elems_(std::move(elems)), rhs_(std::make_pair(rel, std::move(rhs))) {}

    //! Get the aggregate function.
    [[nodiscard]] auto function() const -> AggregateFunction { return fun_; }
    //! Get the aggregate elements.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }
    //! Get the left-hand-side.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! Get the right-hand-side.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

    void set_left_guard(TermV2 lhs, Relation rel);
    [[nodiscard]] auto unpool() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SHeadLiteral> override;

    void accept(HeadLiteralVisitor const &visitor) const override;

  private:
    AggregateFunction fun_;
    ElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

class HeadSetAggregate : public HeadLiteral {
  public:
    explicit HeadSetAggregate(SetAggregate aggr) : aggr_{std::move(aggr)} {}

    //! Get the set aggregate atom.
    [[nodiscard]] auto atom() const -> SetAggregate const & { return aggr_; }

    void set_left_guard(TermV2 lhs, Relation rel);
    [[nodiscard]] auto unpool() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SHeadLiteral> override;

    void accept(HeadLiteralVisitor const &visitor) const override;

  private:
    SetAggregate aggr_;
};

//! A visitor for available head literal types.
class HeadLiteralVisitor {
  public:
    //! Virtual destructor.
    virtual ~HeadLiteralVisitor() = default;

    //! Visit a disjunction.
    virtual void visit(Disjunction const &lit) const = 0;
    //! Visit a head set aggregate.
    virtual void visit(HeadSetAggregate const &lit) const = 0;
    //! Visit a head aggregate.
    virtual void visit(HeadAggregate const &lit) const = 0;
    //! Visit a theory atom in the head.
    virtual void visit(HeadTheoryAtom const &lit) const = 0;
};

} // namespace Gringo::Input
