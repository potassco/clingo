#pragma once

#include <functional>
#include <optional>

#include <input/literal.hh>

namespace Gringo::Input {

enum class AggregateFunction {
    count,
    sum,
    sump,
    min,
    max,
};

using LGuard = std::optional<std::pair<Term, Relation>>;
using RGuard = std::optional<std::pair<Relation, Term>>;

inline auto reduct_is_nonmonotone(LGuard const &lhs, AggregateFunction fun, RGuard const &rhs) -> bool {
    if (!lhs.has_value() && !rhs.has_value()) {
        return false;
    }
    if (lhs.has_value() && lhs->second == Relation::inequal) {
        return true;
    }
    if (rhs.has_value() && rhs->first == Relation::inequal) {
        return true;
    }
    return fun == AggregateFunction::sum;
}

struct SetAggregate {
    struct Element {
        Element(Literal lit, LiteralVec cond) : lit{std::move(lit)}, cond{std::move(cond)} {}
        Literal lit;
        LiteralVec cond;
    };
    using ElementVec = std::vector<Element>;

    SetAggregate(LGuard lhs, ElementVec elems, RGuard rhs)
        : elems{std::move(elems)}, lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    SetAggregate(ElementVec elems) : SetAggregate{std::nullopt, std::move(elems), std::nullopt} {}
    SetAggregate(ElementVec elems, Relation rel, Term rhs)
        : SetAggregate{std::nullopt, std::move(elems), std::make_pair(rel, std::move(rhs))} {}

    ElementVec elems;
    LGuard lhs;
    RGuard rhs;
};

} // namespace Gringo::Input
