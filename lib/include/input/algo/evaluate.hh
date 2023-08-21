#include <map>

#include <input/statement.hh>

namespace Gringo::Input {

auto evaluate(UnaryOperator op, Symbol const &rhs) -> std::optional<Symbol>;
auto evaluate(Symbol const &lhs, BinaryOperator op, Symbol const &rhs) -> std::optional<Symbol>;
auto evaluate(SymbolStore &store, std::unordered_map<String, std::optional<Symbol>> const &map, Term const &term)
    -> std::optional<Symbol>;
auto evaluate_const(std::vector<StatementConst> const &stms) -> std::map<std::string, std::optional<Symbol>>;

} // namespace Gringo::Input
