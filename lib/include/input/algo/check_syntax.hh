#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_check Check
//! @ingroup input_algo
//!
//! Additional syntax checks.
//!
//! @{

//! Check a term.
auto check_term(Term const &term) -> bool;
//! Check a literal.
auto check_literal(Literal const &lit) -> bool;
//! Check a head literal.
auto check_head_literal(HeadLiteral const &lit) -> bool;
//! Check a body literal.
auto check_body_literal(BodyLiteral const &lit) -> bool;
//! Check a statement.
auto check_statement(Statement const &stm) -> bool;

//! @}

} // namespace Gringo::Input
