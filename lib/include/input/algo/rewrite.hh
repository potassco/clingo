#pragma once

#include <input/algo/rewrite_base.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Options to configure rewriting.
struct RewriteOptions {
    //! The rewrite level.
    RewriteLevel level = RewriteLevel::project;
    //! The projection mode.
    ProjectionMode project_mode = ProjectionMode::pure;
    //! Whether to project anonymous variables in negative literals.
    bool project_anonymous = false;
};

//! Rewrite the given statement.
//!
//! This functions pulls together all the different rewriting steps.
//! The highest rewrite level is used to rewrite a program before grounding.
//! There are some optional rewriting steps that can be configured via the given options.
void rewrite(Logger &log, SymbolStore &store, Statement const &stm, RewriteOptions opts, StatementVec &stms);

//! @}

} // namespace Gringo::Input
