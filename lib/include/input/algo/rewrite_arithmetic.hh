#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Simplifies the given term.
//!
//! The type monostate indicates an error, nullopt that the term did not change, and Symbol/Term correspond to the
//! simplified term.
[[nodiscard]] auto simplify(SymbolStore &store, NameGen &gen, Term const &term)
    -> std::variant<std::monostate, std::nullopt_t, Symbol, Term>;

} // namespace Gringo::Input
