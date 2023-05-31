#pragma once

#include <literal.hh>

enum class AggregateFunction {
    count,
    sum,
    sump,
    min,
    max,
};

auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream &;

class SetAggregate {
  public:
    using Element = std::pair<SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    SetAggregate(ElementVec elements) : elements_{std::move(elements)} {}
    SetAggregate(ElementVec elements, Relation rel, STerm rhs)
        : elements_{std::move(elements)}, rhs_(std::make_pair(rel, std::move(rhs))) {}

    void set_rhs(STerm lhs, Relation rel);

    friend auto operator<<(std::ostream &out, SetAggregate const &aggr) -> std::ostream &;

  private:
    ElementVec elements_;
    std::optional<std::pair<STerm, Relation>> lhs_;
    std::optional<std::pair<Relation, STerm>> rhs_;
};
