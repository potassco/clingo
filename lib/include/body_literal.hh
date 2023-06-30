#pragma once

#include <tuple>

#include <aggregate.hh>
#include <literal.hh>
#include <theory.hh>

class BodyLiteral;
using SBodyLiteral = shared_ptr<BodyLiteral>;
using SBodyLiteralVec = std::vector<SBodyLiteral>;
using PoolBodyLiteral = PoolParent<SBodyLiteral, PoolLiteral>;

class BodyLiteral {
  public:
    virtual ~BodyLiteral() = default;

    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, BodyLiteral const &literal) -> std::ostream &;
    auto unpool() -> SBodyLiteralVec;

    virtual void add_sign(Sign sign) = 0;
    virtual void unpool(PoolBodyLiteral &pool) = 0;
    virtual void print(std::ostream &out) const = 0;
    virtual void visit_variables(VarVisitFun const &fun, VariableContext ctx) const = 0;
    [[nodiscard]] virtual auto project(Projection project, bool in_negative_scope) -> SBodyLiteral = 0;
    [[nodiscard]] virtual auto is_atom() const -> bool;
    [[nodiscard]] virtual auto is_test() const -> bool;

    size_t refs = 0;
};

class Conjunction : public BodyLiteral {
  public:
    using Element = std::pair<SLiteralVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit Conjunction(ElementVec elems) : elems_{std::move(elems)} {}

    void add_sign(Sign sign) override;
    void unpool(PoolBodyLiteral &pool) override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_negative_scope) -> SBodyLiteral override;
    [[nodiscard]] auto is_atom() const -> bool override;
    [[nodiscard]] auto is_test() const -> bool override;

  private:
    ElementVec elems_;
};

class BodyAggregate : public BodyLiteral {
  public:
    using Element = std::tuple<STermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit BodyAggregate(Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : sign_{sign}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems) : fun_(fun), elems_(std::move(elems)) {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, STerm rhs)
        : fun_(fun), elems_(std::move(elems)), rhs_(std::make_pair(rel, std::move(rhs))) {}

    void add_sign(Sign sign) override;
    void set_left_guard(STerm lhs, Relation rel);
    void unpool(PoolBodyLiteral &pool) override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_negative_scope) -> SBodyLiteral override;

  private:
    Sign sign_ = Sign::none;
    AggregateFunction fun_;
    ElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

class BodySetAggregate : public BodyLiteral {
  public:
    explicit BodySetAggregate(Sign sign, SetAggregate aggr) : sign_{sign}, aggr_{std::move(aggr)} {}
    explicit BodySetAggregate(SetAggregate aggr) : aggr_{std::move(aggr)} {}

    void add_sign(Sign sign) override;
    void set_left_guard(STerm lhs, Relation rel);
    void unpool(PoolBodyLiteral &pool) override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_negative_scope) -> SBodyLiteral override;

  private:
    Sign sign_ = Sign::none;
    SetAggregate aggr_;
};

class BodyTheoryAtom : public BodyLiteral {
  public:
    explicit BodyTheoryAtom(TheoryAtom atom) : atom_(std::move(atom)) {}
    explicit BodyTheoryAtom(Sign sign, TheoryAtom atom) : sign_{sign}, atom_(std::move(atom)) {}

    void add_sign(Sign sign) override;
    void unpool(PoolBodyLiteral &pool) override;
    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto project(Projection project, bool in_negative_scope) -> SBodyLiteral override;

  private:
    Sign sign_ = Sign::none;
    TheoryAtom atom_;
};
