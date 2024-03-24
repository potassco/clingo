#pragma once

#include <gringo/ground/statement.hh>

namespace Gringo::Ground {

struct Component {
    // TODO: accumulation rules
    // TODO: info about aggregates
    Ground::UStmVec stms;
};

} // namespace Gringo::Ground
