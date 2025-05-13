#pragma once

#include <clingo/input/statement.hh>

#include <clingo/core/logger.hh>

#include <cstring>

namespace CppClingo::Input {

//! @addtogroup input_analyze
//! @{

//! Enumeration for Term::check_type().
enum class TermCheckType : uint8_t {
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
    Number const *pos_number = nullptr;
    //! The identifier represented by the term.
    String identifier;
};

//! Query information about the structure of the given term.
[[nodiscard]] auto check_type(Term const &term, TermCheckType type, CheckTypeResult *res = nullptr) -> bool;

//! Helper to access elements of linear terms.
class LinearTerm {
  public:
    //! The coefficient.
    [[nodiscard]] auto m() const -> Number const & { return std::get<TermSymbol>(term_m()).value().num(); }
    //! The constant.
    [[nodiscard]] auto n() const -> Number const & { return std::get<TermSymbol>(term_n()).value().num(); }
    //! The variable name.
    [[nodiscard]] auto x() const -> String { return std::get<TermVariable>(term_x()).name(); }
    //! The term for the coefficient.
    [[nodiscard]] auto term_m() const -> Term const & { return *std::get<TermBinary>(term_mx()).lhs(); }
    //! The term for the constant.
    [[nodiscard]] auto term_n() const -> Term const & { return mxn_->rhs(); }
    //! The term for the variable.
    [[nodiscard]] auto term_x() const -> Term const & { return *std::get<TermBinary>(term_mx()).rhs(); }
    //! The term for the coefficient times the variable.
    [[nodiscard]] auto term_mx() const -> Term const & { return mxn_->lhs(); }
    //! The term for the whole linear term.
    [[nodiscard]] auto term_mxn() const -> TermBinary const & { return *mxn_; }

    friend auto check_linear(TermBinary const &term) -> std::optional<LinearTerm>;

  private:
    LinearTerm(TermBinary const &mxn) : mxn_{&mxn} {}

    TermBinary const *mxn_;
};

//! Check if the given term is a linear term.
[[nodiscard]] auto is_linear(Term const &term) -> bool;

//! See is_linear().
[[nodiscard]] auto is_linear(TermBinary const &term) -> bool;

//! Check if the given term is a linear term and return an object to access its elements.
auto check_linear(TermBinary const &term) -> std::optional<LinearTerm>;

//! See check_linear().
auto check_linear(Term const &term) -> std::optional<LinearTerm>;

//! Returns true if the term has form t..u.
[[nodiscard]] auto is_interval(Term const &term) -> bool;

//! Returns true if the term has form \@f(...).
[[nodiscard]] auto is_external(Term const &term) -> bool;

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
[[nodiscard]] inline auto is_symbol(Term const &term) -> bool {
    return std::holds_alternative<TermSymbol>(term);
}

//! Check if the term is a variable.
[[nodiscard]] inline auto is_variable(Term const &term) -> bool {
    return std::holds_alternative<TermVariable>(term);
}

//! Get the truth value of a literal, in case it is a Boolean constant.
[[nodiscard]] inline auto is_fixed(Lit const &lit) -> std::optional<bool> {
    if (auto const *blit = std::get_if<LitBool>(&lit); blit != nullptr) {
        return blit->value() == (blit->sign() != Sign::once);
    }
    return std::nullopt;
}

//! Check whether the term can represent an atom.
[[nodiscard]] auto is_atom(Term const &term) -> bool;

//! Check whether the literal is an atom.
//!
//! A literal is an atom if it is a symbolic literal without a sign.
[[nodiscard]] auto is_atom(Lit const &lit) -> bool;

//! Check if the literal is a symbolic atom.
//!
//! This extends the test to disjunctions with exactly one element corresponding to an atom.
[[nodiscard]] auto is_atom(HdLit const &lit) -> bool;

//! Check if the literal is a symbolic atom.
//!
//! This extends the test to conjunctions with exactly one element corresponding to an atom.
[[nodiscard]] auto is_atom(BdLit const &lit) -> bool;

//! Check if a literal is a boolean constant.
[[nodiscard]] inline auto is_boolean(Lit const &lit) -> std::optional<bool> {
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
[[nodiscard]] auto is_test(BdLit const &lit) -> bool;

//! Check if the literal is classical.
//!
//! This function is used to enable additional projection in rule bodies
//! whenever it can be statically determined that the head does not derive anything.
//!
//! Note that this function could also be extended to aggregates that do not derive atoms.
[[nodiscard]] auto is_classical(HdLit const &lit) -> bool;

//! Check if a rule is a fact.
//!
//! Returns the symbol representing the fact.
//! This function evaluates arithmetic expressions as long as they evaluate to one-elementary pools.
[[nodiscard]] auto is_fact(SymbolStore &store, StmRule const &rule) -> std::optional<Symbol>;

//! Check if a statement is a fact.
//!
//! Returns the symbol representing the fact.
//! This function evaluates arithmetic expressions as long as they evaluate to one-elementary pools.
[[nodiscard]] auto is_fact(SymbolStore &store, Stm const &stm) -> std::optional<Symbol>;

//! Check that none of the given variables are local in the statement.
[[nodiscard]] auto check_global(Logger &log, VariableSet const &global, Stm const &stm) -> bool;

//! Check if the given string is a theory operator.
[[nodiscard]] inline auto is_theory_operator(std::string_view name) -> bool {
    return (!name.empty() && std::strchr("/!<=>+-*\\?&@|:;~^.", name.front()) != nullptr) || (name == "not");
}

//! Get the signature of a term.
[[nodiscard]] auto signature(Term const &term) -> std::optional<std::tuple<String, size_t, bool>>;

//! Check if a term is matchable.
//!
//! Matchable terms only contain (non-external) functions, tuples, constants,
//! variables, linear terms, and negated terms.
//!
//! For example, the following terms is matchable: `f(1,c,X,X+1,-X,(X,Y))`.
[[nodiscard]] auto is_matchable(Term const &term) -> bool;

//! Extract terms whose evaluation can fail.
void extract_can_fail(Term const &term, std::vector<Term> &result);

//! @}

} // namespace CppClingo::Input
