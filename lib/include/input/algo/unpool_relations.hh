#pragma once

#include <input/algo/rewrite.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Unpool non-binary relation literals.
[[nodiscard]] auto unpool_relations(Literal const &lit, bool conjunctive) -> std::optional<LiteralVec>;

//! Unpool all non-binary relation in the statement.
[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec>;

//! @}

} // namespace Gringo::Input
