#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto project(TermV2 const &term, Projection project) -> std::optional<TermV2>;

}
