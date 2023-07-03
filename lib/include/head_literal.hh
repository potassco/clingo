#pragma once

#include <tuple>

#include <aggregate.hh>
#include <literal.hh>
#include <theory.hh>

class HeadLiteral;
using SHeadLiteral = shared_ptr<HeadLiteral>;
using SHeadLiteralVec = std::vector<SHeadLiteral>;
using PoolHeadLiteral = PoolParent<SHeadLiteral, PoolLiteral>;

class HeadLiteral {
  public:
    virtual ~HeadLiteral() = default;

    [[nodiscard]] virtual auto print_empty() const -> bool;
    virtual void unpool(PoolHeadLiteral &pool) const;
    virtual void print(std::ostream &out) const = 0;
    virtual void visit_variables(VarVisitFun const &fun, VariableContext ctx) const = 0;
    [[nodiscard]] virtual auto project(Projection project) const -> std::optional<SHeadLiteral> = 0;
    [[nodiscard]] virtual auto is_atom() const -> bool;
    [[nodiscard]] virtual auto is_test() const -> bool;
    [[nodiscard]] virtual auto is_classical() const -> bool;
    [[nodiscard]] virtual auto rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> = 0;

    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, HeadLiteral const &literal) -> std::ostream &;
    [[nodiscard]] auto unpool() const -> SHeadLiteralVec;
    [[nodiscard]] virtual auto unpool_v2() const -> std::optional<SHeadLiteralVec> = 0;

    size_t refs = 0;
};

class Disjunction : public HeadLiteral {
  public:
    using Element = std::pair<SLiteralVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit Disjunction(ElementVec elems) : elems_{std::move(elems)} {}

    [[nodiscard]] auto print_empty() const -> bool override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto unpool_v2() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto is_atom() const -> bool override;
    [[nodiscard]] auto is_test() const -> bool override;
    [[nodiscard]] auto is_classical() const -> bool override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> override;

  private:
    ElementVec elems_;
};

class HeadTheoryAtom : public HeadLiteral {
  public:
    explicit HeadTheoryAtom(TheoryAtom atom) : atom_{std::move(atom)} {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto unpool_v2() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> override;

  private:
    TheoryAtom atom_;
};

class HeadAggregate : public HeadLiteral {
  public:
    using Element = std::tuple<STermVec, SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit HeadAggregate(LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    explicit HeadAggregate(AggregateFunction fun, ElementVec elems) : fun_(fun), elems_(std::move(elems)) {}
    explicit HeadAggregate(AggregateFunction fun, ElementVec elems, Relation rel, STerm rhs)
        : fun_(fun), elems_(std::move(elems)), rhs_(std::make_pair(rel, std::move(rhs))) {}

    void set_left_guard(STerm lhs, Relation rel);
    void print(std::ostream &out) const override;
    [[nodiscard]] auto unpool_v2() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> override;

  private:
    AggregateFunction fun_;
    ElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

class HeadSetAggregate : public HeadLiteral {
  public:
    explicit HeadSetAggregate(SetAggregate aggr) : aggr_{std::move(aggr)} {}

    void set_left_guard(STerm lhs, Relation rel);
    void print(std::ostream &out) const override;
    [[nodiscard]] auto unpool_v2() const -> std::optional<SHeadLiteralVec> override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SHeadLiteral> override;
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> override;

  private:
    SetAggregate aggr_;
};
