#pragma once

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

//! @defgroup body_literal Body Literals
//! @ingroup language
//!
//! Data structures and functions to represent body literals.
//!
//! @{

//! A conjunction.
//!
//! Can also represent a single literal.
//!
//! For example: <tt>p(X); q(X,Y): r(Y)</tt>
struct Conjunction {
    //! Construct a conjunction.
    explicit Conjunction(ConditionalLiteralVec elems) : elems{std::move(elems)} {}

    //! The vector of conditional literals.
    ConditionalLiteralVec elems;
};

//! A body aggregate.
//!
//! For example: <tt>#count { X: q(X) } = 1</tt>
struct BodyAggregate {
    //! An aggregate element.
    struct Element {
        //! The tuple of the element.
        TermVec tuple;
        //! The condition of the element.
        LiteralVec cond;
    };
    //! A vector of aggregate elements.
    using ElementVec = std::vector<Element>;

    //! Construct a body aggregate.
    explicit BodyAggregate(Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : sign{sign}, fun(fun), elems(std::move(elems)), lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
    //! Construct a body aggregate.
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems, Relation rel, Term rhs)
        : BodyAggregate{Sign::none, std::nullopt, fun, std::move(elems), std::make_pair(rel, std::move(rhs))} {}
    //! Construct a body aggregate.
    explicit BodyAggregate(AggregateFunction fun, ElementVec elems)
        : BodyAggregate{Sign::none, std::nullopt, fun, std::move(elems), std::nullopt} {}

    //! The sign of the literal.
    Sign sign;
    //! The aggregate function.
    AggregateFunction fun;
    //! The vector of elements.
    ElementVec elems;
    //! An optional left guard.
    LGuard lhs;
    //! An optional right guard.
    RGuard rhs;
};

//! A body set aggregate.
//!
//! For example: <tt>#count { p(X): q(X) } = 1</tt>
struct BodySetAggregate {
    //! Construct a body set aggregate.
    explicit BodySetAggregate(Sign sign, SetAggregate aggr) : sign{sign}, aggr{std::move(aggr)} {}
    //! Construct a body set aggregate.
    explicit BodySetAggregate(SetAggregate aggr) : BodySetAggregate{Sign::none, std::move(aggr)} {}

    //! The sign of the literal.
    Sign sign;
    //! The corresponding set aggregate.
    SetAggregate aggr;
};

//! A body theory atom.
struct BodyTheoryAtom {
    //! Construct a body theory atom.
    explicit BodyTheoryAtom(Sign sign, TheoryAtom atom) : sign{sign}, atom{std::move(atom)} {}
    //! Construct a body theory atom.
    explicit BodyTheoryAtom(TheoryAtom atom) : BodyTheoryAtom{Sign::none, std::move(atom)} {}

    //! The sign of the literal.
    Sign sign;
    //! The corresponding theory atom aggregate.
    TheoryAtom atom;
};

//! A body literal.
using BodyLiteral = std::variant<Conjunction, BodyAggregate, BodySetAggregate, BodyTheoryAtom>;
//! A vector of body literals.
using BodyLiteralVec = std::vector<BodyLiteral>;

//! Add a sign to the body literal.
//!
//! Note that this function has to be used with care because the library uses shared pointers to literals.
//! This function is currently only used during construction in the parser.
void add_sign(BodyLiteral &lit, Sign sign);

//! @}

} // namespace Gringo::Input
