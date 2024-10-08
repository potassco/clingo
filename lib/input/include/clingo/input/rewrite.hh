#pragma once

#include <clingo/input/rewrite/rewrite_context.hh>

namespace Clingo::Input {

class TheoryAtomParser;

//! @addtogroup input_rewrite
//! @{

//! Rewrite the given statement.
//!
//! This functions pulls together all the different rewriting steps.
//! There are some optional rewriting steps that can be configured via options.
void rewrite(RewriteContext &ctx, Stm const &stm, StmVec &stms);

//! @}

} // namespace Clingo::Input
