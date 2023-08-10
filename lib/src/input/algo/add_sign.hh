#pragma once

#include <input/body_literal.hh>

namespace Gringo::Input {

//! Add a sign to the literal.
//!
//! Note that this function has to be used with care because the library uses shared pointers to literals.
//! This function is currently only used during construction in the parser.
void add_sign(Literal &lit, Sign sign, std::optional<Position> pos = std::nullopt);

//! Add a sign to the body literal.
void add_sign(BodyLiteral &lit, Sign sign, std::optional<Position> pos = std::nullopt);

} // namespace Gringo::Input
