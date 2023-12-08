#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_algo Algorithms
//! @ingroup input
//!
//! Algorithms for the input language.

//! @defgroup input_analyze Analyze
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

//! Check if the given term is a linear term.
//!
//! Returns true if the term has form m*X+n where m is a non-zero number, X a
//! variable, and n a number.
[[nodiscard]] auto is_linear(Term const &term) -> bool;

//! See is_linear().
[[nodiscard]] auto is_linear(TermBinary const &term) -> bool;

//! Returns true if the term has form t..u.
[[nodiscard]] auto is_interval(Term const &term) -> bool;

//! See is_interval().
[[nodiscard]] auto is_interval(TermBinary const &term) -> bool;

//! Check if the term always evaluates to a number.
//!
//! For examble, X+Y but not X because it can also evaluate to other symbols.
[[nodiscard]] auto always_numeric(Term const &term) -> bool;

//! Check if the term never evaluates to a number.
//!
//! This is true for tuples, function, \#sup, \#inf.
[[nodiscard]] auto never_numeric(Term const &term) -> bool;

//! Check if the term is constant.
[[nodiscard]] inline auto is_symbol(Term const &term) -> bool { return std::holds_alternative<TermSymbol>(term); }

//! Check if the term is a variable.
[[nodiscard]] inline auto is_variable(Term const &term) -> bool { return std::holds_alternative<TermVariable>(term); }

//! Get the truth value of a literal, in case it is a Boolean constant.
[[nodiscard]] inline auto is_fixed(Literal const &lit) -> std::optional<bool> {
    if (auto const *blit = std::get_if<LiteralBoolean>(&lit); blit != nullptr) {
        return blit->value == (blit->sign != Sign::once);
    }
    return std::nullopt;
}

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

//! Check if a rule is a fact.
//!
//! Returns the symbol representing the fact.
auto is_fact(SymbolStore &store, Rule const &rule) -> std::optional<Symbol>;

//! Check if a statement is a fact.
//!
//! Returns the symbol representing the fact.
auto is_fact(SymbolStore &store, Statement const &stm) -> std::optional<Symbol>;

//! @}

} // namespace Gringo::Input
