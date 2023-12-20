#pragma once

#include <optional>

#include <input/literal.hh>

namespace Gringo::Input {

//! @defgroup input_aggregate Aggregates
//! @ingroup input_language
//!
//! Common data structures and functions for head and body aggregates.
//!
//! @{

//! Enumeration of aggregate functions.
enum class AggregateFunction {
    count, //! The <tt>\#count</tt> function.
    sum,   //! The <tt>\#sum</tt> function.
    sump,  //! The <tt>\#sum+</tt> function.
    min,   //! The <tt>\#min</tt> function.
    max,   //! The <tt>\#max</tt> function.
};

//! An optical left guard of an aggregate.
using LGuard = std::optional<std::pair<Term, Relation>>;
//! An optical right guard of an aggregate.
using RGuard = std::optional<std::pair<Relation, Term>>;

//! Check whether the given aggregate function/guard combination can result in a nonmonotone aggregate.
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

//! An element of a set aggregate.
struct SetAggregateElement {
    //! The location of the element.
    Location loc;
    //! The literal.
    Literal lit;
    //! The condition.
    LiteralVec cond;
};

//! A vector of set aggregate elements.
using SetAggregateElementVec = std::vector<SetAggregateElement>;

//! A set aggregate.
//!
//! For example: <tt>{ p(X): q(X) } = 1</tt>.
template <bool HasSign> struct SetAggregate : std::conditional_t<HasSign, Signed, Unsigned> {

    //! Construct a set aggregate.
    explicit SetAggregate(Location loc, LGuard lhs, SetAggregateElementVec elems, RGuard rhs)
        : loc{std::move(loc)}, elems{std::move(elems)}, lhs(std::move(lhs)), rhs(std::move(rhs)) {
        static_assert(!HasSign);
    }

    //! Construct a set aggregate.
    explicit SetAggregate(Location loc, Sign sign, LGuard lhs, SetAggregateElementVec elems, RGuard rhs)
        : Signed{sign}, loc{std::move(loc)}, elems{std::move(elems)}, lhs(std::move(lhs)), rhs(std::move(rhs)) {
        static_assert(HasSign);
    }

    //! The location of the aggregate.
    Location loc;
    //! The elements of the set aggregate.
    SetAggregateElementVec elems;
    //! The optical right guard of the aggregate.
    LGuard lhs;
    //! The optical left guard of the aggregate.
    RGuard rhs;
};

//! @}

} // namespace Gringo::Input
