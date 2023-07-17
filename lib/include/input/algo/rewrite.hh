#pragma once

#include <input/algo/project.hh>

namespace Gringo::Input {

enum class RewriteLevel {
    disabled = 0,
    rewrite_anonymous = 1,
    unpool = 2,
    project = 3,
};

struct RewriteOptions {
    RewriteLevel level = RewriteLevel::project;
    ProjectionMode project_mode = ProjectionMode::pure;
    bool project_anonymous = false;
};

void rewrite(Statement const &stm, RewriteOptions opts, StatementVec &stms);

} // namespace Gringo::Input
