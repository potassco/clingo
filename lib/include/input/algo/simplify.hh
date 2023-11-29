#pragma once

#include <util/enum.hh>

#include <input/algo/rewrite_base.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Flags controlling simplification.
//!
//! @todo: It might be possible to split this into flags for terms and literals.
enum class SimplifyFlags : unsigned {
    none = 0,                    //!< No flags are set.
    matchable = 1,               //!< Ensure that expressions are matchable.
    projectable = 2,             //!< Permit projection.
    unfailable = 4,              //!< Ensure that there are no expressions that evaluate to empty pools.
    head = 8,                    //!< Indicate that the expression occurs in a rule head.
    nested_matchable = 16,       //!< Do not make roots of terms matchable.
    preserve_toplevel_dots = 32, //!< Preserve the dots in terms like u..t.
};

GRINGO_ENUM_FLAGS(SimplifyFlags)

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
[[nodiscard]] auto simplify(SimplifyFlags flags, RewriteContext &ctx, Term const &term) -> SimplifyResult<Term, bool>;

//! Simplifies the given literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(SimplifyFlags flags, RewriteContext &ctx, Literal const &lit) -> SimplifyResult<Literal>;

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
