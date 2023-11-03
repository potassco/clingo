#pragma once

#include <logger.hh>

#include <util/enum.hh>

#include <input/statement.hh>

namespace Gringo::Input {

//! Flags controlling simplification.
enum class SimplifyFlags : unsigned {
    none = 0,              //! No flags are set.
    matchable = 1,         //! Ensure that expressions are matchable.
    projectable = 2,       //! Permit projection.
    unfailable = 4,        //! Ensure that there are no expressions that evaluate to empty pools.
    disjunctive = 8,       //! Indicate that the expression occurs in a disjunctive context.
    nested_matchable = 16, //! Do not make roots of terms matchable.
};

GRINGO_ENUM_FLAGS(SimplifyFlags)

// TODO: a map might also be an idea here to avoid duplicates for the same variable
//! A vector of term pairs where the second has been substituted by the first in some other term.
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
[[nodiscard]] auto simplify(SimplifyFlags flags, Logger &log, SymbolStore &store, NameGen &gen, AuxTermVec &aux,
                            Term const &term) -> std::variant<std::monostate, std::nullopt_t, Symbol, Term>;

[[nodiscard]] auto simplify(SimplifyFlags flags, Logger &log, SymbolStore &store, NameGen &gen, AuxTermVec &aux,
                            Literal const &lit) -> std::optional<Literal>;

} // namespace Gringo::Input
