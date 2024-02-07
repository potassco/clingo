#pragma once

#include <optional>

#include <gringo/input/literal.hh>

namespace Gringo::Input {

//! @defgroup input_aggregate Aggregates
//! Common data structures and functions for head and body aggregates.
//!
//! @ingroup input_language
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

//! An optional left guard of an aggregate.
using LGuard = std::optional<std::pair<Term, Relation>>;
//! An optional right guard of an aggregate.
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
class SetAggregateElement {
  public:
    explicit SetAggregateElement(Location loc, Literal lit, LiteralVec cond)
        : loc_{loc}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The literal.
    Literal lit_;
    //! The condition.
    LiteralVec cond_;
};

//! Compare two set aggregate elements.
//!
//! @related SetAggregateElement
auto operator==(SetAggregateElement const &a, SetAggregateElement const &b) -> bool;

//! Compare two set aggregate elements.
//!
//! @related SetAggregateElement
auto operator<(SetAggregateElement const &a, SetAggregateElement const &b) -> bool;

//! A vector of set aggregate elements.
using SetAggregateElementVec = Util::immutable_array<SetAggregateElement>;

//! A set aggregate.
//!
//! For example: <tt>{ p(X): q(X) } = 1</tt>.
template <bool HasSign> class SetAggregate : public std::conditional_t<HasSign, Signed, Unsigned> {
  public:
    //! Construct a set aggregate.
    explicit SetAggregate(Location loc, LGuard lhs, SetAggregateElementVec elems, RGuard rhs)
        : loc_{std::move(loc)}, elems_{std::move(elems)}, lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
        static_assert(!HasSign);
    }

    //! Construct a set aggregate.
    explicit SetAggregate(Location loc, Sign sign, LGuard lhs, SetAggregateElementVec elems, RGuard rhs)
        : Signed{sign}, loc_{std::move(loc)}, elems_{std::move(elems)}, lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
        static_assert(HasSign);
    }

    //! The location of the aggregate.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The elements of the set aggregate.
    SetAggregateElementVec elems_;
    //! The optional right guard of the aggregate.
    LGuard lhs_;
    //! The optional left guard of the aggregate.
    RGuard rhs_;
};

//! A head set aggregate.
using HeadSetAggregate = SetAggregate<false>;

//! A body set aggregate.
using BodySetAggregate = SetAggregate<true>;

//! Compare two set aggregates.
//!
//! @related SetAggregate
template <bool HasSign> auto operator==(SetAggregate<HasSign> const &a, SetAggregate<HasSign> const &b) -> bool;

//! Compare two set aggregates.
//!
//! @related SetAggregate
template <bool HasSign> auto operator<(SetAggregate<HasSign> const &a, SetAggregate<HasSign> const &b) -> bool;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::SetAggregateElement);
GRINGO_HASH_PROTO(Gringo::Input::HeadSetAggregate);
GRINGO_HASH_PROTO(Gringo::Input::BodySetAggregate);

#endif
