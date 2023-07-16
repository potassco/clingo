#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

[[nodiscard]] auto unpool(Term const &term) -> std::optional<TermVec>;

[[nodiscard]] auto unpool(Literal const &lit) -> std::optional<LiteralVec>;

} // namespace Gringo::Input
