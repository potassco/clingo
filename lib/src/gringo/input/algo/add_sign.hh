#pragma once

#include <gringo/input/body_literal.hh>

namespace Gringo::Input {

//! Add a sign to the literal.
//!
//! Note that this function has to be used with care because the library uses shared pointers to literals.
//! This function is currently only used during construction in the parser.
[[nodiscard]] auto add_sign(Literal const &lit, Sign sign, std::optional<Position> pos = std::nullopt)
    -> std::optional<Literal>;

//! Add a sign to the body literal.
[[nodiscard]] auto add_sign(BodyLiteral const &lit, Sign sign, std::optional<Position> pos = std::nullopt)
    -> std::optional<BodyLiteral>;

} // namespace Gringo::Input
