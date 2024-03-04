#pragma once

#include <gringo/input/literal.hh>

#include <optional>

namespace Gringo::Input {

//! @ingroup input_aggregate
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
class SetAggregateElement : public Expression<SetAggregateElement> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &SetAggregateElement::loc_, a_lit = &SetAggregateElement::lit_,
                          a_cond = &SetAggregateElement::cond_};
    }

    //! Construct a set aggregate element.
    explicit SetAggregateElement(Location loc, Lit lit, LitArray cond)
        : loc_{loc}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The literal.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }
    //! The condition.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

  private:
    Location loc_;
    Lit lit_;
    LitArray cond_;
};

//! A vector of set aggregate elements.
using SetAggregateElementArray = Util::immutable_array<SetAggregateElement>;

//! A set aggregate.
//!
//! For example: <tt>{ p(X): q(X) } = 1</tt>.
template <bool HasSign>
class SetAggregate : public std::conditional_t<HasSign, Signed, Unsigned>, public Expression<SetAggregate<HasSign>> {
  public:
    //! The base class.
    using Base = std::conditional_t<HasSign, Signed, Unsigned>;

    //! The record attributes.
    static constexpr auto attributes() {
        if constexpr (HasSign) {
            return std::tuple{a_loc = &SetAggregate::loc_, a_sign = &Signed::sign, a_lhs = &SetAggregate::lhs_,
                              a_elems = &SetAggregate::elems_, a_rhs = &SetAggregate::rhs_};
        } else {
            return std::tuple{a_loc = &SetAggregate::loc_, a_lhs = &SetAggregate::lhs_, a_elems = &SetAggregate::elems_,
                              a_rhs = &SetAggregate::rhs_};
        }
    }

    //! Construct a set aggregate.
    explicit SetAggregate(Location loc, LGuard lhs, SetAggregateElementArray elems, RGuard rhs)
        : loc_{std::move(loc)}, elems_{std::move(elems)}, lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
        static_assert(!HasSign);
    }

    //! Construct a set aggregate.
    explicit SetAggregate(Location loc, Sign sign, LGuard lhs, SetAggregateElementArray elems, RGuard rhs)
        : Signed{sign}, loc_{std::move(loc)}, elems_{std::move(elems)}, lhs_(std::move(lhs)), rhs_(std::move(rhs)) {
        static_assert(HasSign);
    }

    //! The location of the aggregate.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The elements of the set aggregate.
    [[nodiscard]] auto elems() const -> SetAggregateElementArray const & { return elems_; }
    //! The optional right guard of the aggregate.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! The optional left guard of the aggregate.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

  private:
    Location loc_;
    SetAggregateElementArray elems_;
    LGuard lhs_;
    RGuard rhs_;
};

//! A head set aggregate.
using HdLitSetAggregate = SetAggregate<false>;

//! A body set aggregate.
using BdLitSetAggregate = SetAggregate<true>;

//! @}

} // namespace Gringo::Input
