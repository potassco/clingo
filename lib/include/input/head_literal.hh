#pragma once

#include <tuple>

#include <input/aggregate.hh>
#include <input/literal.hh>
#include <input/theory.hh>

namespace Gringo::Input {

//! @defgroup input_head_literal Head Literals
//! @ingroup input_language
//!
//! Data structures and functions to represent head literals.
//!
//! @{

//! A disjunction.
//!
//! Can also reprent a single literal.
//!
//! For example: <tt>p(X); q(X,Y): r(Y)</tt>
struct Disjunction {
    //! Construct a disjunction.
    explicit Disjunction(Location loc, ConditionalLiteralVec elems) : loc{std::move(loc)}, elems{std::move(elems)} {}

    //! The location of the disjunction.
    Location loc;
    //! The vector of conditional literals.
    ConditionalLiteralVec elems;
};

//! A head aggregate.
//!
//! For example: <tt>#count { X: p(X): q(X) } = 1</tt>
struct HeadAggregate {
    //! An element of a head aggregate.
    struct Element {
        //! The tuple of the element.
        TermVec tuple;
        //! The distinguished head literal of the element.
        Literal lit;
        //! The condition of the element.
        LiteralVec cond;
    };
    //! A vector of head aggregate elements.
    using ElementVec = std::vector<Element>;

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
//!
//! For example: <tt>#count { p(X): q(X) } = 1</tt>
struct HeadSetAggregate {
    //! Construct a head set aggregate.
    explicit HeadSetAggregate(Location loc, SetAggregate aggr) : loc{std::move(loc)}, aggr{std::move(aggr)} {}

    //! The location of the aggregate.
    Location loc;
    //! The set aggregate.
    SetAggregate aggr;
};

//! A head theory atom.
struct HeadTheoryAtom {
    //! Construct a head theory atom.
    explicit HeadTheoryAtom(Location loc, TheoryAtom atom) : loc{std::move(loc)}, atom{std::move(atom)} {}

    //! The location of the theory atom.
    Location loc;
    //! The corresponding theory atom aggregate.
    TheoryAtom atom;
};

//! A head literal.
using HeadLiteral = std::variant<Disjunction, HeadAggregate, HeadSetAggregate, HeadTheoryAtom>;
//! A vector of head literals.
using HeadLiteralVec = std::vector<HeadLiteral>;

//! @}

} // namespace Gringo::Input
