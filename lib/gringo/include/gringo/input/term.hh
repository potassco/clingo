#pragma once

#include <variant>
#include <vector>

#include <gringo/symbol.hh>

#include <gringo/util/hash.hh>
#include <gringo/util/immutable_array.hh>
#include <gringo/util/immutable_value.hh>

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

struct TermVariable;
struct TermSymbol;
struct TermTuple;
struct TermFunction;
struct TermAbs;
struct TermUnary;
struct TermBinary;

//! Variant holding the different term types.
using Term = std::variant<TermVariable, TermSymbol, TermTuple, TermFunction, TermAbs, TermUnary, TermBinary>;

//! A vector of terms.
using TermVec = Util::immutable_array<Term>;

//! Indicate a projected position.
struct Projection {
    //! The location of the projected position.
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

//! A variant capturing either a term or a position that is to be projected.
using TupleElem = std::variant<Projection, Term>;
//! A tuple of terms or positions to project.
using TupleVec = Util::immutable_array<TupleElem>;
//! A vector of tuples used as function or predicate arguments.
using PoolVec = Util::immutable_array<TupleVec>;

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
struct TermVariable {
    //! Construct a variable.
    explicit TermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{std::move(name)}, is_anonymous_{is_anonymous} {}

    //! The location of the variable.
    Location loc_;
    //! The name of the variable.
    String name_;
    //! Whether the variable is anonymous.
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
struct TermSymbol {
    //! Construct term with the given symbol.
    explicit TermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{std::move(value)} {}

    //! The location of the symbol.
    Location loc_;
    //! The associated symbol.
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
struct TermTuple {
    //! A tuple element.
    using Element = std::variant<TupleVec, Term>;
    //! A vector of tuple elements.
    using ElementVec = Util::immutable_array<Element>;

    //! Construct a  tuple.
    explicit TermTuple(Location loc, ElementVec args);

    //! The location of the tuple.
    Location loc_;
    //! The argument pool of the tuple.
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
struct TermFunction {
    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(Location loc, String name, PoolVec args, bool external);

    //! The location of the function.
    Location loc_;
    //! The name of the function.
    String name_;
    //! The argument pool of the function.
    PoolVec pool_;
    //! Whether this is an external function.
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
struct TermAbs {
    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(Location loc, TermVec pool);

    //! The location of the function.
    Location loc_;
    //! The argument pool of the absolute term.
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
struct TermUnary {
    //! Construct a term for an unary operation.
    explicit TermUnary(Location loc, UnaryOperator op, Term rhs);
    //! Construct a term for an unary operation.
    explicit TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs);

    //! The location of the function.
    Location loc_;
    //! The operation.
    UnaryOperator op_;
    //! The right-hand-side.
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
struct TermBinary {
    //! Construct a term for an binary operation.
    explicit TermBinary(Location loc, Term lhs, BinaryOperator op, Term rhs);
    //! Construct a term for an binary operation.
    explicit TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                        Util::immutable_value<Term> rhs);

    //! The location of the function.
    Location loc_;
    //! The left-hand-side.
    Util::immutable_value<Term> lhs_;
    //! The right-hand-side.
    Util::immutable_value<Term> rhs_;
    //! The operation.
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

inline TermAbs::TermAbs(Location loc, TermVec pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {}
inline TermUnary::TermUnary(Location loc, UnaryOperator op, Term rhs)
    : loc_{std::move(loc)}, op_{op}, rhs_{Util::make_immutable<Term>(std::move(rhs))} {}
inline TermUnary::TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, op_{op}, rhs_{std::move(rhs)} {}
inline TermBinary::TermBinary(Location loc, Term lhs, BinaryOperator op, Term rhs)
    : loc_{std::move(loc)}, lhs_{Util::make_immutable<Term>(std::move(lhs))},
      rhs_{Util::make_immutable<Term>(std::move(rhs))}, op_{op} {}
inline TermBinary::TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                              Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {}
inline TermFunction::TermFunction(Location loc, String name, PoolVec args, bool external)
    : loc_{std::move(loc)}, name_(std::move(name)), pool_{std::move(args)}, external_{external} {}
inline TermTuple::TermTuple(Location loc, ElementVec args) : loc_{std::move(loc)}, pool_{std::move(args)} {}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::TermVariable);
GRINGO_HASH_PROTO(Gringo::Input::TermSymbol);
GRINGO_HASH_PROTO(Gringo::Input::TermFunction);
GRINGO_HASH_PROTO(Gringo::Input::TermTuple);
GRINGO_HASH_PROTO(Gringo::Input::TermAbs);
GRINGO_HASH_PROTO(Gringo::Input::TermUnary);
GRINGO_HASH_PROTO(Gringo::Input::TermBinary);
GRINGO_HASH_PROTO(Gringo::Input::Projection);

#endif
