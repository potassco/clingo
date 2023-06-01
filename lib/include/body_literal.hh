#pragma once

#include <tuple>

#include <aggregate.hh>
#include <literal.hh>
#include <theory.hh>

class BodyLiteral {
  public:
    virtual ~BodyLiteral() = default;

    virtual void add_sign(Sign sign) = 0;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, BodyLiteral const &literal) -> std::ostream &;

    size_t refs = 0;
};

using SBodyLiteral = shared_ptr<BodyLiteral>;
using SBodyLiteralVec = std::vector<SBodyLiteral>;

class ConditionalLiteral : public BodyLiteral {
  public:
    ConditionalLiteral(SLiteral lit, SLiteralVec cond) : lit_{std::move(lit)}, cond_{std::move(cond)} {}

    void add_sign(Sign sign) override;
    void print(std::ostream &out) const override;

  private:
    SLiteral lit_;
    SLiteralVec cond_;
};

class BodyAggregate : public BodyLiteral {
  public:
    using Element = std::tuple<STermVec, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    BodyAggregate(AggregateFunction fun, ElementVec elems) : fun_(fun), elements(std::move(elems)) {}
    BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, STerm rhs)
        : fun_(fun), elements(std::move(elems)), rhs_(std::make_pair(rel, std::move(rhs))) {}

    void add_sign(Sign sign) override;
    void set_left_guard(STerm lhs, Relation rel);
    void print(std::ostream &out) const override;

  private:
    Sign sign_ = Sign::none;
    AggregateFunction fun_;
    ElementVec elements;
    std::optional<std::pair<STerm, Relation>> lhs_;
    std::optional<std::pair<Relation, STerm>> rhs_;
};

class BodySetAggregate : public BodyLiteral {
  public:
    BodySetAggregate(SetAggregate aggr) : aggr_{std::move(aggr)} {}

    void add_sign(Sign sign) override;
    void set_left_guard(STerm lhs, Relation rel);
    void print(std::ostream &out) const override;

  private:
    Sign sign_ = Sign::none;
    SetAggregate aggr_;
};

class BodyTheoryAtom : public BodyLiteral {
  public:
    BodyTheoryAtom(TheoryAtom atom) : atom_(std::move(atom)) {}

    void add_sign(Sign sign) override;
    void print(std::ostream &out) const override;

  private:
    Sign sign_ = Sign::none;
    TheoryAtom atom_;
};
