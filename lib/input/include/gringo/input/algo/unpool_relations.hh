#pragma once

#include <gringo/input/algo/rewrite.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Unpool non-binary relation literals.
[[nodiscard]] auto unpool_relations(Lit const &lit, bool conjunctive) -> std::optional<LitArray>;

//! Unpool all non-binary relation in the statement.
[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Stm const &stm) -> std::optional<StmVec>;

//! @}

} // namespace Gringo::Input
