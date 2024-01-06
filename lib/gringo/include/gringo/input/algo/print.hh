#pragma once

#include <gringo/input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_print Print
//! Functions to output expressions.
//!
//! @ingroup input_algo
//!
//! @{

//! Output the given unary operator.
auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream &;

//! Output the given binary operator.
auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream &;

//! Output the position to the given stream.
auto operator<<(std::ostream &out, Position const &pos) -> std::ostream &;

//! Output the location to the given stream.
auto operator<<(std::ostream &out, Location const &loc) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, Term const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryElement const &term) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, Literal const &lit) -> std::ostream &;

//! Output the head aggregate element to the given stream.
auto operator<<(std::ostream &out, HeadAggregate::Element const &elem) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HeadLiteral const &lit) -> std::ostream &;

//! Output the head aggregate element to the given stream.
auto operator<<(std::ostream &out, BodyAggregate::Element const &elem) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BodyLiteral const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream &;

//! Convert the given term into a string.
auto to_string(Term const &term) -> std::string;

//! Convert the given term into a string.
auto to_string(TheoryTerm const &term) -> std::string;

//! Convert the given literal into a string.
auto to_string(Literal const &lit) -> std::string;

//! Convert the given head literal into a string.
auto to_string(HeadLiteral const &lit) -> std::string;

//! Convert the given head literal into a string.
auto to_string(BodyLiteral const &lit) -> std::string;

//! Convert the given head literal into a string.
auto to_string(Statement const &stm) -> std::string;

//! @}

} // namespace Gringo::Input
