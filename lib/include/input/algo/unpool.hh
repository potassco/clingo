#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto unpool(TermV2 const &term) -> std::optional<TermVec>;

}
