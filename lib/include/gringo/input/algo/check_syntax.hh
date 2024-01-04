#pragma once

#include <gringo/logger.hh>

#include <gringo/input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_check Check
//! @ingroup input_algo
//!
//! Additional syntax checks.
//!
//! @{

//! Check a term.
auto check_term(Logger &log, Term const &term) -> bool;
//! Check a literal.
auto check_literal(Logger &log, Literal const &lit) -> bool;
//! Check a head literal.
auto check_head_literal(Logger &log, HeadLiteral const &lit) -> bool;
//! Check a body literal.
auto check_body_literal(Logger &log, BodyLiteral const &lit) -> bool;
//! Check a statement.
auto check_statement(Logger &log, Statement const &stm) -> bool;

//! @}

} // namespace Gringo::Input
