#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Term const &term, Projection project) -> std::optional<Term>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Literal const &lit, Projection project) -> std::optional<Literal>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(HeadLiteral const &lit, Projection project) -> std::optional<HeadLiteral>;

//! Project variables according to given projection mode and scope.
//!
//! Some literal occurrences cannot be projected preserving equivalence.
//! For example, variables in nonmonotone aggregates are only projected in classical scope.
[[nodiscard]] auto project(BodyLiteral const &lit, Projection project, bool in_classical_scope)
    -> std::optional<BodyLiteral>;

} // namespace Gringo::Input
