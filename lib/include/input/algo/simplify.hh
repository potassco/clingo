#pragma once

#include <util/enum.hh>

#include <input/algo/rewrite_base.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Flags controlling simplification.
//!
//! @todo: Since only a small number of flags applies to literals,
//! the flags could be split.
enum class SimplifyTermFlags : unsigned {
    none = 0,       //!< No flags are set.
    matchable = 1,  //!< Ensure that terms are matchable.
    unfailable = 2, //!< Ensure that terms do not evaluate to empty pools (should be used together with matchable).
    nested_matchable = 4,  //!< Do not make roots of terms matchable (should be used together with matchable).
    preserve_toplevel = 8, //!< Preserve toplevel terms terms like u..t or @f.
};

GRINGO_ENUM_FLAGS(SimplifyTermFlags)

//! Flags controlling simplification of literals.
enum class SimplifyLiteralFlags : unsigned {
    none = 0,       //!< No flags are set.
    matchable = 1,  //!< Ensure that literals are matchable.
    unfailable = 2, //!< Ensure that terms in literals do not evaluate to empty pools.
    head = 4,       //!< Indicate literals occurring in rule heads.
};

GRINGO_ENUM_FLAGS(SimplifyLiteralFlags)

//! Truth values for expressions.
enum class TruthValue {
    top,     //!< Indicate a true expression.
    bot,     //!< Indicate a false expression.
    unknown, //!< Indicate an expression with an unknown truth value.
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

//! Simplifies the given term.
//!
//! The truth value of the result is false if the term failed to simplify,
//! i.e., it evaluated to an empty pool.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
//!
//! All but flags but the head flag apply to terms.
[[nodiscard]] auto simplify(SimplifyTermFlags flags, RewriteContext &ctx, Term const &term)
    -> SimplifyResult<Term, bool>;

//! Simplifies the given literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
//!
//! Only the matchable and head flags apply to literals.
//! The remaining ones are simply cleared.
[[nodiscard]] auto simplify(SimplifyLiteralFlags flags, RewriteContext &ctx, Literal const &lit)
    -> SimplifyResult<Literal>;

//! Simplifies the given head literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(RewriteContext &ctx, HeadLiteral const &lit) -> SimplifyResult<HeadLiteral>;

//! Simplifies the given body literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(RewriteContext &ctx, BodyLiteral const &lit) -> SimplifyResult<BodyLiteral>;

//! Simplifies the given statement.
//!
//! The result consists of a truth value and an optional statement in case of change.
//! The statement is simplified to \#true/\#false for truth values true/false.
[[nodiscard]] auto simplify(RewriteContext &ctx, Statement const &stm) -> SimplifyResult<Statement>;

//! @}

} // namespace Gringo::Input
