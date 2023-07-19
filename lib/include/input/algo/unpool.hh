#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup algo Algorithms
//! @ingroup language
//!
//! Algorithms for the input language
//!
//! @{

//! @name Functions to unpool expressions
//! @{

//! Remove all pooled arguments from the term.
[[nodiscard]] auto unpool(Term const &term) -> std::optional<TermVec>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(Literal const &lit) -> std::optional<LiteralVec>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(HeadLiteral const &lit) -> std::optional<HeadLiteralVec>;

//! Remove all pooled arguments from the literal.
[[nodiscard]] auto unpool(BodyLiteral const &lit) -> std::optional<BodyLiteralVec>;

//! Remove all pooled arguments from the statement.
[[nodiscard]] auto unpool(Statement const &stm) -> std::optional<StatementVec>;

//! @}

//! @}

} // namespace Gringo::Input
