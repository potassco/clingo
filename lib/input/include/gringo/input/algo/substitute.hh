#pragma once

#include <gringo/input/algo/rewrite_context.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Replace all parameters in the given fact.
[[nodiscard]] auto map_params(RewriteContext &ctx, Location const &loc,
                              SymbolRef const &sym) -> std::variant<SymbolRef, Stm>;

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
[[nodiscard]] auto unmap_params(SymbolStore &store, Util::ordered_map<StringRef, StringRef> const &map,
                                Stm const &stm) -> std::optional<Stm>;

//! Collect all ids appearing in the symbol.
void collect_ids(SymbolRef const &sym, StringRefSet &ids);

//! Collect all ids appearing in the statement.
void collect_ids(Stm const &stm, StringRefSet &ids);

//! @}

} // namespace Gringo::Input
