#pragma once

#include <input/program.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Rewrite the given statement.
//!
//! This functions pulls together all the different rewriting steps.
//! The highest rewrite level is used to rewrite a program before grounding.
//! There are some optional rewriting steps that can be configured via the given options.
void rewrite(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, Statement const &stm,
             RewriteOptions opts, StatementVec &stms);

//! @}

} // namespace Gringo::Input
