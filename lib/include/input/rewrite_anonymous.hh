#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<STerm>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Literal const &lit, NameGen &gen) -> std::optional<SLiteral>;

} // namespace Gringo::Input
