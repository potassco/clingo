#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto project(Term const &term, Projection project) -> std::optional<Term>;

}
