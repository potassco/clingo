#pragma once

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

struct Disjunction {
    explicit Disjunction(ConditionalLiteralVec elems) : elems{std::move(elems)} {}

    ConditionalLiteralVec elems;
};

struct HeadTheoryAtom {
    explicit HeadTheoryAtom(TheoryAtom atom) : atom{std::move(atom)} {}

    TheoryAtom atom;
};

struct HeadAggregate {
    struct Element {
        TermVec tuple;
        Literal lit;
        LiteralVec cond;
    };
    using ElementVec = std::vector<Element>;

    explicit HeadAggregate(LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : fun(fun), elems(std::move(elems)), lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
    explicit HeadAggregate(AggregateFunction fun, ElementVec elems)
        : HeadAggregate{std::nullopt, fun, std::move(elems), std::nullopt} {}
    explicit HeadAggregate(AggregateFunction fun, ElementVec elems, Relation rel, Term rhs)
        : HeadAggregate{std::nullopt, fun, std::move(elems), std::make_pair(rel, rhs)} {}

    AggregateFunction fun;
    ElementVec elems;
    LGuard lhs;
    RGuard rhs;
};

struct HeadSetAggregate {
    explicit HeadSetAggregate(SetAggregate aggr) : aggr{std::move(aggr)} {}

    SetAggregate aggr;
};

using HeadLiteral = std::variant<Disjunction, HeadAggregate, HeadSetAggregate, HeadTheoryAtom>;
using HeadLiteralVec = std::vector<HeadLiteral>;

} // namespace Gringo::Input
