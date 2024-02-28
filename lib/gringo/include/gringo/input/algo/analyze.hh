#pragma once

#include <gringo/input/statement.hh>

#include <gringo/logger.hh>

namespace Gringo::Input {

//! @defgroup input_algo Algorithms
//! Algorithms for the input language.
//!
//! @ingroup input

//! @defgroup input_analyze Analyze
//! Functions to analyze expressions.
//!
//! @ingroup input_algo
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
//! Returns "X" if the term has form m*X+n where m is a non-zero number, X a
//! variable, and n a number.
[[nodiscard]] auto is_linear(Term const &term) -> std::optional<String>;

//! See is_linear().
[[nodiscard]] auto is_linear(TermBinary const &term) -> std::optional<String>;

//! Returns true if the term has form t..u.
[[nodiscard]] auto is_interval(Term const &term) -> bool;

//! See is_interval().
[[nodiscard]] auto is_interval(TermBinary const &term) -> bool;

//! Check if the term always evaluates to a number.
//!
//! For example, X+Y but not X because it can also evaluate to other symbols.
[[nodiscard]] auto always_numeric(Term const &term) -> bool;

//! Check if the term never evaluates to a number.
//!
//! This is true for tuples, functions, \#sup, and \#inf.
[[nodiscard]] auto never_numeric(Term const &term) -> bool;

//! Check if the term is a symbol.
[[nodiscard]] inline auto is_symbol(Term const &term) -> bool { return std::holds_alternative<TermSymbol>(term); }

//! Check if the term is a variable.
[[nodiscard]] inline auto is_variable(Term const &term) -> bool { return std::holds_alternative<TermVariable>(term); }

//! Get the truth value of a literal, in case it is a Boolean constant.
[[nodiscard]] inline auto is_fixed(Lit const &lit) -> std::optional<bool> {
    if (auto const *blit = std::get_if<LitBool>(&lit); blit != nullptr) {
        return blit->value() == (blit->sign() != Sign::once);
    }
    return std::nullopt;
}

//! Check whether the literal is an atom.
//!
//! A literal is an atom if it is a symbolic literal without a sign.
auto is_atom(Lit const &lit) -> bool;

//! Check if the literal is a symbolic atom.
//!
//! This extends the test to disjunctions with exactly one element corresponding to an atom.
auto is_atom(HdLit const &lit) -> bool;

//! Check if the literal is a symbolic atom.
//!
//! This extends the test to conjunctions with exactly one element corresponding to an atom.
auto is_atom(BdLit const &lit) -> bool;

//! Check if a literal is a boolean constant.
inline auto is_boolean(Lit const &lit) -> std::optional<bool> {
    if (auto const *lb = std::get_if<LitBool>(&lit); lb != nullptr) {
        return lb->value() == (lb->sign() != Sign::once);
    }
    return std::nullopt;
}

//! Check if the literal is a test.
//!
//! This is used to prevent projection in projection-like rules.
//! For example, variable Y in `p(X) :- p(X,Y), X>10` is not projected
//! because `X>10` is classified as a test.
auto is_test(BdLit const &lit) -> bool;

//! Check if the literal is classical.
//!
//! This function is used to enable additional projction in rule bodies
//! whenever it can be statically determined that the head does not derive anything.
//!
//! Note that this function could also be extended to aggregates that do not derive atoms.
auto is_classical(HdLit const &lit) -> bool;

//! Check if a rule is a fact.
//!
//! Returns the symbol representing the fact.
//! This function evaluates arithmetic expressions as long as they evaluate to one-elementary pools.
auto is_fact(SymbolStore &store, StmRule const &rule) -> std::optional<Symbol>;

//! Check if a statement is a fact.
//!
//! Returns the symbol representing the fact.
//! This function evaluates arithmetic expressions as long as they evaluate to one-elementary pools.
auto is_fact(SymbolStore &store, Stm const &stm) -> std::optional<Symbol>;

//! Check that none of the given varables are local in the statement.
auto check_global(Logger &log, VariableSet const &global, Stm const &stm) -> bool;

//! @}

} // namespace Gringo::Input
