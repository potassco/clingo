#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Project positional anonymous variables in the term.
//!
//! This is a deprecated feature to support old programs.
//! The projection star should be used instead.
[[nodiscard]] auto project_anonymous(Term const &term) -> std::optional<Term>;

//! Project anonymous variables in negated symbolic literals.
//!
//! This is a deprecated feature to support old programs.
//! The projection star should be used instead.
[[nodiscard]] auto project_anonymous(Literal const &lit) -> std::optional<Literal>;

//! Project anonymous variables in (nested) negated symbolic literals.
//!
//! This is a deprecated feature to support old programs.
//! The projection star should be used instead.
[[nodiscard]] auto project_anonymous(HeadLiteral const &lit) -> std::optional<HeadLiteral>;

//! Project anonymous variables in (nested) negated symbolic literals.
//!
//! This is a deprecated feature to support old programs.
//! The projection star should be used instead.
[[nodiscard]] auto project_anonymous(BodyLiteral const &lit) -> std::optional<BodyLiteral>;

//! Project anonymous variables in (nested) negated symbolic literals.
//!
//! This is a deprecated feature to support old programs.
//! The projection star should be used instead.
[[nodiscard]] auto project_anonymous(Statement const &stm) -> std::optional<Statement>;

} // namespace Gringo::Input
