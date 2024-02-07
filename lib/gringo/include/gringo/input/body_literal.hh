#pragma once

#include <gringo/input/aggregate.hh>
#include <gringo/input/literal.hh>
#include <gringo/input/theory.hh>

namespace Gringo::Input {

//! @defgroup input_body_literal Body Literals
//! Data structures and functions to represent body literals.
//!
//! @ingroup input_language
//!
//! @{

//! A single literal in a rule body.
struct SimpleBodyLiteral {
    //! Wrap a literal in a body literal.
    SimpleBodyLiteral(Literal lit) : lit_{std::move(lit)} {}
    //! The literal.
    Literal lit_;
};

//! Compare two literals.
//!
//! @related SimpleBodyLiteral
auto operator==(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool;

//! Compare two literals.
//!
//! @related SimpleBodyLiteral
auto operator<(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool;

//! A conditional literal in a rule body.
struct Conjunction {
    //! Construct a conjunction.
    Conjunction(ConditionalLiteral lit) : lit_{std::move(lit)} {}
    //! The conditional literal representing the elements of the conjunction.
    ConditionalLiteral lit_;
};

//! Compare two body conjunctions.
//!
//! @related Conjunction
auto operator==(Conjunction const &a, Conjunction const &b) -> bool;

//! Compare two body conjunction.
//!
//! @related Conjunction
auto operator<(Conjunction const &a, Conjunction const &b) -> bool;

//! A body aggregate.
//!
//! For example: <tt>\#count { X: q(X) } = 1</tt>
struct BodyAggregate {
    //! An aggregate element.
    struct Element {
        //! The location of the element.
        Location loc_;
        //! The tuple of the element.
        TermVec tuple_;
        //! The condition of the element.
        LiteralVec cond_;
    };
    //! A vector of aggregate elements.
    using ElementVec = Util::immutable_array<Element>;

    //! Construct a body aggregate.
    explicit BodyAggregate(Location loc, Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : loc_{std::move(loc)}, sign_{sign}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)},
          rhs_{std::move(rhs)} {}

    //! The location of the literal.
    Location loc_;
    //! The sign of the literal.
    Sign sign_;
    //! The aggregate function.
    AggregateFunction fun_;
    //! The vector of elements.
    ElementVec elems_;
    //! An optional left guard.
    LGuard lhs_;
    //! An optional right guard.
    RGuard rhs_;
};

//! Compare two body aggregates elements.
//!
//! @related BodyAggregate
auto operator==(BodyAggregate::Element const &a, BodyAggregate::Element const &b) -> bool;

//! Compare two body aggregates elements.
//!
//! @related BodyAggregate
auto operator<(BodyAggregate::Element const &a, BodyAggregate::Element const &b) -> bool;

//! Compare two body aggregates.
//!
//! @related BodyAggregate
auto operator==(BodyAggregate const &a, BodyAggregate const &b) -> bool;

//! Compare two body aggregates.
//!
//! @related BodyAggregate
auto operator<(BodyAggregate const &a, BodyAggregate const &b) -> bool;

//! A body set aggregate.
using BodySetAggregate = SetAggregate<true>;

//! A body theory atom.
using BodyTheoryAtom = TheoryAtom<true>;

//! A body literal.
using BodyLiteral = std::variant<SimpleBodyLiteral, Conjunction, BodyAggregate, BodySetAggregate, BodyTheoryAtom>;
//! A vector of body literals.
using BodyLiteralVec = Util::immutable_array<BodyLiteral>;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::SimpleBodyLiteral);
GRINGO_HASH_PROTO(Gringo::Input::Conjunction);
GRINGO_HASH_PROTO(Gringo::Input::BodyAggregate::Element);
GRINGO_HASH_PROTO(Gringo::Input::BodyAggregate);

#endif
