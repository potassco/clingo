#pragma once

#include <input/algo/rewrite.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Unpool non-binary relation literals.
[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Literal const &lit, bool head) -> std::optional<LiteralVecVec>;

//! Unpool all non-binary relation in the literal.
[[nodiscard]] auto unpool_relations(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteralVec>;

//! Unpool all non-binary relation in the literal.
[[nodiscard]] auto unpool_relations(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteralVec>;

//! Unpool all non-binary relation in the statement.
[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec>;

//! @}

} // namespace Gringo::Input
