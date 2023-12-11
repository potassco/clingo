#pragma once

#include <input/term.hh>

namespace Gringo::Input {

//! @defgroup input_literal Literals
//! @ingroup input_language
//!
//! Data structures and functions to represent simple literals.
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

//! Simple struct with a sign.
struct Signed {
    //! The sign of the struct.
    Sign sign;
};

//! Simple struct without a sign.
struct Unsigned {};

//! Enumeration of relation symbols.
enum class Relation {
    less,          //!< The less than symbol (<).
    less_equal,    //!< The less than or equal to symbol (<=).
    greater,       //!< The greater than symbol (>).
    greater_equal, //!< The greater than or equal to symbol (>=).
    equal,         //!< The equal to symbol (=).
    inequal,       //!< The not equal to symbol (!=).
};

//! The right-hand-side of a relation atom including the symbol.
using Guard = std::pair<Relation, Term>;
//! A vector of guards.
using GuardVec = std::vector<Guard>;

[[nodiscard]] auto flip(Relation rel) -> Relation;
[[nodiscard]] auto complement(Relation rel) -> Relation;

//! Literal representing a Boolean constant.
//!
//! For example <tt>\#true</tt>.
class LiteralBoolean {
  public:
    //! Construct a Boolean literal.
    explicit LiteralBoolean(Location loc, Sign sign, bool value) : loc{std::move(loc)}, sign(sign), value(value) {}

    //! The location of the literal.
    Location loc;
    //! The sign of the literal.
    Sign sign;
    //! The Boolean value.
    bool value;
};

//! Check whether two Boolean literals are equivalent.
//!
//! \related LiteralBoolean
auto operator==(LiteralBoolean const &a, LiteralBoolean const &b) -> bool;

//! Literal representing a relation literal.
//!
//! For example <tt>1 <= X <= 10</tt>.
class LiteralRelation {
  public:
    //! Construct a relation literal.
    explicit LiteralRelation(Location loc, Sign sign, Term lhs, GuardVec rhs)
        : loc{std::move(loc)}, sign(sign), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    //! The location of the literal.
    Location loc;
    //! The sign of the literal.
    Sign sign;
    //! The term on the left hand side.
    Term lhs;
    //! The guards on the right hand side.
    GuardVec rhs;
};

//! Check whether two relation literals are equivalent.
//!
//! \related LiteralRelation
auto operator==(LiteralRelation const &a, LiteralRelation const &b) -> bool;

//! Literal representing a symbolic literal.
//!
//! For example <tt>not p(X)</tt>.
struct LiteralSymbolic {
  public:
    //! Construct a symbolic literal.
    explicit LiteralSymbolic(Location loc, Sign sign, Term term)
        : loc{std::move(loc)}, sign(sign), term(std::move(term)) {}

    //! The location of the literal.
    Location loc;
    //! The sign of the literal.
    Sign sign;
    //! The term representing the atom.
    Term term;
};

//! Check whether two symbolic literals are equivalent.
//!
//! \related LiteralSymbolic
auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool;

//! Variant holding the different literal types.
using Literal = std::variant<LiteralBoolean, LiteralRelation, LiteralSymbolic>;

//! A vector of literals.
using LiteralVec = std::vector<Literal>;

//! A vector of literal vectors.
using LiteralVecVec = std::vector<LiteralVec>;

//! A conditional literal.
struct ConditionalLiteral {
  public:
    //! Construct a conditional literal.
    explicit ConditionalLiteral(Location loc, LiteralVec lits, LiteralVec cond)
        : loc{std::move(loc)}, lits{std::move(lits)}, cond{std::move(cond)} {}
    //! The location of the literal.
    Location loc;
    //! The literals on the left-hand-side.
    LiteralVec lits;
    //! The literals on the right-hand-side.
    LiteralVec cond;
};

//! A vector of conditional literals.
//!
//! Can be either used in the head or body.
//!
//! \related ConditionalLiteral
using ConditionalLiteralVec = std::vector<ConditionalLiteral>;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::LiteralBoolean)
GRINGO_HASH_PROTO(Gringo::Input::LiteralRelation)
GRINGO_HASH_PROTO(Gringo::Input::LiteralSymbolic)

#endif
