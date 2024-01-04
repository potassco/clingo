#include <gringo/logger.hh>

#include <gringo/util/unordered_map.hh>

#include <gringo/input/program.hh>

namespace Gringo::Input {

auto evaluate(Symbol lhs, Relation rel, Symbol rhs) -> bool;
auto evaluate(SymbolStore &store, UnaryOperator op, Symbol rhs) -> std::optional<Symbol>;
auto evaluate(SymbolStore &store, Symbol lhs, BinaryOperator op, Symbol rhs) -> std::optional<Symbol>;
void evaluate_const(Logger &log, SymbolStore &store, std::vector<StatementConst> const &stms, ConstMap &res);

} // namespace Gringo::Input
