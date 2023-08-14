#pragma once

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

//! @defgroup input_body_literal Body Literals
//! @ingroup input_language
//!
//! Data structures and functions to represent body literals.
//!
//! @{

//! A conjunction of oconditional literals.
using Conjunction = Junction<true>;

//! A conjunction//! A body aggregate.
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
    explicit BodyAggregate(Location loc, Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : loc{std::move(loc)}, sign{sign}, fun(fun), elems(std::move(elems)), lhs{std::move(lhs)}, rhs{std::move(rhs)} {
    }

    //! The location of the literal.
    Location loc;
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

    //! The sign of the literal.
    Sign sign;
    //! The corresponding set aggregate.
    SetAggregate aggr;
};

//! A body theory atom.
struct BodyTheoryAtom {
    //! Construct a body theory atom.
    explicit BodyTheoryAtom(Sign sign, TheoryAtom atom) : sign{sign}, atom{std::move(atom)} {}

    //! The sign of the literal.
    Sign sign;
    //! The corresponding theory atom aggregate.
    TheoryAtom atom;
};

//! A body literal.
using BodyLiteral = std::variant<Conjunction, BodyAggregate, BodySetAggregate, BodyTheoryAtom>;
//! A vector of body literals.
using BodyLiteralVec = std::vector<BodyLiteral>;

//! @}

} // namespace Gringo::Input
