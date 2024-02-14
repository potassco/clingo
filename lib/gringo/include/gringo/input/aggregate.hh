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
    //! The literal.
    [[nodiscard]] auto lit() const -> Literal const & { return lit_; }
    //! The condition.
    [[nodiscard]] auto cond() const -> LiteralVec const & { return cond_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_lit, a_cond}, Types{args...});
        return SetAggregateElement{select<Opt>(a_loc, loc_, args...), select<Opt>(a_lit, lit_, args...),
                                   select<Opt>(a_cond, cond_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(SetAggregateElement const &a, SetAggregateElement const &b) -> bool;
    friend auto operator<(SetAggregateElement const &a, SetAggregateElement const &b) -> bool;
    friend struct Util::value_hasher<SetAggregateElement>;

    Location loc_;
    Literal lit_;
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
    //! The elements of the set aggregate.
    [[nodiscard]] auto elems() const -> SetAggregateElementVec const & { return elems_; }
    //! The optional right guard of the aggregate.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! The optional left guard of the aggregate.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        if constexpr (HasSign) {
            check(Types{a_loc, a_sign, a_lhs, a_elems, a_rhs}, Types{args...});
            return SetAggregate{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, this->sign(), args...),
                                select<Opt>(a_lhs, lhs_, args...), select<Opt>(a_elems, elems_, args...),
                                select<Opt>(a_rhs, rhs_, args...)};
        } else {
            check(Types{a_loc, a_lhs, a_elems, a_rhs}, Types{args...});
            return SetAggregate{select<Opt>(a_loc, loc_, args...), select<Opt>(a_lhs, lhs_, args...),
                                select<Opt>(a_elems, elems_, args...), select<Opt>(a_rhs, rhs_, args...)};
        }
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(SetAggregate<HasSign> const &a, SetAggregate<HasSign> const &b) -> bool;
    friend auto operator<(SetAggregate<HasSign> const &a, SetAggregate<HasSign> const &b) -> bool;
    friend struct Util::value_hasher<SetAggregate<HasSign>>;

    Location loc_;
    SetAggregateElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

//! A head set aggregate.
using HeadSetAggregate = SetAggregate<false>;

//! A body set aggregate.
using BodySetAggregate = SetAggregate<true>;

//! Compare two set aggregates.
//!
//! @related SetAggregate
auto operator==(HeadSetAggregate const &a, HeadSetAggregate const &b) -> bool;

//! Compare two set aggregates.
//!
//! @related SetAggregate
auto operator<(HeadSetAggregate const &a, HeadSetAggregate const &b) -> bool;

//! Compare two set aggregates.
//!
//! @related SetAggregate
auto operator==(BodySetAggregate const &a, BodySetAggregate const &b) -> bool;

//! Compare two set aggregates.
//!
//! @related SetAggregate
auto operator<(BodySetAggregate const &a, BodySetAggregate const &b) -> bool;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::SetAggregateElement);
GRINGO_HASH_PROTO(Gringo::Input::HeadSetAggregate);
GRINGO_HASH_PROTO(Gringo::Input::BodySetAggregate);

#endif
