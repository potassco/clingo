#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

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
    //! Wheather the term is signed.
    bool has_sign = false;
    //! The number represented by the term.
    int pos_number = 0;
    //! The identifier represented by the term.
    std::string identifier;
};

//! Query information about the structure of the given term.
auto check_type(Term const &term, TermCheckType type, CheckTypeResult *res = nullptr) -> bool;

//! Check if the literal is a symbolic atom.
auto is_atom(Literal const &lit) -> bool;

//! Check if the literal is a test.
auto is_test(Literal const &lit) -> bool;

} // namespace Gringo::Input
