#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto unpool(Term const &term) -> std::optional<TermVec>;

}
