#pragma once

#include <input/program.hh>

namespace Gringo::Input {

//! @defgroup input_rewrite Rewrite
//! @ingroup input_algo
//!
//! Functions to rewrite expressions.
//!
//! @{

//! Rewrite the given statement.
//!
//! This functions pulls together all the different rewriting steps.
//! There are some optional rewriting steps that can be configured via options.
void rewrite(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, Statement const &stm,
             RewriteOptions opts, StatementVec &stms);

//! @}

} // namespace Gringo::Input
