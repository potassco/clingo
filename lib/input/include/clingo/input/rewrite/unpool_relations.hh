#pragma once

#include <clingo/input/rewrite.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Negate the given literal.
[[nodiscard]] auto negate(Lit const &lit) -> Lit;

//! Unpool non-binary relation literals.
[[nodiscard]] auto unpool_relations(Lit const &lit, bool conjunctive) -> std::optional<LitArray>;

//! Unpool all non-binary relation in the statement.
[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Stm const &stm) -> std::optional<StmVec>;

//! @}

} // namespace CppClingo::Input
