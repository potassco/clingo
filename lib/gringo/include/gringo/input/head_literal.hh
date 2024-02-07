#pragma once

#include <gringo/input/aggregate.hh>
#include <gringo/input/literal.hh>
#include <gringo/input/theory.hh>

namespace Gringo::Input {

//! @defgroup input_head_literal Head Literals
//! Data structures and functions to represent head literals.
//!
//! @ingroup input_language
//!
//! @{

//! A single literal in a rule head.
class SimpleHeadLiteral {
  public:
    //! Wrap a literal in a head literal.
    SimpleHeadLiteral(Literal lit) : lit_{std::move(lit)} {}
    //! The literal.
    Literal lit_;
};

//! Compare two literals.
//!
//! @related SimpleHeadLiteral
auto operator==(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool;

//! Compare two literals.
//!
//! @related SimpleHeadLiteral
auto operator<(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool;

//! A disjunction of conditional literals.
class Disjunction {
  public:
    //! An element of a disjunction.
    using Element = std::variant<Literal, ConditionalLiteral>;
    //! A vector of elements.
    using ElementVec = Util::immutable_array<Element>;
    //! Wrap a literal in a head literal.
    explicit Disjunction(Location loc, ElementVec elems) : loc_{loc}, elems_{std::move(elems)} {}
    //! The location of the disjunction.
    Location loc_;
    //! The elements of the disjunction.
    ElementVec elems_;
};

//! Compare two head disjunctions.
//!
//! @related Disjunction
auto operator==(Disjunction const &a, Disjunction const &b) -> bool;

//! Compare two head disjunctions.
//!
//! @related Disjunction
auto operator<(Disjunction const &a, Disjunction const &b) -> bool;

//! A head aggregate.
//!
//! For example: <tt>\#count { X: p(X): q(X) } = 1</tt>
class HeadAggregate {
  public:
    //! An element of a head aggregate.
    class Element {
      public:
        explicit Element(Location loc, TermVec tuple, Literal lit, LiteralVec cond)
            : loc_{loc}, tuple_{std::move(tuple)}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
        //! The location of the element.
        Location loc_;
        //! The tuple of the element.
        TermVec tuple_;
        //! The distinguished head literal of the element.
        Literal lit_;
        //! The condition of the element.
        LiteralVec cond_;
    };
    //! A vector of head aggregate elements.
    using ElementVec = Util::immutable_array<Element>;

    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : loc_{std::move(loc)}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, AggregateFunction fun, ElementVec elems)
        : HeadAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::nullopt} {}
    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, AggregateFunction fun, ElementVec elems, Relation rel, Term rhs)
        : HeadAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::make_pair(rel, rhs)} {}

    //! The location of the aggregate.
    Location loc_;
    //! The aggregate function.
    AggregateFunction fun_;
    //! The vector of elements.
    ElementVec elems_;
    //! An optional left guard.
    LGuard lhs_;
    //! An optional right guard.
    RGuard rhs_;
};

//! Compare two head aggregates elements.
//!
//! @related HeadAggregate
auto operator==(HeadAggregate::Element const &a, HeadAggregate::Element const &b) -> bool;

//! Compare two head aggregates elements.
//!
//! @related HeadAggregate
auto operator<(HeadAggregate::Element const &a, HeadAggregate::Element const &b) -> bool;

//! Compare two head aggregates.
//!
//! @related HeadAggregate
auto operator==(HeadAggregate const &a, HeadAggregate const &b) -> bool;

//! Compare two head aggregates.
//!
//! @related HeadAggregate
auto operator<(HeadAggregate const &a, HeadAggregate const &b) -> bool;

//! A head literal.
using HeadLiteral = std::variant<SimpleHeadLiteral, Disjunction, HeadAggregate, HeadSetAggregate, HeadTheoryAtom>;
//! A vector of head literals.
using HeadLiteralVec = Util::immutable_array<HeadLiteral>;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::SimpleHeadLiteral);
GRINGO_HASH_PROTO(Gringo::Input::Disjunction);
GRINGO_HASH_PROTO(Gringo::Input::HeadAggregate::Element);
GRINGO_HASH_PROTO(Gringo::Input::HeadAggregate);

#endif
