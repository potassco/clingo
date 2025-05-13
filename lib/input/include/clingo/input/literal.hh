#pragma once

#include <clingo/input/term.hh>

#include <clingo/core/core.hh>

#include <utility>

namespace CppClingo::Input {

//! @addtogroup input_literal
//! @{

//! Simple class with a sign.
class Signed {
  public:
    //! Construct with the given sign.
    explicit Signed(Sign sign) : sign_{sign} {}
    //! The sign of the class.
    [[nodiscard]] auto sign() const -> Sign { return sign_; };

    //! Compare the signs.
    friend auto operator==(Signed const &a, Signed const &b) -> bool = default;
    //! Compare the signs.
    friend auto operator<=>(Signed const &a, Signed const &b) -> std::strong_ordering = default;

  private:
    Sign sign_;
};

//! Simple class without a sign.
class Unsigned {
    //! Compare the signs.
    friend auto operator==(Unsigned const &a, Unsigned const &b) -> bool = default;
    //! Compare the signs.
    friend auto operator<=>(Unsigned const &a, Unsigned const &b) -> std::strong_ordering = default;
};

//! The right-hand-side of a relation atom including the symbol.
using Guard = std::pair<Relation, Term>;
//! A vector of guards.
using GuardArray = Util::immutable_array<Guard>;

//! Literal representing a Boolean constant.
//!
//! For example <tt>\#true</tt>.
class LitBool : public Expression<LitBool> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &LitBool::loc_, a_sign = &LitBool::sign_, a_value = &LitBool::value_};
    }

    //! Construct a Boolean literal.
    explicit LitBool(Location loc, Sign sign, bool value) : loc_{std::move(loc)}, sign_(sign), value_(value) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The Boolean value.
    [[nodiscard]] auto value() const -> bool { return value_; }

  private:
    Location loc_;
    Sign sign_;
    bool value_;
};

//! Literal representing a relation literal.
//!
//! For example <tt>1 <= X <= 10</tt>.
class LitComparison : public Expression<LitComparison> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &LitComparison::loc_, a_sign = &LitComparison::sign_, a_lhs = &LitComparison::lhs_,
                          a_rhs = &LitComparison::rhs_};
    }

    //! Construct a relation literal.
    explicit LitComparison(Location loc, Sign sign, Term lhs, GuardArray rhs)
        : loc_{std::move(loc)}, sign_(sign), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The term on the left hand side.
    [[nodiscard]] auto lhs() const -> Term const & { return lhs_; }
    //! The guards on the right hand side.
    [[nodiscard]] auto rhs() const -> GuardArray const & { return rhs_; }

  private:
    Location loc_;
    Sign sign_;
    Term lhs_;
    GuardArray rhs_;
};

//! Literal representing a symbolic literal.
//!
//! For example <tt>not p(X)</tt>.
class LitSymbolic : public Expression<LitSymbolic> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &LitSymbolic::loc_, a_sign = &LitSymbolic::sign_, a_term = &LitSymbolic::term_};
    }

    //! Construct a symbolic literal.
    explicit LitSymbolic(Location loc, Sign sign, Term term)
        : loc_{std::move(loc)}, sign_(sign), term_(std::move(term)) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The term representing the atom.
    [[nodiscard]] auto term() const -> Term const & { return term_; }

  private:
    Location loc_;
    Sign sign_;
    Term term_;
};

//! Variant holding the different literal types.
using Lit = std::variant<LitBool, LitComparison, LitSymbolic>;

//! A vector of literals.
using LitArray = Util::immutable_array<Lit>;

//! A conditional literal.
class CondLit : public Expression<CondLit> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &CondLit::loc_, a_lit = &CondLit::lit_, a_cond = &CondLit::cond_};
    }

    //! Construct a conditional literal.
    explicit CondLit(Location loc, Lit lit, LitArray cond)
        : loc_{std::move(loc)}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The literals on the left-hand-side.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }
    //! The literals on the right-hand-side.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

  private:
    Location loc_;
    Lit lit_;
    LitArray cond_;
};

//! @}

} // namespace CppClingo::Input
