#pragma once

#include <gringo/input/body_literal.hh>

namespace Gringo::Input {

//! Add a sign to the literal.
[[nodiscard]] auto add_sign(Lit const &lit, Sign sign, std::optional<Position> pos = std::nullopt)
    -> std::optional<Lit>;

//! Add a sign to the body literal.
[[nodiscard]] auto add_sign(BdLit const &lit, Sign sign, std::optional<Position> pos = std::nullopt)
    -> std::optional<BdLit>;

} // namespace Gringo::Input
