#include <map>

#include <logger.hh>

#include <input/statement.hh>

namespace Gringo::Input {

auto evaluate(Symbol lhs, Relation rel, Symbol rhs) -> bool;
auto evaluate(SymbolStore &store, UnaryOperator op, Symbol rhs) -> std::optional<Symbol>;
auto evaluate(SymbolStore &store, Symbol lhs, BinaryOperator op, Symbol rhs) -> std::optional<Symbol>;
auto evaluate(Logger &log, SymbolStore &store, std::unordered_map<String, std::optional<Symbol>> const &map,
              Term const &term) -> std::optional<Symbol>;
auto evaluate_const(Logger &log, SymbolStore &store, std::vector<StatementConst> const &stms)
    -> std::unordered_map<String, std::optional<Symbol>>;

} // namespace Gringo::Input
