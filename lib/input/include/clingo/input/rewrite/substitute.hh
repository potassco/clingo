#pragma once

#include <clingo/input/rewrite/rewrite_context.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Replace all parameters in the given fact.
[[nodiscard]] auto map_params(RewriteContext &ctx, Location const &loc, Symbol const &sym) -> std::variant<Symbol, Stm>;

//! Replace all parameters in the given term.
[[nodiscard]] auto map_params(RewriteContext &ctx, Term const &term) -> std::optional<Term>;

//! Replace all parameters in the given literal.
[[nodiscard]] auto map_params(RewriteContext &ctx, Lit const &lit) -> std::optional<Lit>;

//! Replace all parameters in the given head literal.
[[nodiscard]] auto map_params(RewriteContext &ctx, HdLit const &lit) -> std::optional<HdLit>;

//! Replace all parameters in the given body literal.
[[nodiscard]] auto map_params(RewriteContext &ctx, BdLit const &lit) -> std::optional<BdLit>;

//! Replace all parameters in the given statement.
[[nodiscard]] auto map_params(RewriteContext &ctx, Stm const &stm) -> std::optional<Stm>;

//! Replace all variables with the given names in the statement.
[[nodiscard]] auto unmap_params(SymbolStore &store, ParamUnmap const &map, Stm const &stm) -> std::optional<Stm>;

//! Collect all ids appearing in the symbol.
void collect_ids(Symbol const &sym, StringSet &ids);

//! Collect all ids appearing in the statement.
void collect_ids(Stm const &stm, StringSet &ids);

//! Substitute variables in terms of form `X=term` in the statement.
auto substitute(RewriteContext &ctx, Stm const &stm) -> Util::ResultState<Stm, TruthValue>;

//! @}

} // namespace CppClingo::Input
