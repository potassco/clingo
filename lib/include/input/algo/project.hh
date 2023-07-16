#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto project(Term const &term, Projection project) -> std::optional<Term>;

[[nodiscard]] auto project(Literal const &term, Projection project) -> std::optional<Literal>;

} // namespace Gringo::Input
