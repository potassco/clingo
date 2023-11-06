#pragma once

#include <logger.hh>

#include <util/enum.hh>

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_simplify Simplify Arithmetic
//! @ingroup input_algo
//!
//! Functions to simplify arithmetic terms in expressions.
//!
//! @{

//! Flags controlling simplification.
enum class SimplifyFlags : unsigned {
    none = 0,              //!< No flags are set.
    matchable = 1,         //!< Ensure that expressions are matchable.
    projectable = 2,       //!< Permit projection.
    unfailable = 4,        //!< Ensure that there are no expressions that evaluate to empty pools.
    disjunctive = 8,       //!< Indicate that the expression occurs in a disjunctive context.
    nested_matchable = 16, //!< Do not make roots of terms matchable.
};

GRINGO_ENUM_FLAGS(SimplifyFlags)

// TODO: a map might also be an idea here to avoid duplicates for the same variable
//! A vector of term pairs where the second has been substituted by the first in some other term.
using AuxTermVec = std::vector<std::pair<Term, Term>>;

//! Helper to pass arguments to simplify.
struct SimplifyContext {
    Logger &log;        //!< Logger to report messages.
    SymbolStore &store; //!< Symbol store to create fresh symbols.
    NameGen &gen;       //!< Genartor to create fresh variable names.
    AuxTermVec &aux;    //!< Vector of variable term pairs.
};

//! Indicate that simplifying an expression failed due to an invalid operation or error.
//!
//! If there has been an error, this will be indicated by the logger.
struct SimplifyFail {};
//! Indicate that an expression did not change during simplification.
struct SimplifyUnchanged {};
//! Indicate a simplifaction.
//!
//! The variant either captures that an expression failed to simplify,
//! did not change, or the changed expression.
template <class Expr> using SimplifyResult = std::variant<SimplifyFail, SimplifyUnchanged, Expr>;

//! Check if the given term is a linear term.
//!
//! Returns true if the term has form m*X+n where m is a non-zero number, X a
//! variable, and n a number.
[[nodiscard]] auto is_linear(Term const &term) -> bool;

//! See is_linear().
[[nodiscard]] auto is_linear(TermBinary const &term) -> bool;

//! Simplifies the given term.
//!
//! The type monostate indicates an error, nullopt that the term did not change, and Symbol/Term correspond to the
//! simplified term.
[[nodiscard]] auto simplify(SimplifyFlags flags, SimplifyContext ctx, Term const &term)
    -> std::variant<std::monostate, std::nullopt_t, Symbol, Term>;

//! Simplifies the given literal.
[[nodiscard]] auto simplify(SimplifyFlags flags, SimplifyContext ctx, Literal const &lit) -> SimplifyResult<Literal>;

//! @}

} // namespace Gringo::Input
