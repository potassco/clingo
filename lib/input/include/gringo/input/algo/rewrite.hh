#pragma once

#include <gringo/input/algo/rewrite_context.hh>

namespace Gringo::Input {

class TheoryAtomParser;

//! @addtogroup input_rewrite
//! @{

//! Rewrite the given statement.
//!
//! This functions pulls together all the different rewriting steps.
//! There are some optional rewriting steps that can be configured via options.
void rewrite(RewriteContext &ctx, Stm const &stm, StmVec &stms);

//! @}

} // namespace Gringo::Input
