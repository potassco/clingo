#pragma once

#include <input/program.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Replace all parameters in the given fact.
[[nodiscard]] auto substitute(RewriteContext &ctx, Location const &loc, Symbol const &sym)
    -> std::variant<Symbol, Statement>;

//! Replace all parameters in the given term.
[[nodiscard]] auto substitute(RewriteContext &ctx, Term const &term) -> std::optional<Term>;

//! Replace all parameters in the given literal.
[[nodiscard]] auto substitute(RewriteContext &ctx, Literal const &lit) -> std::optional<Literal>;

//! Replace all parameters in the given head literal.
[[nodiscard]] auto substitute(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteral>;

//! Replace all parameters in the given body literal.
[[nodiscard]] auto substitute(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteral>;

//! Replace all parameters in the given statement.
[[nodiscard]] auto substitute(RewriteContext &ctx, Statement const &stm) -> std::optional<Statement>;

//! @}

} // namespace Gringo::Input
