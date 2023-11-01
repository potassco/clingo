#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

// TODO: a map might also be an idea here to avoid duplicates for the same variable
using AuxTermVec = std::vector<std::pair<Term, Term>>;

//! Simplifies the given term.
//!
//! The type monostate indicates an error, nullopt that the term did not change, and Symbol/Term correspond to the
//! simplified term.
[[nodiscard]] auto simplify(SymbolStore &store, NameGen &gen, AuxTermVec &aux, Term const &term)
    -> std::variant<std::monostate, std::nullopt_t, Symbol, Term>;

} // namespace Gringo::Input
