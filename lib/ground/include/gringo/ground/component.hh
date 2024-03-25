#pragma once

#include <gringo/ground/statement.hh>

namespace Gringo::Ground {

struct Component {
    // TODO: eta-rules
    // TODO: epsilon-rules (might be merged with the above)
    // TODO: alpha-rules
    // TODO: info about aggregates
    Ground::UStmVec stms;
};

} // namespace Gringo::Ground
