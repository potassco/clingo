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
struct SimpleHeadLiteral {
    //! Wrap a literal in a head literal.
    SimpleHeadLiteral(Literal lit) : lit{std::move(lit)} {}
    //! The literal.
    Literal lit;
};

//! A disjunction of conditional literals.
struct Disjunction {
    //! An element of a disjunction.
    using Element = std::variant<Literal, ConditionalLiteral>;
    //! A vector of elements.
    using ElementVec = Util::immutable_vector<Element>;
    //! Wrap a literal in a head literal.
    Disjunction(Location loc, ElementVec elems) : loc{loc}, elems{std::move(elems)} {}
    //! The location of the disjunction.
    Location loc;
    //! The location of the disjunction.
    ElementVec elems;
};

//! A head aggregate.
//!
//! For example: <tt>\#count { X: p(X): q(X) } = 1</tt>
struct HeadAggregate {
    //! An element of a head aggregate.
    struct Element {
        //! The location of the element.
        Location loc;
        //! The tuple of the element.
        TermVec tuple;
        //! The distinguished head literal of the element.
        Literal lit;
        //! The condition of the element.
        LiteralVec cond;
    };
    //! A vector of head aggregate elements.
    using ElementVec = Util::immutable_vector<Element>;

    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : loc{std::move(loc)}, fun(fun), elems(std::move(elems)), lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, AggregateFunction fun, ElementVec elems)
        : HeadAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::nullopt} {}
    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, AggregateFunction fun, ElementVec elems, Relation rel, Term rhs)
        : HeadAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::make_pair(rel, rhs)} {}

    //! The location of the aggregate.
    Location loc;
    //! The aggregate function.
    AggregateFunction fun;
    //! The vector of elements.
    ElementVec elems;
    //! An optional left guard.
    LGuard lhs;
    //! An optional right guard.
    RGuard rhs;
};

//! A head set aggregate.
using HeadSetAggregate = SetAggregate<false>;

//! A head theory atom.
using HeadTheoryAtom = TheoryAtom<false>;

//! A head literal.
using HeadLiteral = std::variant<SimpleHeadLiteral, Disjunction, HeadAggregate, HeadSetAggregate, HeadTheoryAtom>;
//! A vector of head literals.
using HeadLiteralVec = Util::immutable_vector<HeadLiteral>;

//! @}

} // namespace Gringo::Input
