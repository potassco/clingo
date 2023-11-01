#pragma once

#include <util/enum.hh>

#include <input/statement.hh>

namespace Gringo::Input {

enum class SimplifyFlags : unsigned {
    none = 0,
    matchable = 1,
    projectable = 2,
    unfailable = 4,
};

GRINGO_ENUM_FLAGS(SimplifyFlags)

// TODO: a map might also be an idea here to avoid duplicates for the same variable
using AuxTermVec = std::vector<std::pair<Term, Term>>;

//! Check if the given term is a linear term.
//!
//! Returns true if the term has form m*X+n where m is a non-zero number, X a
//! variable, and n a number.
[[nodiscard]] auto is_linear(Term const &term) -> bool;

//! See is_linear().
[[nodiscard]] auto is_linear(TermBinary const &term) -> bool;

//! Simplifies the given term.
//!
//! The type monostate indicates an error, nullopt that the term did not change, and Symbol/Term correspond to the
//! simplified term.
[[nodiscard]] auto simplify(SimplifyFlags flags, SymbolStore &store, NameGen &gen, AuxTermVec &aux, Term const &term)
    -> std::variant<std::monostate, std::nullopt_t, Symbol, Term>;

} // namespace Gringo::Input
