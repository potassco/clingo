#pragma once

#include <gringo/util/optional.hh>

#include <gringo/input/program.hh>

namespace Gringo::Input {

//! Compute bounds from comparisons and intervals.
[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Statement const &stm) -> Util::ResultState<Statement>;

} // namespace Gringo::Input
