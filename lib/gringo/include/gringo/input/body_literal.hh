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
class BdLitSimple {
  public:
    //! Wrap a literal in a body literal.
    BdLitSimple(Lit lit) : lit_{std::move(lit)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return location(lit_); }
    //! The literal.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_lit}, Types{args...});
        return BdLitSimple{select<Opt>(a_lit, lit_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(BdLitSimple const &a, BdLitSimple const &b) -> bool;
    friend auto operator<(BdLitSimple const &a, BdLitSimple const &b) -> bool;
    friend struct Util::value_hasher<BdLitSimple>;

    Lit lit_;
};

//! Compare two literals.
//!
//! @related SimpleBodyLiteral
auto operator==(BdLitSimple const &a, BdLitSimple const &b) -> bool;

//! Compare two literals.
//!
//! @related SimpleBodyLiteral
auto operator<(BdLitSimple const &a, BdLitSimple const &b) -> bool;

//! A conditional literal in a rule body.
class BdLitConjunction {
  public:
    //! Construct a conjunction.
    BdLitConjunction(CondLit lit) : lit_{std::move(lit)} {}
    //! Get the location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return lit_.loc(); }
    //! The conditional literal representing the elements of the conjunction.
    [[nodiscard]] auto lit() const -> CondLit const & { return lit_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_lit}, Types{args...});
        return BdLitConjunction{select<Opt>(a_lit, lit_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(BdLitConjunction const &a, BdLitConjunction const &b) -> bool;
    friend auto operator<(BdLitConjunction const &a, BdLitConjunction const &b) -> bool;
    friend struct Util::value_hasher<BdLitConjunction>;

    CondLit lit_;
};

//! Compare two body conjunctions.
//!
//! @related Conjunction
auto operator==(BdLitConjunction const &a, BdLitConjunction const &b) -> bool;

//! Compare two body conjunction.
//!
//! @related Conjunction
auto operator<(BdLitConjunction const &a, BdLitConjunction const &b) -> bool;

//! A body aggregate element.
class BdLitAggregateElement {
  public:
    explicit BdLitAggregateElement(Location loc, TermArray tuple, LitArray cond)
        : loc_{loc}, tuple_{std::move(tuple)}, cond_{std::move(cond)} {}
    //! The location of the element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the element.
    [[nodiscard]] auto tuple() const -> TermArray const & { return tuple_; }
    //! The condition of the element.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_tuple, a_cond}, Types{args...});
        return BdLitAggregateElement{select<Opt>(a_loc, loc_, args...), select<Opt>(a_tuple, tuple_, args...),
                                     select<Opt>(a_cond, cond_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(BdLitAggregateElement const &a, BdLitAggregateElement const &b) -> bool;
    friend auto operator<(BdLitAggregateElement const &a, BdLitAggregateElement const &b) -> bool;
    friend struct Util::value_hasher<BdLitAggregateElement>;

    Location loc_;
    TermArray tuple_;
    LitArray cond_;
};
//! A vector of aggregate elements.
using BdLitAggregateElementArray = Util::immutable_array<BdLitAggregateElement>;

//! A body aggregate.
//!
//! For example: <tt>\#count { X: q(X) } = 1</tt>
class BdLitAggregate {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_sign, a_fun, a_lhs, a_elems, a_rhs}, Types{args...});
        return BdLitAggregate{select<Opt>(a_loc, loc_, args...),     select<Opt>(a_sign, sign_, args...),
                              select<Opt>(a_lhs, lhs_, args...),     select<Opt>(a_fun, fun_, args...),
                              select<Opt>(a_elems, elems_, args...), select<Opt>(a_rhs, rhs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(BdLitAggregate const &a, BdLitAggregate const &b) -> bool;
    friend auto operator<(BdLitAggregate const &a, BdLitAggregate const &b) -> bool;
    friend struct Util::value_hasher<BdLitAggregate>;

    Location loc_;
    Sign sign_;
    AggregateFunction fun_;
    BdLitAggregateElementArray elems_;
    LGuard lhs_;
    RGuard rhs_;
};

//! Compare two body aggregates elements.
//!
//! @related BodyAggregate
auto operator==(BdLitAggregateElement const &a, BdLitAggregateElement const &b) -> bool;

//! Compare two body aggregates elements.
//!
//! @related BodyAggregate
auto operator<(BdLitAggregateElement const &a, BdLitAggregateElement const &b) -> bool;

//! Compare two body aggregates.
//!
//! @related BodyAggregate
auto operator==(BdLitAggregate const &a, BdLitAggregate const &b) -> bool;

//! Compare two body aggregates.
//!
//! @related BodyAggregate
auto operator<(BdLitAggregate const &a, BdLitAggregate const &b) -> bool;

//! A body literal.
using BdLit = std::variant<BdLitSimple, BdLitConjunction, BdLitAggregate, BdLitSetAggregate, BdLitTheoryAtom>;
//! A vector of body literals.
using BdLitArray = Util::immutable_array<BdLit>;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::BdLitSimple);
GRINGO_HASH_PROTO(Gringo::Input::BdLitConjunction);
GRINGO_HASH_PROTO(Gringo::Input::BdLitAggregateElement);
GRINGO_HASH_PROTO(Gringo::Input::BdLitAggregate);

#endif
