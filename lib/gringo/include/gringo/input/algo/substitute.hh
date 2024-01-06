#pragma once

#include <gringo/input/program.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Replace all parameters in the given fact.
[[nodiscard]] auto map_params(RewriteContext &ctx, Location const &loc, Symbol const &sym)
    -> std::variant<Symbol, Statement>;

//! Replace all parameters in the given term.
[[nodiscard]] auto map_params(RewriteContext &ctx, Term const &term) -> std::optional<Term>;

//! Replace all parameters in the given literal.
[[nodiscard]] auto map_params(RewriteContext &ctx, Literal const &lit) -> std::optional<Literal>;

//! Replace all parameters in the given head literal.
[[nodiscard]] auto map_params(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteral>;

//! Replace all parameters in the given body literal.
[[nodiscard]] auto map_params(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteral>;

//! Replace all parameters in the given statement.
[[nodiscard]] auto map_params(RewriteContext &ctx, Statement const &stm) -> std::optional<Statement>;

//! Replace all variables with the given names in the statement.
[[nodiscard]] auto unmap_params(SymbolStore &store, Util::ordered_map<String, String> const &map, Statement const &stm)
    -> std::optional<Statement>;

//! Collect all ids appearing in the symbol.
void collect_ids(Symbol const &sym, StringSet &ids);

//! Collect all ids appearing in the statement.
void collect_ids(Statement const &stm, StringSet &ids);

//! @}

} // namespace Gringo::Input
