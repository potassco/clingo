#pragma once

#include <gringo/input/statement.hh>

#include <gringo/logger.hh>

namespace Gringo::Input {

//! @defgroup input_check Check
//! Additional syntax checks.
//!
//! @ingroup input_algo
//!
//! @{

//! Check a term.
auto check_term(Logger &log, Term const &term) -> bool;
//! Check a literal.
auto check_literal(Logger &log, Lit const &lit) -> bool;
//! Check a head literal.
auto check_head_literal(Logger &log, HdLit const &lit) -> bool;
//! Check a body literal.
auto check_body_literal(Logger &log, BdLit const &lit) -> bool;
//! Check a statement.
auto check_statement(Logger &log, Stm const &stm) -> bool;

//! @}

} // namespace Gringo::Input
