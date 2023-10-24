#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_algo Algorithms
//! @ingroup input
//!
//! Algorithms for the input language.

//! @defgroup input_check_type Check Type
//! @ingroup input_algo
//!
//! Functions to analyze expressions.
//!
//! @{

//! Enumeration for Term::check_type().
enum class TermCheckType : int {
    atom,              //!< Check if term is an atom.
    sig,               //!< Check if term is a signature.
    identifier,        //!< Check if term is an identifier.
    signed_identifier, //!< Check if term is a signed identifier.
    pos_number         //!< Check if term is a positive number.
};

//! Extract additional information while checking the type of a term.
//!
//! @see Term::check_type()
struct CheckTypeResult {
    //! Whether the term is signed.
    bool has_sign = false;
    //! The number represented by the term.
    NumberRef pos_number;
    //! The identifier represented by the term.
    String identifier;
};

//! Query information about the structure of the given term.
auto check_type(Term const &term, TermCheckType type, CheckTypeResult *res = nullptr) -> bool;

//! Check whether the literal is an atoms.
//!
//! A literal is an atom if it is a symbolic literal without a sign.
//! This corresponds for example to a conjunction with one element equal to a such an atom.
auto is_atom(Literal const &lit) -> bool;

//! Check if the literal is a symbolic atom.
auto is_atom(HeadLiteral const &lit) -> bool;

//! Check if the literal is a symbolic atom.
auto is_atom(BodyLiteral const &lit) -> bool;

//! Check whether the literal is a test.
//!
//! A test is a negated or not a symbolic literal.
//! This corresponds to conjunctions where the right-hand-side of elements is empty
//! and the left-hand-side is composed of tests.
auto is_test(Literal const &lit) -> bool;

//! Check if the literal is a test.
auto is_test(HeadLiteral const &lit) -> bool;

//! Check if the literal is a test.
auto is_test(BodyLiteral const &lit) -> bool;

//! Check if the literal is classical.
auto is_classical(HeadLiteral const &lit) -> bool;

//! @}

} // namespace Gringo::Input
