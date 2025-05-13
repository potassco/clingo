#pragma once

#include <clingo/input/program.hh>

#include <clingo/input/rewrite/rewrite_context.hh>

#include <clingo/util/optional.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Compute bounds from comparisons and intervals.
[[nodiscard]] auto compute_bounds(RewriteContext &ctx, Stm const &stm) -> Util::ResultState<Stm>;

//! @}

} // namespace CppClingo::Input
