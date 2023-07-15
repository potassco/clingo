#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto project_anonymous(TermV2 const &term) -> std::optional<TermV2>;

}
