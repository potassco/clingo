#pragma once

#include <clingo/input/program.hh>

#include <clingo/input/rewrite/rewrite_context.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Parse theory atoms in the given statement with the given parser.
auto rewrite_theory(RewriteContext &ctx, Stm const &stm) -> std::optional<Stm>;

//! @}

} // namespace CppClingo::Input
