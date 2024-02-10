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
class SimpleBodyLiteral {
  public:
    //! Wrap a literal in a body literal.
    SimpleBodyLiteral(Literal lit) : lit_{std::move(lit)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return location(lit_); }
    //! The literal.
    [[nodiscard]] auto lit() const -> Literal const & { return lit_; }

  private:
    friend auto operator==(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool;
    friend auto operator<(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool;
    friend struct Util::value_hasher<SimpleBodyLiteral>;

    Literal lit_;
};

//! Compare two literals.
//!
//! @related SimpleBodyLiteral
auto operator==(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool;

//! Compare two literals.
//!
//! @related SimpleBodyLiteral
auto operator<(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool;

//! A conditional literal in a rule body.
class Conjunction {
  public:
    //! Construct a conjunction.
    Conjunction(ConditionalLiteral lit) : lit_{std::move(lit)} {}
    //! Get the location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return lit_.loc(); }
    //! The conditional literal representing the elements of the conjunction.
    [[nodiscard]] auto lit() const -> ConditionalLiteral const & { return lit_; }

  private:
    friend auto operator==(Conjunction const &a, Conjunction const &b) -> bool;
    friend auto operator<(Conjunction const &a, Conjunction const &b) -> bool;
    friend struct Util::value_hasher<Conjunction>;

    ConditionalLiteral lit_;
};

//! Compare two body conjunctions.
//!
//! @related Conjunction
auto operator==(Conjunction const &a, Conjunction const &b) -> bool;

//! Compare two body conjunction.
//!
//! @related Conjunction
auto operator<(Conjunction const &a, Conjunction const &b) -> bool;

//! A body aggregate.
//!
//! For example: <tt>\#count { X: q(X) } = 1</tt>
class BodyAggregate {
  public:
    //! An aggregate element.
    class Element {
      public:
        explicit Element(Location loc, TermVec tuple, LiteralVec cond)
            : loc_{loc}, tuple_{std::move(tuple)}, cond_{std::move(cond)} {}
        //! The location of the element.
        [[nodiscard]] auto loc() const -> Location const & { return loc_; }
        //! The tuple of the element.
        [[nodiscard]] auto tuple() const -> TermVec const & { return tuple_; }
        //! The condition of the element.
        [[nodiscard]] auto cond() const -> LiteralVec const & { return cond_; }

      private:
        friend auto operator==(Element const &a, Element const &b) -> bool;
        friend auto operator<(Element const &a, Element const &b) -> bool;
        friend struct Util::value_hasher<Element>;

        Location loc_;
        TermVec tuple_;
        LiteralVec cond_;
    };
    //! A vector of aggregate elements.
    using ElementVec = Util::immutable_array<Element>;

    //! Construct a body aggregate.
    explicit BodyAggregate(Location loc, Sign sign, LGuard lhs, AggregateFunction fun, ElementVec elems, RGuard rhs)
        : loc_{std::move(loc)}, sign_{sign}, fun_(fun), elems_(std::move(elems)), lhs_{std::move(lhs)},
          rhs_{std::move(rhs)} {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction { return fun_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> ElementVec const & { return elems_; }
    //! An optional left guard.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! An optional right guard.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

  private:
    friend auto operator==(BodyAggregate const &a, BodyAggregate const &b) -> bool;
    friend auto operator<(BodyAggregate const &a, BodyAggregate const &b) -> bool;
    friend struct Util::value_hasher<BodyAggregate>;

    Location loc_;
    Sign sign_;
    AggregateFunction fun_;
    ElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

//! Compare two body aggregates elements.
//!
//! @related BodyAggregate
auto operator==(BodyAggregate::Element const &a, BodyAggregate::Element const &b) -> bool;

//! Compare two body aggregates elements.
//!
//! @related BodyAggregate
auto operator<(BodyAggregate::Element const &a, BodyAggregate::Element const &b) -> bool;

//! Compare two body aggregates.
//!
//! @related BodyAggregate
auto operator==(BodyAggregate const &a, BodyAggregate const &b) -> bool;

//! Compare two body aggregates.
//!
//! @related BodyAggregate
auto operator<(BodyAggregate const &a, BodyAggregate const &b) -> bool;

//! A body theory atom.
using BodyTheoryAtom = TheoryAtom<true>;

//! A body literal.
using BodyLiteral = std::variant<SimpleBodyLiteral, Conjunction, BodyAggregate, BodySetAggregate, BodyTheoryAtom>;
//! A vector of body literals.
using BodyLiteralVec = Util::immutable_array<BodyLiteral>;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::SimpleBodyLiteral);
GRINGO_HASH_PROTO(Gringo::Input::Conjunction);
GRINGO_HASH_PROTO(Gringo::Input::BodyAggregate::Element);
GRINGO_HASH_PROTO(Gringo::Input::BodyAggregate);

#endif
