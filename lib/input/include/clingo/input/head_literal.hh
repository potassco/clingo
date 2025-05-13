#pragma once

#include <clingo/input/aggregate.hh>
#include <clingo/input/literal.hh>
#include <clingo/input/theory.hh>

#include <utility>

namespace CppClingo::Input {

//! @addtogroup input_head
//! @{

//! A single literal in a rule head.
class HdLitSimple : public Expression<HdLitSimple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_lit = &HdLitSimple::lit_}; }

    //! Wrap a literal in a head literal.
    HdLitSimple(Lit lit) : lit_{std::move(lit)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return location(lit_); }
    //! The literal.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }

  private:
    Lit lit_;
};

//! An element of a disjunction.
using HdLitDisjunctionElement = std::variant<Lit, CondLit>;
//! A vector of elements.
using HdLitDisjunctionElementArray = Util::immutable_array<HdLitDisjunctionElement>;

//! A disjunction of conditional literals.
class HdLitDisjunction : public Expression<HdLitDisjunction> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &HdLitDisjunction::loc_, a_elems = &HdLitDisjunction::elems_};
    }

    //! Wrap a literal in a head literal.
    explicit HdLitDisjunction(Location loc, HdLitDisjunctionElementArray elems)
        : loc_{std::move(loc)}, elems_{std::move(elems)} {}
    //! The location of the disjunction.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The elements of the disjunction.
    [[nodiscard]] auto elems() const -> HdLitDisjunctionElementArray const & { return elems_; }

  private:
    Location loc_;
    HdLitDisjunctionElementArray elems_;
};

//! An element of a head aggregate.
class HdLitAggregateElement : public Expression<HdLitAggregateElement> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &HdLitAggregateElement::loc_, a_tuple = &HdLitAggregateElement::tuple_,
                          a_lit = &HdLitAggregateElement::lit_, a_cond = &HdLitAggregateElement::cond_};
    }

    //! Construct a head aggregate element.
    explicit HdLitAggregateElement(Location loc, TermArray tuple, Lit lit, LitArray cond)
        : loc_{std::move(loc)}, tuple_{std::move(tuple)}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the element.
    [[nodiscard]] auto tuple() const -> TermArray const & { return tuple_; }
    //! The distinguished head literal of the element.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }
    //! The condition of the element.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

  private:
    Location loc_;
    TermArray tuple_;
    Lit lit_;
    LitArray cond_;
};
//! A vector of head aggregate elements.
using HdLitAggregateElementArray = Util::immutable_array<HdLitAggregateElement>;

//! A head aggregate.
//!
//! For example: <tt>\#count { X: p(X): q(X) } = 1</tt>
class HdLitAggregate : public Expression<HdLitAggregate> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &HdLitAggregate::loc_, a_lhs = &HdLitAggregate::lhs_, a_fun = &HdLitAggregate::fun_,
                          a_elems = &HdLitAggregate::elems_, a_rhs = &HdLitAggregate::rhs_};
    }

    //! Construct a head set aggregate.
    explicit HdLitAggregate(Location loc, LGuard lhs, AggregateFunction fun, HdLitAggregateElementArray elems,
                            RGuard rhs)
        : loc_{std::move(loc)}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    //! Construct a head set aggregate.
    explicit HdLitAggregate(Location loc, AggregateFunction fun, HdLitAggregateElementArray elems)
        : HdLitAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::nullopt} {}
    //! Construct a head set aggregate.
    explicit HdLitAggregate(Location loc, AggregateFunction fun, HdLitAggregateElementArray elems, Relation rel,
                            Term rhs)
        : HdLitAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::make_pair(rel, std::move(rhs))} {}

    //! The location of the aggregate.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction { return fun_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> HdLitAggregateElementArray const & { return elems_; }
    //! An optional left guard.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! An optional right guard.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

  private:
    Location loc_;
    AggregateFunction fun_;
    HdLitAggregateElementArray elems_;
    LGuard lhs_;
    RGuard rhs_;
};

//! A head literal.
using HdLit = std::variant<HdLitSimple, HdLitDisjunction, HdLitAggregate, HdLitSetAggregate, HdLitTheoryAtom>;
//! A vector of head literals.
using HdLitArray = Util::immutable_array<HdLit>;

//! @}

} // namespace CppClingo::Input
