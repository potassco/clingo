#pragma once

#include <clingo/input/statement.hh>

#include <clingo/core/logger.hh>

namespace CppClingo::Input {

//! @addtogroup input_check
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

} // namespace CppClingo::Input
