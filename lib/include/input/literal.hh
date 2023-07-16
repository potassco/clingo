#pragma once

//! @file
//! This file contains the literal interface and derived literals.

#include <input/term.hh>

namespace Gringo::Input {

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
//!
//! @see LiteralRelation
using Guard = std::pair<Relation, Term>;
//! A vector of guards.
using GuardVec = std::vector<Guard>;

//! Literal representing a Boolean constant.
//!
//! For example <tt>#true</tt>.
class LiteralBoolean {
  public:
    //! Construct a Boolean literal.
    LiteralBoolean(bool value) : LiteralBoolean{Sign::none, value} {}
    //! Construct a Boolean literal.
    LiteralBoolean(Sign sign, bool value) : sign(sign), value(value) {}

    Sign sign;
    bool value;
};

auto operator==(LiteralBoolean const &a, LiteralBoolean const &b);

//! Literal representing a relation literal.
//!
//! For example <tt>1 <= X <= 10</tt>.
class LiteralRelation {
  public:
    //! Construct a relation literal.
    LiteralRelation(Term lhs, GuardVec rhs) : LiteralRelation{Sign::none, std::move(lhs), std::move(rhs)} {}
    //! Construct a relation literal.
    LiteralRelation(Sign sign, Term lhs, GuardVec rhs) : sign(sign), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    Sign sign;
    Term lhs;
    GuardVec rhs;
};

auto operator==(LiteralRelation const &a, LiteralRelation const &b);

//! Literal representing a symbolic literal.
//!
//! For example <tt>not p(X)</tt>.
struct LiteralSymbolic {
  public:
    //! Construct a symbolic literal.
    LiteralSymbolic(Term term) : LiteralSymbolic{Sign::none, std::move(term)} {}
    //! Construct a symbolic literal.
    LiteralSymbolic(Sign sign, Term term) : sign(sign), term(std::move(term)) {}

    /*
    void add_sign(Sign s) override;
    [[nodiscard]] auto unpool() const -> std::optional<SLiteralVec> override;
    [[nodiscard]] auto is_equal(Literal const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SLiteral> override;
    [[nodiscard]] auto is_atom() const -> bool override;
    [[nodiscard]] auto is_test() const -> bool override;

    void accept(LiteralVisitor const &visitor) const override;
    */

    Sign sign;
    Term term;
};

auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b);

//! Variant holding the different literal types.
using Literal = std::variant<LiteralBoolean, LiteralRelation, LiteralSymbolic>;

//! A vector of shared pointers to literals.
using LiteralVec = std::vector<Literal>;

// TODO: move somewhere or maybe even keep here
void add_sign(Literal &lit, Sign sign);
auto is_atom(Literal const &lit) -> bool;
auto is_test(Literal const &lit) -> bool;

} // namespace Gringo::Input

HASH_PROTO(Gringo::Input::LiteralBoolean)
HASH_PROTO(Gringo::Input::LiteralRelation)
HASH_PROTO(Gringo::Input::LiteralSymbolic)
