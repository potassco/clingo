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
class SimpleHeadLiteral {
  public:
    //! Wrap a literal in a head literal.
    SimpleHeadLiteral(Literal lit) : lit_{std::move(lit)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return location(lit_); }
    //! The literal.
    [[nodiscard]] auto lit() const -> Literal const & { return lit_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_lit}, Types{args...});
        return SimpleHeadLiteral{select<Opt>(a_lit, lit_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool;
    friend auto operator<(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool;
    friend struct Util::value_hasher<SimpleHeadLiteral>;

    Literal lit_;
};

//! Compare two literals.
//!
//! @related SimpleHeadLiteral
auto operator==(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool;

//! Compare two literals.
//!
//! @related SimpleHeadLiteral
auto operator<(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool;

//! An element of a disjunction.
using DisjunctionElement = std::variant<Literal, ConditionalLiteral>;
//! A vector of elements.
using DisjunctionElementVec = Util::immutable_array<DisjunctionElement>;

//! A disjunction of conditional literals.
class Disjunction {
  public:
    //! Wrap a literal in a head literal.
    explicit Disjunction(Location loc, DisjunctionElementVec elems) : loc_{loc}, elems_{std::move(elems)} {}
    //! The location of the disjunction.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The elements of the disjunction.
    [[nodiscard]] auto elems() const -> DisjunctionElementVec const & { return elems_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_elems}, Types{args...});
        return Disjunction{select<Opt>(a_loc, loc_, args...), select<Opt>(a_elems, elems_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(Disjunction const &a, Disjunction const &b) -> bool;
    friend auto operator<(Disjunction const &a, Disjunction const &b) -> bool;
    friend struct Util::value_hasher<Disjunction>;

    Location loc_;
    DisjunctionElementVec elems_;
};

//! Compare two head disjunctions.
//!
//! @related Disjunction
auto operator==(Disjunction const &a, Disjunction const &b) -> bool;

//! Compare two head disjunctions.
//!
//! @related Disjunction
auto operator<(Disjunction const &a, Disjunction const &b) -> bool;

//! An element of a head aggregate.
class HeadAggregateElement {
  public:
    explicit HeadAggregateElement(Location loc, TermVec tuple, Literal lit, LiteralVec cond)
        : loc_{loc}, tuple_{std::move(tuple)}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the element.
    [[nodiscard]] auto tuple() const -> TermVec const & { return tuple_; }
    //! The distinguished head literal of the element.
    [[nodiscard]] auto lit() const -> Literal const & { return lit_; }
    //! The condition of the element.
    [[nodiscard]] auto cond() const -> LiteralVec const & { return cond_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_tuple, a_lit, a_cond}, Types{args...});
        return HeadAggregateElement{select<Opt>(a_loc, loc_, args...), select<Opt>(a_tuple, tuple_, args...),
                                    select<Opt>(a_lit, lit_, args...), select<Opt>(a_cond, cond_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool;
    friend auto operator<(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool;
    friend struct Util::value_hasher<HeadAggregateElement>;

    Location loc_;
    TermVec tuple_;
    Literal lit_;
    LiteralVec cond_;
};
//! A vector of head aggregate elements.
using HeadAggregateElementVec = Util::immutable_array<HeadAggregateElement>;

//! A head aggregate.
//!
//! For example: <tt>\#count { X: p(X): q(X) } = 1</tt>
class HeadAggregate {
  public:
    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, LGuard lhs, AggregateFunction fun, HeadAggregateElementVec elems, RGuard rhs)
        : loc_{std::move(loc)}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}
    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, AggregateFunction fun, HeadAggregateElementVec elems)
        : HeadAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::nullopt} {}
    //! Construct a head set aggregate.
    explicit HeadAggregate(Location loc, AggregateFunction fun, HeadAggregateElementVec elems, Relation rel, Term rhs)
        : HeadAggregate{std::move(loc), std::nullopt, fun, std::move(elems), std::make_pair(rel, rhs)} {}

    //! The location of the aggregate.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction { return fun_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> HeadAggregateElementVec const & { return elems_; }
    //! An optional left guard.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! An optional right guard.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_fun, a_lhs, a_elems, a_rhs}, Types{args...});
        return HeadAggregate{select<Opt>(a_loc, loc_, args...), select<Opt>(a_lhs, lhs_, args...),
                             select<Opt>(a_fun, fun_, args...), select<Opt>(a_elems, elems_, args...),
                             select<Opt>(a_rhs, rhs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(HeadAggregate const &a, HeadAggregate const &b) -> bool;
    friend auto operator<(HeadAggregate const &a, HeadAggregate const &b) -> bool;
    friend struct Util::value_hasher<HeadAggregate>;

    Location loc_;
    AggregateFunction fun_;
    HeadAggregateElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

//! Compare two head aggregates elements.
//!
//! @related HeadAggregate
auto operator==(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool;

//! Compare two head aggregates elements.
//!
//! @related HeadAggregate
auto operator<(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool;

//! Compare two head aggregates.
//!
//! @related HeadAggregate
auto operator==(HeadAggregate const &a, HeadAggregate const &b) -> bool;

//! Compare two head aggregates.
//!
//! @related HeadAggregate
auto operator<(HeadAggregate const &a, HeadAggregate const &b) -> bool;

//! A head literal.
using HeadLiteral = std::variant<SimpleHeadLiteral, Disjunction, HeadAggregate, HeadSetAggregate, HeadTheoryAtom>;
//! A vector of head literals.
using HeadLiteralVec = Util::immutable_array<HeadLiteral>;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::SimpleHeadLiteral);
GRINGO_HASH_PROTO(Gringo::Input::Disjunction);
GRINGO_HASH_PROTO(Gringo::Input::HeadAggregateElement);
GRINGO_HASH_PROTO(Gringo::Input::HeadAggregate);

#endif
