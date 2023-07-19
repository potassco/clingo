#pragma once

#include <input/term.hh>

namespace Gringo::Input {

//! @defgroup literal Literals
//! @ingroup language
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

//! Literal representing a Boolean constant.
//!
//! For example <tt>\#true</tt>.
class LiteralBoolean {
  public:
    //! Construct a Boolean literal.
    LiteralBoolean(bool value) : LiteralBoolean{Sign::none, value} {}
    //! Construct a Boolean literal.
    LiteralBoolean(Sign sign, bool value) : sign(sign), value(value) {}

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
    LiteralRelation(Term lhs, GuardVec rhs) : LiteralRelation{Sign::none, std::move(lhs), std::move(rhs)} {}
    //! Construct a relation literal.
    LiteralRelation(Sign sign, Term lhs, GuardVec rhs) : sign(sign), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

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
    LiteralSymbolic(Term term) : LiteralSymbolic{Sign::none, std::move(term)} {}
    //! Construct a symbolic literal.
    LiteralSymbolic(Sign sign, Term term) : sign(sign), term(std::move(term)) {}

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

//! A vector of shared pointers to literals.
using LiteralVec = std::vector<Literal>;

//! A conditional literal.
struct ConditionalLiteral {
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

//! Add a sign to the literal.
void add_sign(Literal &lit, Sign sign);

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::LiteralBoolean)
GRINGO_HASH_PROTO(Gringo::Input::LiteralRelation)
GRINGO_HASH_PROTO(Gringo::Input::LiteralSymbolic)

#endif
