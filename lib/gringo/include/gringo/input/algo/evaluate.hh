#include <gringo/input/program.hh>

#include <gringo/core/logger.hh>

#include <gringo/util/unordered_map.hh>

namespace Gringo::Input {

//! @defgroup input_evaluate Evaluate
//! Functions to evaluate expressions.
//!
//! @ingroup input_algo
//!
//! @{

//! Evaluate the comparison.
[[nodiscard]] auto evaluate(Symbol lhs, Relation rel, Symbol rhs) -> bool;
//! Evaluate the unary operator.
[[nodiscard]] auto evaluate(SymbolStore &store, UnaryOperator op, Symbol rhs) -> std::optional<Symbol>;
//! Evaluate the binary operator.
[[nodiscard]] auto evaluate(SymbolStore &store, Symbol lhs, BinaryOperator op, Symbol rhs) -> std::optional<Symbol>;
//! Evaluate the term.
//!
//! Note that this will fail if the term contains variables or external functions.
[[nodiscard]] auto evaluate(Logger &log, SymbolStore &store, ConstMap const &map, Term const &term)
    -> std::optional<Symbol>;

//! Evaluate the given const statements storing the result in the given map.
//!
//! Note that this will fail if the const statements contain variables or external functions.
void evaluate_const(Logger &log, SymbolStore &store, std::vector<StmConst> const &stms, ConstMap &res);

//! @}

} // namespace Gringo::Input
