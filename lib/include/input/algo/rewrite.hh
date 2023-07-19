#pragma once

#include <input/algo/project.hh>

namespace Gringo::Input {

//! @addtogroup algo
//! @{

//! Enumeration of available rewrite levels.
enum class RewriteLevel {
    disabled = 0,
    rewrite_anonymous = 1,
    unpool = 2,
    project = 3,
};

//! Options to configure rewriting.
struct RewriteOptions {
    RewriteLevel level = RewriteLevel::project;
    ProjectionMode project_mode = ProjectionMode::pure;
    bool project_anonymous = false;
};

//! @name Functions to rewrite statements
//! @{

//! Rewrite the given statement.
//!
//! This functions pulls together all the different rewriting steps.
//! The highest rewrite level is used to rewrite a program before grounding.
//! There are some optional rewriting steps that can be configured via the given options.
void rewrite(Statement const &stm, RewriteOptions opts, StatementVec &stms);

//! @}

//! @}

} // namespace Gringo::Input
