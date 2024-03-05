#pragma once

#include <gringo/input/program.hh>

#include <gringo/input/algo/rewrite_context.hh>

#include <gringo/util/optional.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Compute bounds from comparisons and intervals.
[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Stm const &stm) -> Util::ResultState<Stm>;

//! @}

} // namespace Gringo::Input
