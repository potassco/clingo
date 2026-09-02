#pragma once

#include <clingo/input/aggregate.hh>
#include <clingo/input/literal.hh>
#include <clingo/input/theory.hh>

#include <utility>

namespace CppClingo::Input {

//! @addtogroup input_body
//! @{

//! A single literal in a rule body.
class BdLitSimple : public Expression<BdLitSimple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_lit = &BdLitSimple::lit_}; }

    //! Wrap a literal in a body literal.
    BdLitSimple(Lit lit) : lit_{std::move(lit)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return location(lit_); }
    //! The literal.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }

  private:
    Lit lit_;
};

//! A conditional literal in a rule body.
class BdLitConjunction : public Expression<BdLitConjunction> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_lit = &BdLitConjunction::lit_}; }

    //! Construct a conjunction.
    BdLitConjunction(CondLit lit) : lit_{std::move(lit)} {}
    //! Get the location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return lit_.loc(); }
    //! The conditional literal representing the elements of the conjunction.
    [[nodiscard]] auto lit() const -> CondLit const & { return lit_; }

  private:
    CondLit lit_;
};

//! A body aggregate element.
class BdLitAggregateElement : public Expression<BdLitAggregateElement> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &BdLitAggregateElement::loc_, a_tuple = &BdLitAggregateElement::tuple_,
                          a_cond = &BdLitAggregateElement::cond_};
    }

    //! Construct a body aggregate element.
    explicit BdLitAggregateElement(Location loc, TermArray tuple, LitArray cond)
        : loc_{std::move(loc)}, tuple_{std::move(tuple)}, cond_{std::move(cond)} {}
    //! The location of the element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the element.
    [[nodiscard]] auto tuple() const -> TermArray const & { return tuple_; }
    //! The condition of the element.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

  private:
    Location loc_;
    TermArray tuple_;
    LitArray cond_;
};
//! A vector of aggregate elements.
using BdLitAggregateElementArray = Util::immutable_array<BdLitAggregateElement>;

//! A body aggregate.
//!
//! For example: <tt>\#count { X: q(X) } = 1</tt>
class BdLitAggregate : public Expression<BdLitAggregate> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &BdLitAggregate::loc_,     a_sign = &BdLitAggregate::sign_,
                          a_lhs = &BdLitAggregate::lhs_,     a_fun = &BdLitAggregate::fun_,
                          a_elems = &BdLitAggregate::elems_, a_rhs = &BdLitAggregate::rhs_};
    }

    //! Construct a body aggregate.
    explicit BdLitAggregate(Location loc, Sign sign, LGuard lhs, AggregateFunction fun,
                            BdLitAggregateElementArray elems, RGuard rhs)
        : loc_{std::move(loc)}, sign_{sign}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)},
          rhs_{std::move(rhs)} {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction { return fun_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> BdLitAggregateElementArray const & { return elems_; }
    //! An optional left guard.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! An optional right guard.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

  private:
    Location loc_;
    Sign sign_;
    AggregateFunction fun_;
    BdLitAggregateElementArray elems_;
    LGuard lhs_;
    RGuard rhs_;
};

//! A body sort literal.
//!
//! For example: <tt>(X,Y) = #sort { Z: q(Z) }</tt>
class BdLitSort : public Expression<BdLitSort> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &BdLitSort::loc_, a_sign = &BdLitSort::sign_, a_lhs = &BdLitSort::outputs_,
                          a_elems = &BdLitSort::elems_};
    }

    //! Construct a body sort literal.
    explicit BdLitSort(Location loc, Sign sign, Term outputs, BdLitAggregateElementArray elems)
        : loc_{std::move(loc)}, sign_{sign}, outputs_{std::move(outputs)}, elems_{std::move(elems)} {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The pair of output terms.
    [[nodiscard]] auto outputs() const -> Term const & { return outputs_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> BdLitAggregateElementArray const & { return elems_; }

  private:
    Location loc_;
    Sign sign_;
    Term outputs_;
    BdLitAggregateElementArray elems_;
};

//! A body literal.
using BdLit =
    std::variant<BdLitSimple, BdLitConjunction, BdLitAggregate, BdLitSort, BdLitSetAggregate, BdLitTheoryAtom>;
//! A vector of body literals.
using BdLitArray = Util::immutable_array<BdLit>;

//! @}

} // namespace CppClingo::Input
