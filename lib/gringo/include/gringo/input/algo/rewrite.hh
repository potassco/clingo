#pragma once

#include <gringo/input/program.hh>

namespace Gringo::Input {

class TheoryAtomParser;

//! @defgroup input_rewrite Rewrite
//! Functions to rewrite expressions.
//!
//! @ingroup input_algo
//!
//! @{

//! Rewrite the given statement.
//!
//! This functions pulls together all the different rewriting steps.
//! There are some optional rewriting steps that can be configured via options.
void rewrite(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, TheoryAtomParser const &parser,
             Stm const &stm, RewriteOptions opts, StmVec &stms);

//! @}

} // namespace Gringo::Input
