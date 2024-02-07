#pragma once

#include <gringo/input/term.hh>

namespace Gringo::Input {

//! @defgroup input_literal Literals
//! Data structures and functions to represent simple literals.
//!
//! @ingroup input_language
//!
//! @{

//! Enumeration of signs (default negation).
enum class Sign {
    none,  //!< No sign.
    once,  //!< One sign (not).
    twice, //!< Two signs (not not).
};

//! Negate the sign.
auto operator-(Sign a) -> Sign;
//! Combine two signs.
auto operator+(Sign a, Sign b) -> Sign;
//! Combine two signs.
auto operator+=(Sign &a, Sign b) -> Sign &;

//! Simple class with a sign.
class Signed {
  public:
    explicit Signed(Sign sign) : sign_{sign} {}
    //! The sign of the class.
    Sign sign_;
};

//! Simple class without a sign.
class Unsigned {};

//! Enumeration of relation symbols.
enum class Relation {
    equal,         //!< The equal to symbol (=).
    inequal,       //!< The not equal to symbol (!=).
    less,          //!< The less than symbol (<).
    less_equal,    //!< The less than or equal to symbol (<=).
    greater,       //!< The greater than symbol (>).
    greater_equal, //!< The greater than or equal to symbol (>=).
};

//! The right-hand-side of a relation atom including the symbol.
using Guard = std::pair<Relation, Term>;
//! A vector of guards.
using GuardVec = Util::immutable_array<Guard>;

//! Return the equivalent relation when arguments are flipped.
[[nodiscard]] auto flip(Relation rel) -> Relation;
//! Return the complement of the given relation.
[[nodiscard]] auto complement(Relation rel) -> Relation;

//! Literal representing a Boolean constant.
//!
//! For example <tt>\#true</tt>.
class LiteralBoolean {
  public:
    //! Construct a Boolean literal.
    explicit LiteralBoolean(Location loc, Sign sign, bool value) : loc_{std::move(loc)}, sign_(sign), value_(value) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The sign of the literal.
    Sign sign_;
    //! The Boolean value.
    bool value_;
};

//! Check whether two Boolean literals are equivalent.
//!
//! \related LiteralBoolean
auto operator==(LiteralBoolean const &a, LiteralBoolean const &b) -> bool;

//! Compare two Boolean literals.
//!
//! \related LiteralBoolean
auto operator<(LiteralBoolean const &a, LiteralBoolean const &b) -> bool;

//! Literal representing a relation literal.
//!
//! For example <tt>1 <= X <= 10</tt>.
class LiteralRelation {
  public:
    //! Construct a relation literal.
    explicit LiteralRelation(Location loc, Sign sign, Term lhs, GuardVec rhs)
        : loc_{std::move(loc)}, sign_(sign), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The sign of the literal.
    Sign sign_;
    //! The term on the left hand side.
    Term lhs_;
    //! The guards on the right hand side.
    GuardVec rhs_;
};

//! Check whether two relation literals are equivalent.
//!
//! \related LiteralRelation
auto operator==(LiteralRelation const &a, LiteralRelation const &b) -> bool;

//! Compare two relation literals.
//!
//! \related LiteralRelation
auto operator<(LiteralRelation const &a, LiteralRelation const &b) -> bool;

//! Literal representing a symbolic literal.
//!
//! For example <tt>not p(X)</tt>.
class LiteralSymbolic {
  public:
    //! Construct a symbolic literal.
    explicit LiteralSymbolic(Location loc, Sign sign, Term term)
        : loc_{std::move(loc)}, sign_(sign), term_(std::move(term)) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The sign of the literal.
    Sign sign_;
    //! The term representing the atom.
    Term term_;
};

//! Check whether two symbolic literals are equivalent.
//!
//! \related LiteralSymbolic
auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool;

//! Compare two symbolic literals.
//!
//! \related LiteralSymbolic
auto operator<(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool;

//! Variant holding the different literal types.
using Literal = std::variant<LiteralBoolean, LiteralRelation, LiteralSymbolic>;

//! A vector of literals.
using LiteralVec = Util::immutable_array<Literal>;
//! A span of literals.
using LiteralSpan = tcb::span<Literal const>;

//! A vector of literal vectors.
using LiteralVecVec = Util::immutable_array<LiteralVec>;

//! A conditional literal.
class ConditionalLiteral {
  public:
    //! Construct a conditional literal.
    explicit ConditionalLiteral(Location loc, Literal lit, LiteralVec cond)
        : loc_{std::move(loc)}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;

  public:
    //! The literals on the left-hand-side.
    Literal lit_;
    //! The literals on the right-hand-side.
    LiteralVec cond_;
};

//! Check whether two symbolic literals are equivalent.
//!
//! \related LiteralSymbolic
auto operator==(ConditionalLiteral const &a, ConditionalLiteral const &b) -> bool;

//! Compare two symbolic literals.
//!
//! \related LiteralSymbolic
auto operator<(ConditionalLiteral const &a, ConditionalLiteral const &b) -> bool;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::LiteralBoolean)
GRINGO_HASH_PROTO(Gringo::Input::LiteralRelation)
GRINGO_HASH_PROTO(Gringo::Input::LiteralSymbolic)
GRINGO_HASH_PROTO(Gringo::Input::ConditionalLiteral)

#endif
