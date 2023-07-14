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
using Guard = std::pair<Relation, STerm>;
//! A vector of guards.
using GuardVec = std::vector<Guard>;

class Literal;
class LiteralVisitor;
//! A shared pointer to a literal.
using SLiteral = Util::shared_ptr<Literal>;
//! A vector of shared pointers to literals.
using SLiteralVec = std::vector<SLiteral>;

//! The literal interface to implement simple literals.
//!
//! Simple literals are literals without aggregate-like conditions,
//! which can be used to build more complex literals.
class Literal {
  public:
    //! The virtual destructor.
    virtual ~Literal() = default;

    //! Convert the literal to a string.
    [[nodiscard]] auto to_string() const -> std::string;
    //! Equality compare two literals.
    friend auto operator==(Literal const &a, Literal const &b) { return a.is_equal(b); }

    //! Add a sign to the literal.
    //!
    //! Note that this function has to be used with care because the library uses shared pointers to literals.
    //! This function is currently only used during construction in the parser.
    virtual void add_sign(Sign sign) = 0;
    //! Remove all pooled arguments from the literal.
    [[nodiscard]] virtual auto unpool() const -> std::optional<SLiteralVec> = 0;
    //! Check whether the literal is an atoms.
    //!
    //! A literal is an atom if it is a symbolic literal without a sign.
    [[nodiscard]] virtual auto is_atom() const -> bool;
    //! Check whether the literal is a test.
    //!
    //! A test is a negated or not a symbolic literal.
    [[nodiscard]] virtual auto is_test() const -> bool;
    //! Equality compare two literals.
    //!
    //! The implementation uses a dynamic cast and assums that the inheritance structure is flat.
    [[nodiscard]] virtual auto is_equal(Literal const &other) const -> bool = 0;
    //! Compute a hash for the literal.
    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    //! Visit variables with the given function.
    virtual void visit_variables(VarVisitFun const &fun) const = 0;
    //! Project variables according to given projection mode.
    [[nodiscard]] virtual auto project(Projection project) const -> std::optional<SLiteral> = 0;
    //! Project anonymous variables in negated symbolic literals.
    //!
    //! This is a deprecated feature to support old programs.
    //! The projection star should be used instead.
    [[nodiscard]] virtual auto project_anonymous() const -> std::optional<SLiteral> = 0;
    // TODO: remove
    [[nodiscard]] auto rewrite_anonymous(NameGen &gen) const -> std::optional<SLiteral>;

    //! Visit literals with the given visitor.
    virtual void accept(LiteralVisitor const &visitor) const = 0;

  private:
    //! Increment reference count of the literal.
    friend void inc_ref_count(Literal &lit) { ++lit.refs_; }
    //! Decrement reference count of the literal.
    friend void dec_ref_count(Literal &lit) { ++lit.refs_; }
    //! Get reference count of the literal.
    friend auto get_ref_count(Literal const &lit) -> size_t { return lit.refs_; }

    //! The reference count of the literal.
    size_t refs_ = 0;
};

//! Literal representing a relation literal.
//!
//! For example <tt>1 <= X <= 10</tt>.
class LiteralRelation : public Literal {
  public:
    //! Construct a relation literal.
    LiteralRelation(STerm lhs, GuardVec rhs) : LiteralRelation{Sign::none, std::move(lhs), std::move(rhs)} {}
    //! Construct a relation literal.
    LiteralRelation(Sign sign, STerm lhs, GuardVec rhs) : sign_(sign), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    //! Get the sign of the symbolic literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! Get the left-hand-side of the relation literal.
    [[nodiscard]] auto lhs() const -> STerm const & { return lhs_; }
    //! Get the right-hand-side of the relation literal.
    [[nodiscard]] auto rhs() const -> GuardVec const & { return rhs_; }

    void add_sign(Sign s) override;
    [[nodiscard]] auto unpool() const -> std::optional<SLiteralVec> override;
    [[nodiscard]] auto is_equal(Literal const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SLiteral> override;

    void accept(LiteralVisitor const &visitor) const override;

  private:
    Sign sign_;
    STerm lhs_;
    GuardVec rhs_;
};

//! Literal representing a Boolean constant.
//!
//! For example <tt>#true</tt>.
class LiteralBoolean : public Literal {
  public:
    //! Construct a Boolean literal.
    LiteralBoolean(bool value) : LiteralBoolean{Sign::none, value} {}
    //! Construct a Boolean literal.
    LiteralBoolean(Sign sign, bool value) : sign_(sign), value_(value) {}

    //! Get the sign of the Boolean literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! Get the value of the Boolean literal.
    [[nodiscard]] auto value() const -> bool { return value_; }

    void add_sign(Sign s) override;
    [[nodiscard]] auto unpool() const -> std::optional<SLiteralVec> override;
    [[nodiscard]] auto is_equal(Literal const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SLiteral> override;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SLiteral> override;

    void accept(LiteralVisitor const &visitor) const override;

  private:
    Sign sign_;
    bool value_;
};

//! Literal representing a symbolic literal.
//!
//! For example <tt>not p(X)</tt>.
class LiteralSymbolic : public Literal {
  public:
    //! Construct a symbolic literal.
    LiteralSymbolic(STerm term) : LiteralSymbolic{Sign::none, std::move(term)} {}
    //! Construct a symbolic literal.
    LiteralSymbolic(Sign sign, STerm term) : sign_(sign), term_(std::move(term)) {}

    //! Get the sign of the symbolic literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! Get the (function) term representing the symbolic literal.
    [[nodiscard]] auto term() const -> STerm const & { return term_; }

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

  private:
    Sign sign_;
    STerm term_;
};

//! A visitor for available literal types.
class LiteralVisitor {
  public:
    //! Virtual destructor.
    virtual ~LiteralVisitor() = default;

    //! Visit a boolean literal.
    virtual void visit(LiteralBoolean const &lit) const = 0;
    //! Visit a relation literal.
    virtual void visit(LiteralRelation const &lit) const = 0;
    //! Visit a symbolic literal.
    virtual void visit(LiteralSymbolic const &lit) const = 0;
};

} // namespace Gringo::Input

HASH(Gringo::Input::Literal)
