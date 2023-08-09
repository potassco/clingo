#pragma once

#include <unordered_set>
#include <variant>
#include <vector>

#include <util/hash.hh>
#include <util/shared_ptr.hh>

#include <symbol.hh>

#include <input/location.hh>

namespace Gringo::Input {

//! @defgroup input Input
//!
//! Data structures and functions to parse and rewrite the gringo language.

//! @defgroup input_language Language
//! @ingroup input
//!
//! Data structures and functions to capture the gringo language.

//! @defgroup input_term Terms
//! @ingroup input_language
//!
//! Data structures and functions to represent terms.
//!
//! @{

//! A set of variable names.
using VariableSet = std::unordered_set<std::string>;
//! A vector of variable names.
using VariableVec = std::vector<std::string>;

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
using TermVec = std::vector<Term>;
//! A vector of vectors of terms.
using TermVecVec = std::vector<TermVec>;

//! A variant capturing either a term or a position that is to be projected.
using TupleElem = std::variant<std::monostate, Term>;
//! A tuple of terms or positions to project.
using TupleVec = std::vector<TupleElem>;
//! A vector of tuples used as function or predicate arguments.
using PoolVec = std::vector<TupleVec>;

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
struct TermVariable {
    //! Construct a variable.
    explicit TermVariable(Location loc, std::string name, bool is_anonymous = false)
        : loc{std::move(loc)}, name{std::move(name)}, is_anonymous{is_anonymous} {}

    //! The location of the variable.
    Location loc;
    //! The name of the variable.
    std::string name;
    //! Whether the variable is anonymous.
    bool is_anonymous;
};

//! Compare two variables.
//!
//! @related TermVariable
auto operator==(TermVariable const &a, TermVariable const &b) -> bool;

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
struct TermSymbol {
    //! Construct term with the given symbol.
    explicit TermSymbol(Location loc, Symbol value) : loc{std::move(loc)}, value{std::move(value)} {}

    //! The location of the symbol.
    Location loc;
    //! The associated symbol.
    Symbol value;
};

//! Compare two symbols.
//!
//! @related TermSymbol
auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool;

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
struct TermTuple {
    //! A tuple element.
    using Element = std::variant<TupleVec, Term>;
    //! A vector of tuple elements.
    using ElementVec = std::vector<Element>;

    //! Construct a  tuple.
    explicit TermTuple(ElementVec args);

    //! The argument pool of the tuple.
    ElementVec pool;
};

//! Compare two tuple terms.
//!
//! @related TermTuple
auto operator==(TermTuple const &a, TermTuple const &b) -> bool;

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
struct TermFunction {
    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(std::string name, PoolVec args, bool external);

    //! The name of the function.
    std::string name;
    //! The argument pool of the function.
    PoolVec pool;
    //! Whether this is an external function.
    bool external;
};

//! Compare two function terms.
//!
//! @related TermFunction
auto operator==(TermFunction const &a, TermFunction const &b) -> bool;

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
struct TermAbs {
    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(TermVec pool);

    //! The argument pool of the absolute term.
    TermVec pool;
};

//! Compare two absolute terms.
//!
//! @related TermAbs
auto operator==(TermAbs const &a, TermAbs const &b) -> bool;

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
    explicit TermUnary(UnaryOperator op, Term rhs);
    //! Construct a term for an unary operation.
    explicit TermUnary(UnaryOperator op, Util::shared_ptr<Term> rhs);

    //! The operation.
    UnaryOperator op;
    //! The right-hand-side.
    Util::shared_ptr<Term> rhs;
};

//! Compare two unary terms.
//!
//! @related TermUnary
auto operator==(TermUnary const &a, TermUnary const &b) -> bool;

//! Enumeration of available binary operators.
enum class BinaryOperator : int {
    dots,  //!< The interval operator.
    xor_,  //!< The XOR bit operation.
    or_,   //!< The OR bit operation.
    and_,  //!< The AND bit operation.
    plus,  //!< The plus arithmetic operation.
    minus, //!< The minus arithmetic operation.
    times, //!< The multiply arithmetic operation.
    div,   //!< The (integer) divide arithmetic operation.
    mod,   //!< The modulo arithmetic operation.
    pow,   //!< The exponentiation arithmetic operation.
};

//! Term representing a binary operation.
//!
//! For example <tt>X-Y</tt>.
struct TermBinary {
    //! Construct a term for an binary operation.
    explicit TermBinary(Term lhs, BinaryOperator op, Term rhs);
    //! Construct a term for an binary operation.
    explicit TermBinary(Util::shared_ptr<Term> lhs, BinaryOperator op, Util::shared_ptr<Term> rhs);

    //! The operation.
    BinaryOperator op;
    //! The left-hand-side.
    Util::shared_ptr<Term> lhs;
    //! The right-hand-side.
    Util::shared_ptr<Term> rhs;
};

//! Compare two binary terms.
//!
//! @related TermBinary
auto operator==(TermBinary const &a, TermBinary const &b) -> bool;

//! @}

// Note that constructors are defined here because at this point all types are
// complete.

inline TermAbs::TermAbs(TermVec pool) : pool{std::move(pool)} {}
inline TermUnary::TermUnary(UnaryOperator op, Term rhs) : op{op}, rhs{Util::construct_shared<Term>(std::move(rhs))} {}
inline TermUnary::TermUnary(UnaryOperator op, Util::shared_ptr<Term> rhs) : op{op}, rhs{std::move(rhs)} {}
inline TermBinary::TermBinary(Term lhs, BinaryOperator op, Term rhs)
    : op{op}, lhs{Util::construct_shared<Term>(std::move(lhs))}, rhs{Util::construct_shared<Term>(std::move(rhs))} {}
inline TermBinary::TermBinary(Util::shared_ptr<Term> lhs, BinaryOperator op, Util::shared_ptr<Term> rhs)
    : op{op}, lhs{std::move(lhs)}, rhs{std::move(rhs)} {}
inline TermFunction::TermFunction(std::string name, PoolVec args, bool external)
    : name(std::move(name)), pool{std::move(args)}, external{external} {}
inline TermTuple::TermTuple(ElementVec args) : pool{std::move(args)} {}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::TermVariable);
GRINGO_HASH_PROTO(Gringo::Input::TermSymbol);
GRINGO_HASH_PROTO(Gringo::Input::TermFunction);
GRINGO_HASH_PROTO(Gringo::Input::TermTuple);
GRINGO_HASH_PROTO(Gringo::Input::TermAbs);
GRINGO_HASH_PROTO(Gringo::Input::TermUnary);
GRINGO_HASH_PROTO(Gringo::Input::TermBinary);

#endif
