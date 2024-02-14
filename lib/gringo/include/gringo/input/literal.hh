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
    //! Construct with the given sign.
    explicit Signed(Sign sign) : sign_{sign} {}
    //! The sign of the class.
    [[nodiscard]] auto sign() const -> Sign { return sign_; };

  private:
    friend auto operator==(Signed const &a, Signed const &b) -> bool;
    friend auto operator<(Signed const &a, Signed const &b) -> bool;

    Sign sign_;
};

//! Compare the signs.
auto operator==(Signed const &a, Signed const &b) -> bool;

//! Compare the signs.
auto operator<(Signed const &a, Signed const &b) -> bool;

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
using GuardArray = Util::immutable_array<Guard>;

//! Return the equivalent relation when arguments are flipped.
[[nodiscard]] auto flip(Relation rel) -> Relation;
//! Return the complement of the given relation.
[[nodiscard]] auto complement(Relation rel) -> Relation;

//! Literal representing a Boolean constant.
//!
//! For example <tt>\#true</tt>.
class LitBool {
  public:
    //! Construct a Boolean literal.
    explicit LitBool(Location loc, Sign sign, bool value) : loc_{std::move(loc)}, sign_(sign), value_(value) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The Boolean value.
    [[nodiscard]] auto value() const -> bool { return value_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_sign, a_value}, Types{args...});
        return LitBool{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, sign_, args...),
                       select<Opt>(a_value, value_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(LitBool const &a, LitBool const &b) -> bool;
    friend auto operator<(LitBool const &a, LitBool const &b) -> bool;
    friend struct Util::value_hasher<LitBool>;

    Location loc_;
    Sign sign_;
    bool value_;
};

//! Check whether two Boolean literals are equivalent.
//!
//! \related LiteralBoolean
auto operator==(LitBool const &a, LitBool const &b) -> bool;

//! Compare two Boolean literals.
//!
//! \related LiteralBoolean
auto operator<(LitBool const &a, LitBool const &b) -> bool;

//! Literal representing a relation literal.
//!
//! For example <tt>1 <= X <= 10</tt>.
class LitComparison {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_sign, a_lhs, a_rhs}, Types{args...});
        return LitComparison{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, sign_, args...),
                             select<Opt>(a_lhs, lhs_, args...), select<Opt>(a_rhs, rhs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(LitComparison const &a, LitComparison const &b) -> bool;
    friend auto operator<(LitComparison const &a, LitComparison const &b) -> bool;
    friend struct Util::value_hasher<LitComparison>;

    Location loc_;
    Sign sign_;
    Term lhs_;
    GuardArray rhs_;
};

//! Check whether two relation literals are equivalent.
//!
//! \related LiteralRelation
auto operator==(LitComparison const &a, LitComparison const &b) -> bool;

//! Compare two relation literals.
//!
//! \related LiteralRelation
auto operator<(LitComparison const &a, LitComparison const &b) -> bool;

//! Literal representing a symbolic literal.
//!
//! For example <tt>not p(X)</tt>.
class LitSymbolic {
  public:
    //! Construct a symbolic literal.
    explicit LitSymbolic(Location loc, Sign sign, Term term)
        : loc_{std::move(loc)}, sign_(sign), term_(std::move(term)) {}

    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The sign of the literal.
    [[nodiscard]] auto sign() const -> Sign { return sign_; }
    //! The term representing the atom.
    [[nodiscard]] auto term() const -> Term const & { return term_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_sign, a_term}, Types{args...});
        return LitSymbolic{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, sign_, args...),
                           select<Opt>(a_term, term_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(LitSymbolic const &a, LitSymbolic const &b) -> bool;
    friend auto operator<(LitSymbolic const &a, LitSymbolic const &b) -> bool;
    friend struct Util::value_hasher<LitSymbolic>;

    Location loc_;
    Sign sign_;
    Term term_;
};

//! Check whether two symbolic literals are equivalent.
//!
//! \related LiteralSymbolic
auto operator==(LitSymbolic const &a, LitSymbolic const &b) -> bool;

//! Compare two symbolic literals.
//!
//! \related LiteralSymbolic
auto operator<(LitSymbolic const &a, LitSymbolic const &b) -> bool;

//! Variant holding the different literal types.
using Lit = std::variant<LitBool, LitComparison, LitSymbolic>;

//! A vector of literals.
using LitArray = Util::immutable_array<Lit>;

//! A conditional literal.
class CondLit {
  public:
    //! Construct a conditional literal.
    explicit CondLit(Location loc, Lit lit, LitArray cond)
        : loc_{std::move(loc)}, lit_{std::move(lit)}, cond_{std::move(cond)} {}
    //! The location of the literal.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The literals on the left-hand-side.
    [[nodiscard]] auto lit() const -> Lit const & { return lit_; }
    //! The literals on the right-hand-side.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_lit, a_cond}, Types{args...});
        return CondLit{select<Opt>(a_loc, loc_, args...), select<Opt>(a_lit, lit_, args...),
                       select<Opt>(a_cond, cond_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(CondLit const &a, CondLit const &b) -> bool;
    friend auto operator<(CondLit const &a, CondLit const &b) -> bool;
    friend struct Util::value_hasher<CondLit>;

    Location loc_;
    Lit lit_;
    LitArray cond_;
};

//! Check whether two symbolic literals are equivalent.
//!
//! \related LiteralSymbolic
auto operator==(CondLit const &a, CondLit const &b) -> bool;

//! Compare two symbolic literals.
//!
//! \related LiteralSymbolic
auto operator<(CondLit const &a, CondLit const &b) -> bool;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::LitBool)
GRINGO_HASH_PROTO(Gringo::Input::LitComparison)
GRINGO_HASH_PROTO(Gringo::Input::LitSymbolic)
GRINGO_HASH_PROTO(Gringo::Input::CondLit)

#endif
