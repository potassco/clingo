#pragma once

#include <gringo/util/optional.hh>

#include <gringo/input/program.hh>

namespace Gringo::Input {

//! Compute bounds from comparisons and intervals.
[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Stm const &stm) -> Util::ResultState<Stm>;

} // namespace Gringo::Input
