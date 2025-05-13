#pragma once

#include <clingo/input/rewrite.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Remove all pooled arguments from the term.
[[nodiscard]] auto unpool(RewriteContext &ctx, Term const &term) -> std::optional<std::vector<Term>>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(RewriteContext &ctx, Lit const &lit) -> std::optional<std::vector<Lit>>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(RewriteContext &ctx, HdLit const &lit) -> std::optional<std::vector<HdLit>>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(RewriteContext &ctx, BdLit const &lit) -> std::optional<std::vector<BdLit>>;

//! Remove all pooled arguments from the statement.
[[nodiscard]] auto unpool(RewriteContext &ctx, Stm const &stm) -> std::optional<StmVec>;

//! @}

} // namespace CppClingo::Input
