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
    virtual void unpool(PoolHeadLiteral &pool) = 0;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, HeadLiteral const &literal) -> std::ostream &;
    auto unpool() -> SHeadLiteralVec;

    size_t refs = 0;
};

class Disjunction : public HeadLiteral {
  public:
    using Element = std::pair<SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    Disjunction(ElementVec elems) : elems_{std::move(elems)} {}

    [[nodiscard]] auto print_empty() const -> bool override;
    void print(std::ostream &out) const override;
    void unpool(PoolHeadLiteral &pool) override;

  private:
    ElementVec elems_;
};

class HeadTheoryAtom : public HeadLiteral {
  public:
    HeadTheoryAtom(TheoryAtom atom) : atom_{std::move(atom)} {}

    void print(std::ostream &out) const override;
    void unpool(PoolHeadLiteral &pool) override;

  private:
    TheoryAtom atom_;
};

class HeadAggregate : public HeadLiteral {
  public:
    using LHS = std::optional<std::pair<STerm, Relation>>;
    using RHS = std::optional<std::pair<Relation, STerm>>;
    using Element = std::tuple<STermVec, SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    HeadAggregate(AggregateFunction fun, ElementVec elems) : fun_(fun), elems_(std::move(elems)) {}
    HeadAggregate(AggregateFunction fun, ElementVec elems, Relation rel, STerm rhs)
        : fun_(fun), elems_(std::move(elems)), rhs_(std::make_pair(rel, std::move(rhs))) {}

    void set_left_guard(STerm lhs, Relation rel);
    void print(std::ostream &out) const override;
    void unpool(PoolHeadLiteral &pool) override;

  private:
    AggregateFunction fun_;
    ElementVec elems_;
    LHS lhs_;
    RHS rhs_;
};

class HeadSetAggregate : public HeadLiteral {
  public:
    HeadSetAggregate(SetAggregate aggr) : aggr_{std::move(aggr)} {}

    void set_left_guard(STerm lhs, Relation rel);
    void print(std::ostream &out) const override;
    void unpool(PoolHeadLiteral &pool) override;

  private:
    SetAggregate aggr_;
};
