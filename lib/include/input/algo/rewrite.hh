#pragma once

#include <input/algo/project.hh>

namespace Gringo::Input {

//! @defgroup input_rewrite Rewrite
//! @ingroup input_algo
//!
//! Functions to rewrite statements
//!
//! @{

//! Enumeration of available rewrite levels.
enum class RewriteLevel {
    disabled = 0,          //!< Disable rewriting.
    rewrite_anonymous = 1, //!< Give names to anonymous variables.
    unpool = 2,            //!< Remove argument pools.
    project = 3,           //!< Project variables.
};

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
void rewrite(SymbolStore &store, Statement const &stm, RewriteOptions opts, StatementVec &stms);

//! @}

} // namespace Gringo::Input
