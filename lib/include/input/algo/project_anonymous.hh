#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto project_anonymous(Term const &term) -> std::optional<Term>;

[[nodiscard]] auto project_anonymous(Literal const &lit) -> std::optional<Term>;

} // namespace Gringo::Input
