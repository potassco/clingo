#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Output the term to the given stream.
auto operator<<(std::ostream &out, Term const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, Literal const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HeadLiteral const &lit) -> std::ostream &;

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

} // namespace Gringo::Input
