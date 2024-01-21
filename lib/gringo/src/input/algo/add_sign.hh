#pragma once

#include <gringo/input/body_literal.hh>

namespace Gringo::Input {

//! Add a sign to the literal.
[[nodiscard]] auto add_sign(Literal const &lit, Sign sign, std::optional<Position> pos = std::nullopt)
    -> std::optional<Literal>;

//! Add a sign to the body literal.
[[nodiscard]] auto add_sign(BodyLiteral const &lit, Sign sign, std::optional<Position> pos = std::nullopt)
    -> std::optional<BodyLiteral>;

} // namespace Gringo::Input
