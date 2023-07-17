#pragma once

//! @file
//! This file contains the body literal interface and derived body literals.

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

struct Conjunction {
    explicit Conjunction(ConditionalLiteralVec elems) : elems{std::move(elems)} {}

    ConditionalLiteralVec elems;
};

struct BodyAggregate {
    struct Element {
        Element(TermVec tuple, LiteralVec cond) : tuple{std::move(tuple)}, cond{std::move(cond)} {}
        TermVec tuple;
        LiteralVec cond;
    };
    using ElementVec = std::vector<Element>;

    explicit BodyAggregate(Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : sign{sign}, fun(fun), elems(std::move(elems)), lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, Term rhs)
        : BodyAggregate{Sign::none, std::nullopt, fun, std::move(elems), std::make_pair(rel, std::move(rhs))} {}
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems)
        : BodyAggregate{Sign::none, std::nullopt, fun, std::move(elems), std::nullopt} {}

    Sign sign;
    AggregateFunction fun;
    ElementVec elems;
    LGuard lhs;
    RGuard rhs;
};

struct BodySetAggregate {
    explicit BodySetAggregate(Sign sign, SetAggregate aggr) : sign{sign}, aggr{std::move(aggr)} {}
    explicit BodySetAggregate(SetAggregate aggr) : BodySetAggregate{Sign::none, std::move(aggr)} {}

    Sign sign;
    SetAggregate aggr;
};

struct BodyTheoryAtom {
    explicit BodyTheoryAtom(Sign sign, TheoryAtom atom) : sign{sign}, atom{std::move(atom)} {}
    explicit BodyTheoryAtom(TheoryAtom atom) : BodyTheoryAtom{Sign::none, std::move(atom)} {}

    Sign sign;
    TheoryAtom atom;
};

using BodyLiteral = std::variant<Conjunction, BodyAggregate, BodySetAggregate, BodyTheoryAtom>;
using BodyLiteralVec = std::vector<BodyLiteral>;

//! Add a sign to the body literal.
//!
//! Note that this function has to be used with care because the library uses shared pointers to literals.
//! This function is currently only used during construction in the parser.
void add_sign(BodyLiteral &lit, Sign sign);

} // namespace Gringo::Input
