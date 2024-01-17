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
    SimpleBodyLiteral(Literal lit) : lit{std::move(lit)} {}
    //! The literal.
    Literal lit;
};

//! A conditional literal in a rule body.
struct Conjunction {
    //! Construct a conjunction.
    Conjunction(ConditionalLiteral lit) : lit{std::move(lit)} {}
    //! The conditional literal representing the elements of the conjunction.
    ConditionalLiteral lit;
};

//! A body aggregate.
//!
//! For example: <tt>\#count { X: q(X) } = 1</tt>
struct BodyAggregate {
    //! An aggregate element.
    struct Element {
        //! The location of the element.
        Location loc;
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
using BodySetAggregate = SetAggregate<true>;

//! A body theory atom.
using BodyTheoryAtom = TheoryAtom<true>;

//! A body literal.
using BodyLiteral = std::variant<SimpleBodyLiteral, Conjunction, BodyAggregate, BodySetAggregate, BodyTheoryAtom>;
//! A vector of body literals.
using BodyLiteralVec = Util::immutable_vector<BodyLiteral>;
//! A vector of body literal vectors.
using BodyLiteralVecVec = std::vector<BodyLiteralVec>;

//! @}

} // namespace Gringo::Input
