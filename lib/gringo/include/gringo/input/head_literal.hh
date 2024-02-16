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
class HdLitSimple {
  public:
    //! Wrap a literal in a head literal.
    HdLitSimple(Lit lit) : lit_{std::move(lit)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return location(lit_); }
    //! The literal.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_lit}, Types{args...});
        return HdLitSimple{select<Opt>(a_lit, lit_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two literals.
    friend auto operator==(HdLitSimple const &a, HdLitSimple const &b) -> bool = default;
    //! Compare two literals.
    friend auto operator<=>(HdLitSimple const &a, HdLitSimple const &b) = default;

  private:
    friend struct Util::value_hasher<HdLitSimple>;

    Lit lit_;
};

//! An element of a disjunction.
using HdLitDisjunctionElement = std::variant<Lit, CondLit>;
//! A vector of elements.
using HdLitDisjunctionElementArray = Util::immutable_array<HdLitDisjunctionElement>;

//! A disjunction of conditional literals.
class HdLitDisjunction {
  public:
    //! Wrap a literal in a head literal.
    explicit HdLitDisjunction(Location loc, HdLitDisjunctionElementArray elems) : loc_{loc}, elems_{std::move(elems)} {}
    //! The location of the disjunction.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The elements of the disjunction.
    [[nodiscard]] auto elems() const -> HdLitDisjunctionElementArray const & { return elems_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_elems}, Types{args...});
        return HdLitDisjunction{select<Opt>(a_loc, loc_, args...), select<Opt>(a_elems, elems_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two disjunctions.
    friend auto operator==(HdLitDisjunction const &a, HdLitDisjunction const &b) -> bool {
        return a.elems_ == b.elems_;
    }
    //! Compare two disjunctions.
    friend auto operator<=>(HdLitDisjunction const &a, HdLitDisjunction const &b) -> std::strong_ordering {
        return a.elems_ <=> b.elems_;
    }

  private:
    friend struct Util::value_hasher<HdLitDisjunction>;

    Location loc_;
    HdLitDisjunctionElementArray elems_;
};

//! An element of a head aggregate.
class HdLitAggregateElement {
  public:
    explicit HdLitAggregateElement(Location loc, TermArray tuple, Lit lit, LitArray cond)
        : loc_{loc}, tuple_{std::move(tuple)}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the element.
    [[nodiscard]] auto tuple() const -> TermArray const & { return tuple_; }
    //! The distinguished head literal of the element.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }
    //! The condition of the element.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_tuple, a_lit, a_cond}, Types{args...});
        return HdLitAggregateElement{select<Opt>(a_loc, loc_, args...), select<Opt>(a_tuple, tuple_, args...),
                                     select<Opt>(a_lit, lit_, args...), select<Opt>(a_cond, cond_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two head aggregate elements.
    friend auto operator==(HdLitAggregateElement const &a, HdLitAggregateElement const &b) -> bool {
        return std::tie(a.tuple_, a.lit_, a.cond_) == std::tie(b.tuple_, b.lit_, b.cond_);
    }
    //! Compare two head aggregate elements.
    friend auto operator<=>(HdLitAggregateElement const &a, HdLitAggregateElement const &b) -> std::strong_ordering {
        return std::tie(a.tuple_, a.lit_, a.cond_) <=> std::tie(b.tuple_, b.lit_, b.cond_);
    }

  private:
    friend struct Util::value_hasher<HdLitAggregateElement>;

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
class HdLitAggregate {
  public:
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
        : HdLitAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::make_pair(rel, rhs)} {}

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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_fun, a_lhs, a_elems, a_rhs}, Types{args...});
        return HdLitAggregate{select<Opt>(a_loc, loc_, args...), select<Opt>(a_lhs, lhs_, args...),
                              select<Opt>(a_fun, fun_, args...), select<Opt>(a_elems, elems_, args...),
                              select<Opt>(a_rhs, rhs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two head aggregates.
    friend auto operator==(HdLitAggregate const &a, HdLitAggregate const &b) -> bool {
        return std::tie(a.fun_, a.lhs_, a.elems_, a.rhs_) == std::tie(b.fun_, b.lhs_, b.elems_, b.rhs_);
    }
    //! Compare two head aggregates.
    friend auto operator<=>(HdLitAggregate const &a, HdLitAggregate const &b) -> std::strong_ordering {
        // Note: std::optional does not produce a strong_ordering - bug???
        return Util::make_strong_ordering(std::tie(a.fun_, a.lhs_, a.elems_, a.rhs_) <=>
                                          std::tie(b.fun_, b.lhs_, b.elems_, b.rhs_));
    }

  private:
    friend struct Util::value_hasher<HdLitAggregate>;

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

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::HdLitSimple);
GRINGO_HASH_PROTO(Gringo::Input::HdLitDisjunction);
GRINGO_HASH_PROTO(Gringo::Input::HdLitAggregateElement);
GRINGO_HASH_PROTO(Gringo::Input::HdLitAggregate);

#endif
