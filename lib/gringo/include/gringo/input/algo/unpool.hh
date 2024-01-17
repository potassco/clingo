#pragma once

#include <gringo/input/algo/rewrite.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Remove all pooled arguments from the term.
[[nodiscard]] auto unpool(RewriteContext &ctx, Term const &term) -> std::optional<TermVec>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(RewriteContext &ctx, Literal const &lit) -> std::optional<LiteralVec>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<std::vector<HeadLiteral>>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<std::vector<BodyLiteral>>;

//! Remove all pooled arguments from the statement.
[[nodiscard]] auto unpool(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec>;

//! @}

} // namespace Gringo::Input
