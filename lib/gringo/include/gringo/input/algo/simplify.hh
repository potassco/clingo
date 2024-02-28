#pragma once

#include <gringo/input/program.hh>

#include <gringo/util/enum.hh>
#include <gringo/util/optional.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Flags controlling simplification of terms.
enum class SimplifyTermFlags : unsigned {
    none = 0,       //!< No flags are set.
    matchable = 1,  //!< Ensure that terms are matchable.
    unfailable = 2, //!< Ensure that terms do not evaluate to empty pools (should be used together with matchable).
    nested_matchable = 4,  //!< Do not make roots of terms matchable (should be used together with matchable).
    preserve_toplevel = 8, //!< Preserve toplevel terms terms like u..t or \@f.
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

using Util::TruthValue;

//! The result of a simplification.
template <class E> using SimplifyResult = Util::ResultState<E, TruthValue>;

//! The result of a simplification.
using SimplifyTermResult = Util::ResultState<Term, bool>;

//! Simplifies the given term.
//!
//! The truth value of the result is false if the term failed to simplify,
//! i.e., it evaluated to an empty pool.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
//!
//! All but flags but the head flag apply to terms.
[[nodiscard]] auto simplify(SimplifyTermFlags flags, RewriteContext &ctx, Term const &term) -> SimplifyTermResult;

//! Simplifies the given literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
//!
//! Only the matchable and head flags apply to literals.
//! The remaining ones are simply cleared.
[[nodiscard]] auto simplify(SimplifyLiteralFlags flags, RewriteContext &ctx, Lit const &lit) -> SimplifyResult<Lit>;

//! Simplifies the given head literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(RewriteContext &ctx, HdLit const &lit) -> SimplifyResult<HdLit>;

//! Simplifies the given body literal.
//!
//! The result consists of a truth value and an optional literal in case of change.
//! The literal is simplified to \#true/\#false for truth values true/false.
//!
//! Terms that are replaced during simplification by auxiliary variables are added to the given context.
[[nodiscard]] auto simplify(RewriteContext &ctx, BdLit const &lit) -> SimplifyResult<BdLit>;

//! Simplifies the given statement.
//!
//! The result consists of a truth value and an optional statement in case of change.
//! The statement is simplified to \#true/\#false for truth values true/false.
[[nodiscard]] auto simplify(RewriteContext &ctx, Stm const &stm) -> SimplifyResult<Stm>;

//! @}

} // namespace Gringo::Input
