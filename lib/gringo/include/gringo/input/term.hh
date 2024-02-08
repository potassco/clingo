#pragma once

#include <variant>
#include <vector>

#include <gringo/symbol.hh>

#include <gringo/util/hash.hh>
#include <gringo/util/immutable_array.hh>
#include <gringo/util/immutable_value.hh>
#include <gringo/util/optional.hh>

#include <gringo/input/location.hh>

namespace Gringo::Input {

//! @defgroup input Input
//! Data structures and functions to parse and rewrite the gringo language.
//!
//! @ingroup API

//! @defgroup input_language Language
//! Data structures and functions to capture the gringo language.
//!
//! @ingroup input

//! @defgroup input_term Terms
//! Data structures and functions to represent terms.
//!
//! @ingroup input_language
//!
//! @{

//! A set of variable names.
using VariableSet = StringSet;
//! A vector of variable names.
using VariableVec = std::vector<String>;

class TermVariable;
class TermSymbol;
class TermTuple;
class TermFunction;
class TermAbs;
class TermUnary;
class TermBinary;

//! Variant holding the different term types.
using Term = std::variant<TermVariable, TermSymbol, TermTuple, TermFunction, TermAbs, TermUnary, TermBinary>;

//! A vector of terms.
using TermVec = Util::immutable_array<Term>;
using TermSpan = tcb::span<Term const>;

//! Indicate a projected position.
class Projection {
  public:
    //! Construct a projection indicator.
    explicit Projection(Location loc) : loc_{loc} {}
    //! The location of the projected position.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    friend auto operator==(Projection const &a, Projection const &b) -> bool;
    friend auto operator<(Projection const &a, Projection const &b) -> bool;
    friend struct Util::value_hasher<Projection>;

    Location loc_;
};

//! Compare two projected positions.
//!
//! @related Projection
auto operator==(Projection const &a, Projection const &b) -> bool;

//! Compare two projected positions.
//!
//! @related Projection
auto operator<(Projection const &a, Projection const &b) -> bool;

class ArgumentTuple {
  public:
    //! A variant capturing either a term or a position that is to be projected.
    using Element = std::variant<Projection, Term>;
    //! A tuple of terms or positions to project.
    using ElementVec = Util::immutable_array<Element>;
    //! A tuple of terms or positions to project.
    using ElementSpan = tcb::span<Element const>;

    //! Construct an argument tuple.
    ArgumentTuple(ElementVec elems);

    //! Update tuple.
    [[nodiscard]] auto update(std::optional<ElementVec> elems) const -> ArgumentTuple;
    //! Update tuple.
    [[nodiscard]] auto opt_update(std::optional<ElementVec> elems) const -> std::optional<ArgumentTuple>;

    //! The elements of the tuple.
    [[nodiscard]] auto elems() const -> ElementSpan;

  private:
    friend auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool;
    friend auto operator<(ArgumentTuple const &a, ArgumentTuple const &b) -> bool;
    friend struct Util::value_hasher<ArgumentTuple>;

    ElementVec elems_;
};

//! Compare two argument tuples.
//!
//! @related ArgumentTuple
auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool;

//! Compare two argument tuples.
//!
//! @related ArgumentTuple
auto operator<(ArgumentTuple const &a, ArgumentTuple const &b) -> bool;

//! A vector of tuples used as function or predicate arguments.
using PoolVec = Util::immutable_array<ArgumentTuple>;
//! A span of tuples used as function or predicate arguments.
using PoolSpan = tcb::span<ArgumentTuple const>;

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
class TermVariable {
  public:
    //! Construct a variable.
    explicit TermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{std::move(name)}, is_anonymous_{is_anonymous} {}

    //! Update term.
    [[nodiscard]] auto update(std::optional<Location> loc, std::optional<String> name,
                              std::optional<bool> is_anonymous) const -> TermVariable;
    //! Update term.
    [[nodiscard]] auto opt_update(std::optional<Location> loc, std::optional<String> name,
                                  std::optional<bool> is_anonymous) const -> std::optional<TermVariable>;

    // could as well be something like:
    //   update<TermVariable>(term.loc(), uv(opt, term.name()), term.is_anonymous())
    //   opt_update<TermVariable>(term.loc(), uv(opt, term.name()), term.is_anonymous())
    // for immutable value:
    //   update<TermUnary>(term.loc(), uv(opt, term.rhs_raw()))
    // for immutable array:
    //   just return the array and get rid of span
    // struct uv {
    //   N opt_new;
    //   O const &old;
    // }

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto is_anonymous() const -> bool { return is_anonymous_; }

  private:
    friend auto operator==(TermVariable const &a, TermVariable const &b) -> bool;
    friend auto operator<(TermVariable const &a, TermVariable const &b) -> bool;
    friend struct Util::value_hasher<TermVariable>;

    Location loc_;
    String name_;
    bool is_anonymous_;
};

//! Compare two variables.
//!
//! @related TermVariable
auto operator==(TermVariable const &a, TermVariable const &b) -> bool;

//! Compare two variables.
//!
//! @related TermVariable
auto operator<(TermVariable const &a, TermVariable const &b) -> bool;

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
class TermSymbol {
  public:
    //! Construct term with the given symbol.
    explicit TermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{std::move(value)} {}

    //! Update term.
    [[nodiscard]] auto update(std::optional<Location> loc, std::optional<Symbol> value) const -> TermSymbol;
    //! Update term.
    [[nodiscard]] auto opt_update(std::optional<Location> loc, std::optional<Symbol> value) const
        -> std::optional<TermSymbol>;

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The associated symbol.
    [[nodiscard]] auto value() const -> Symbol { return value_; }

  private:
    friend auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool;
    friend auto operator<(TermSymbol const &a, TermSymbol const &b) -> bool;
    friend struct Util::value_hasher<TermSymbol>;

    Location loc_;
    Symbol value_;
};

//! Compare two symbols.
//!
//! @related TermSymbol
auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool;

//! Compare two symbols.
//!
//! @related TermSymbol
auto operator<(TermSymbol const &a, TermSymbol const &b) -> bool;

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
class TermTuple {
  public:
    //! A tuple element.
    using Element = std::variant<ArgumentTuple, Term>;
    //! A vector of tuple elements.
    using ElementVec = Util::immutable_array<Element>;
    //! A span of tuple elements.
    using ElementSpan = tcb::span<Element const>;

    //! Construct a  tuple.
    explicit TermTuple(Location loc, ElementVec pool);

    //! Update term.
    [[nodiscard]] auto update(std::optional<Location> loc, std::optional<ElementVec> pool) const -> TermTuple;
    //! Update term.
    [[nodiscard]] auto opt_update(std::optional<Location> loc, std::optional<ElementVec> pool) const
        -> std::optional<TermTuple>;

    //! The location of the tuple.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The argument pool of the tuple.
    [[nodiscard]] auto pool() const -> ElementSpan;

  private:
    friend auto operator==(TermTuple const &a, TermTuple const &b) -> bool;
    friend auto operator<(TermTuple const &a, TermTuple const &b) -> bool;
    friend struct Util::value_hasher<TermTuple>;

    Location loc_;
    ElementVec pool_;
};

//! Compare two tuple terms.
//!
//! @related TermTuple
auto operator==(TermTuple const &a, TermTuple const &b) -> bool;

//! Compare two tuple terms.
//!
//! @related TermTuple
auto operator<(TermTuple const &a, TermTuple const &b) -> bool;

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
class TermFunction {
  public:
    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(Location loc, String name, PoolVec pool, bool external);

    //! Update term.
    [[nodiscard]] auto update(std::optional<Location> loc, std::optional<String> name, std::optional<PoolVec> pool,
                              std::optional<bool> external) const -> TermFunction;
    //! Update term.
    [[nodiscard]] auto opt_update(std::optional<Location> loc, std::optional<String> name, std::optional<PoolVec> pool,
                                  std::optional<bool> external) const -> std::optional<TermFunction>;

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the function.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The argument pool of the function.
    [[nodiscard]] auto pool() const -> PoolSpan { return pool_; }
    //! Whether this is an external function.
    [[nodiscard]] auto external() const -> bool { return external_; }

  private:
    friend auto operator==(TermFunction const &a, TermFunction const &b) -> bool;
    friend auto operator<(TermFunction const &a, TermFunction const &b) -> bool;
    friend struct Util::value_hasher<TermFunction>;

    Location loc_;
    String name_;
    PoolVec pool_;
    bool external_;
};

//! Compare two function terms.
//!
//! @related TermFunction
auto operator==(TermFunction const &a, TermFunction const &b) -> bool;

//! Compare two function terms.
//!
//! @related TermFunction
auto operator<(TermFunction const &a, TermFunction const &b) -> bool;

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
class TermAbs {
  public:
    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(Location loc, TermVec pool);

    //! Update term.
    [[nodiscard]] auto update(std::optional<Location> loc, std::optional<TermVec> pool) const -> TermAbs;
    //! Update term.
    [[nodiscard]] auto opt_update(std::optional<Location> loc, std::optional<TermVec> pool) const
        -> std::optional<TermAbs>;

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The argument pool of the absolute term.
    [[nodiscard]] auto pool() const -> TermSpan;

  private:
    friend auto operator==(TermAbs const &a, TermAbs const &b) -> bool;
    friend auto operator<(TermAbs const &a, TermAbs const &b) -> bool;
    friend struct Util::value_hasher<TermAbs>;

    Location loc_;
    TermVec pool_;
};

//! Compare two absolute terms.
//!
//! @related TermAbs
auto operator==(TermAbs const &a, TermAbs const &b) -> bool;

//! Compare two absolute terms.
//!
//! @related TermAbs
auto operator<(TermAbs const &a, TermAbs const &b) -> bool;

//! Enumeration of available unary operators.
enum class UnaryOperator : int {
    negate, //!< The unary minus sign (-).
    invert, //!< The unary negation sign (~).
};

//! Term representing an unary operation.
//!
//! For example <tt>-X</tt>.
class TermUnary {
  public:
    //! Construct a term for an unary operation.
    explicit TermUnary(Location loc, UnaryOperator op, Term rhs);
    //! Construct a term for an unary operation.
    explicit TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs);

    //! Update term.
    [[nodiscard]] auto update(std::optional<Location> loc, std::optional<UnaryOperator> op,
                              std::optional<Term> rhs) const -> TermUnary;
    //! Update term.
    [[nodiscard]] auto opt_update(std::optional<Location> loc, std::optional<UnaryOperator> op,
                                  std::optional<Term> rhs) const -> std::optional<TermUnary>;

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The operation.
    [[nodiscard]] auto op() const -> UnaryOperator { return op_; }
    //! The right-hand-side.
    [[nodiscard]] auto rhs() const -> Term const &;

  private:
    friend auto operator==(TermUnary const &a, TermUnary const &b) -> bool;
    friend auto operator<(TermUnary const &a, TermUnary const &b) -> bool;
    friend struct Util::value_hasher<TermUnary>;

    Location loc_;
    UnaryOperator op_;
    Util::immutable_value<Term> rhs_;
};

//! Compare two unary terms.
//!
//! @related TermUnary
auto operator==(TermUnary const &a, TermUnary const &b) -> bool;

//! Compare two unary terms.
//!
//! @related TermUnary
auto operator<(TermUnary const &a, TermUnary const &b) -> bool;

//! Enumeration of available binary operators.
enum class BinaryOperator : int {
    and_,  //!< The AND bit operation.
    div,   //!< The (integer) divide arithmetic operation.
    minus, //!< The minus arithmetic operation.
    mod,   //!< The modulo arithmetic operation.
    times, //!< The multiply arithmetic operation.
    or_,   //!< The OR bit operation.
    plus,  //!< The plus arithmetic operation.
    pow,   //!< The exponentiation arithmetic operation.
    xor_,  //!< The XOR bit operation.
    dots,  //!< The interval operator.
};

//! Term representing a binary operation.
//!
//! For example <tt>X-Y</tt>.
class TermBinary {
  public:
    //! Construct a term for an binary operation.
    explicit TermBinary(Location loc, Term lhs, BinaryOperator op, Term rhs);
    //! Construct a term for an binary operation.
    explicit TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                        Util::immutable_value<Term> rhs);

    //! Update term.
    [[nodiscard]] auto update(std::optional<Location> loc, std::optional<Term> lhs, std::optional<BinaryOperator> op,
                              std::optional<Term> rhs) const -> TermBinary;
    //! Update term.
    [[nodiscard]] auto opt_update(std::optional<Location> loc, std::optional<Term> lhs,
                                  std::optional<BinaryOperator> op, std::optional<Term> rhs) const
        -> std::optional<TermBinary>;

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The left-hand-side.
    [[nodiscard]] auto lhs() const -> Term const &;
    //! The right-hand-side.
    [[nodiscard]] auto rhs() const -> Term const &;
    //! The operation.
    [[nodiscard]] auto op() const -> BinaryOperator { return op_; }

  private:
    friend auto operator==(TermBinary const &a, TermBinary const &b) -> bool;
    friend auto operator<(TermBinary const &a, TermBinary const &b) -> bool;
    friend struct Util::value_hasher<TermBinary>;

    Location loc_;
    Util::immutable_value<Term> lhs_;
    Util::immutable_value<Term> rhs_;
    BinaryOperator op_;
};

//! Compare two binary terms.
//!
//! @related TermBinary
auto operator==(TermBinary const &a, TermBinary const &b) -> bool;

//! Compare two binary terms.
//!
//! @related TermBinary
auto operator<(TermBinary const &a, TermBinary const &b) -> bool;

//! @}

// Note that constructors are defined here because at this point all types are
// complete.

inline ArgumentTuple::ArgumentTuple(ElementVec elems) : elems_{std::move(elems)} {}

inline auto ArgumentTuple::update(std::optional<ElementVec> elems) const -> ArgumentTuple {
    return {Util::update_value(std::move(elems), elems_)};
}

inline auto ArgumentTuple::opt_update(std::optional<ElementVec> elems) const -> std::optional<ArgumentTuple> {
    if (!elems) {
        return std::nullopt;
    }
    return update(std::move(elems));
}

inline auto ArgumentTuple::elems() const -> ElementSpan { return elems_; }

inline auto TermVariable::update(std::optional<Location> loc, std::optional<String> name,
                                 std::optional<bool> is_anonymous) const -> TermVariable {
    return TermVariable(std::move(loc).value_or(loc_), std::move(name).value_or(name_),
                        Util::update_value(std::move(is_anonymous), is_anonymous_));
}

inline auto TermVariable::opt_update(std::optional<Location> loc, std::optional<String> name,
                                     std::optional<bool> is_anonymous) const -> std::optional<TermVariable> {
    if (!loc && !name && !is_anonymous) {
        return std::nullopt;
    }
    return update(std::move(loc), std::move(name), std::move(is_anonymous));
}

inline auto TermSymbol::update(std::optional<Location> loc, std::optional<Symbol> value) const -> TermSymbol {
    return TermSymbol(std::move(loc).value_or(loc_), std::move(value).value_or(value_));
}

inline auto TermSymbol::opt_update(std::optional<Location> loc, std::optional<Symbol> value) const
    -> std::optional<TermSymbol> {
    if (!loc && !value) {
        return std::nullopt;
    }
    return update(std::move(loc), std::move(value));
}

inline TermTuple::TermTuple(Location loc, ElementVec pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {}

inline auto TermTuple::pool() const -> TermTuple::ElementSpan { return tcb::make_span(pool_); }

inline auto TermTuple::update(std::optional<Location> loc, std::optional<ElementVec> pool) const -> TermTuple {
    return TermTuple(std::move(loc).value_or(loc_), Util::update_value(std::move(pool), pool_));
}

inline auto TermTuple::opt_update(std::optional<Location> loc, std::optional<ElementVec> pool) const
    -> std::optional<TermTuple> {
    if (!loc && !pool) {
        return std::nullopt;
    }
    return update(std::move(loc), std::move(pool));
}

inline TermFunction::TermFunction(Location loc, String name, PoolVec pool, bool external)
    : loc_{std::move(loc)}, name_(std::move(name)), pool_{std::move(pool)}, external_{external} {}

inline auto TermFunction::update(std::optional<Location> loc, std::optional<String> name, std::optional<PoolVec> pool,
                                 std::optional<bool> external) const -> TermFunction {
    return TermFunction(std::move(loc).value_or(loc_), std::move(name).value_or(name_),
                        Util::update_value(std::move(pool), pool_), std::move(external).value_or(external_));
}

inline auto TermFunction::opt_update(std::optional<Location> loc, std::optional<String> name,
                                     std::optional<PoolVec> pool, std::optional<bool> external) const
    -> std::optional<TermFunction> {
    if (!loc && !name && !pool && !external) {
        return std::nullopt;
    }
    return update(std::move(loc), std::move(name), std::move(pool), std::move(external));
}

inline TermAbs::TermAbs(Location loc, TermVec pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {}

inline auto TermAbs::update(std::optional<Location> loc, std::optional<TermVec> pool) const -> TermAbs {
    return TermAbs(std::move(loc).value_or(loc_), Util::update_value(std::move(pool), pool_));
}

inline auto TermAbs::opt_update(std::optional<Location> loc, std::optional<TermVec> pool) const
    -> std::optional<TermAbs> {
    if (!loc && !pool) {
        return std::nullopt;
    }
    return update(std::move(loc), std::move(pool));
}

inline auto TermAbs::pool() const -> TermSpan { return tcb::make_span(pool_); }

inline TermUnary::TermUnary(Location loc, UnaryOperator op, Term rhs)
    : loc_{std::move(loc)}, op_{op}, rhs_{Util::make_immutable<Term>(std::move(rhs))} {}

inline TermUnary::TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, op_{op}, rhs_{std::move(rhs)} {}

inline auto TermUnary::update(std::optional<Location> loc, std::optional<UnaryOperator> op,
                              std::optional<Term> rhs) const -> TermUnary {
    return TermUnary(std::move(loc).value_or(loc_), std::move(op).value_or(op_),
                     Util::update_value(std::move(rhs), rhs_));
}

inline auto TermUnary::opt_update(std::optional<Location> loc, std::optional<UnaryOperator> op,
                                  std::optional<Term> rhs) const -> std::optional<TermUnary> {
    if (!loc && !op && !rhs) {
        return std::nullopt;
    }
    return update(std::move(loc), std::move(op), std::move(rhs));
}

inline auto TermUnary::rhs() const -> Term const & { return *rhs_; }

inline TermBinary::TermBinary(Location loc, Term lhs, BinaryOperator op, Term rhs)
    : loc_{std::move(loc)}, lhs_{Util::make_immutable<Term>(std::move(lhs))},
      rhs_{Util::make_immutable<Term>(std::move(rhs))}, op_{op} {}

inline TermBinary::TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                              Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {}

inline auto TermBinary::update(std::optional<Location> loc, std::optional<Term> lhs, std::optional<BinaryOperator> op,
                               std::optional<Term> rhs) const -> TermBinary {
    return TermBinary(std::move(loc).value_or(loc_), Util::update_value(std::move(lhs), lhs_),
                      std::move(op).value_or(op_), Util::update_value(std::move(rhs), rhs_));
}

inline auto TermBinary::opt_update(std::optional<Location> loc, std::optional<Term> lhs,
                                   std::optional<BinaryOperator> op, std::optional<Term> rhs) const
    -> std::optional<TermBinary> {
    if (!loc && !lhs && !op && !rhs) {
        return std::nullopt;
    }
    return update(std::move(loc), std::move(lhs), std::move(op), std::move(rhs));
}

inline auto TermBinary::lhs() const -> Term const & { return *lhs_; }

inline auto TermBinary::rhs() const -> Term const & { return *rhs_; }

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::ArgumentTuple);
GRINGO_HASH_PROTO(Gringo::Input::TermVariable);
GRINGO_HASH_PROTO(Gringo::Input::TermSymbol);
GRINGO_HASH_PROTO(Gringo::Input::TermFunction);
GRINGO_HASH_PROTO(Gringo::Input::TermTuple);
GRINGO_HASH_PROTO(Gringo::Input::TermAbs);
GRINGO_HASH_PROTO(Gringo::Input::TermUnary);
GRINGO_HASH_PROTO(Gringo::Input::TermBinary);
GRINGO_HASH_PROTO(Gringo::Input::Projection);

#endif
