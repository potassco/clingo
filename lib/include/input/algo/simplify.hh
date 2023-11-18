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
    head = 8,              //!< Indicate that the expression occurs in a rule head.
    nested_matchable = 16, //!< Do not make roots of terms matchable.
};

GRINGO_ENUM_FLAGS(SimplifyFlags)

//! Truth values for expressions.
enum class TruthValue {
    top,     //! Indicate a true expression.
    bot,     //! Indicate a false expression.
    unknown, //! Indicate an expression with an unknown truth value.
};

//! The result of a simplification.
//!
//! The result consists of a state resulting from simplification
//! together with an optional value in case the expression was changed.
template <class T, class S = TruthValue> struct SimplifyResult {
    //! A truth value or state.
    S state = S{};
    //! An optional rewritten expression.
    std::optional<T> value = std::nullopt;
};

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

//! Check if the given term is a linear term.
//!
//! Returns true if the term has form m*X+n where m is a non-zero number, X a
//! variable, and n a number.
[[nodiscard]] auto is_linear(Term const &term) -> bool;

//! See is_linear().
[[nodiscard]] auto is_linear(TermBinary const &term) -> bool;

//! Check if the term always evaluates to a number.
//!
//! For examble, X+Y but not X because it can also evaluate to other symbols.
[[nodiscard]] auto is_numeric(Term const &term) -> bool;

//! Check if the term is constant.
[[nodiscard]] inline auto is_symbol(Term const &term) -> bool { return std::holds_alternative<TermSymbol>(term); }

//! Get the truth value of a literal, in case it is a Boolean constant.
[[nodiscard]] inline auto is_fixed(Literal const &lit) -> std::optional<bool> {
    if (auto const *blit = std::get_if<LiteralBoolean>(&lit); blit != nullptr) {
        return blit->value == (blit->sign != Sign::once);
    }
    return std::nullopt;
}

//! Simplifies the given term.
//!
//! The truth value of the result is false if the term failed to simplify,
//! i.e., it evaluated to an empty pool.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(SimplifyFlags flags, SimplifyContext ctx, Term const &term) -> SimplifyResult<Term, bool>;

//! Simplifies the given literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(SimplifyFlags flags, SimplifyContext ctx, Literal const &lit) -> SimplifyResult<Literal>;

//! Simplifies the given head literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(SimplifyContext ctx, HeadLiteral const &lit) -> SimplifyResult<HeadLiteral>;

//! Simplifies the given body literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(SimplifyContext ctx, BodyLiteral const &lit) -> SimplifyResult<BodyLiteral>;

//! Simplifies the given statement.
//!
//! The result consists of a truth value and an optional statement in case of change.
//! The statement is simplified to \#true/\#false for truth values true/false.
[[nodiscard]] auto simplify(Logger &log, SymbolStore &store, Statement const &stm) -> SimplifyResult<Statement>;

//! @}

} // namespace Gringo::Input
