#pragma once

#include <clingo/input/program.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<Term>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(TheoryTerm const &term, NameGen &gen) -> std::optional<TheoryTerm>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Lit const &lit, NameGen &gen) -> std::optional<Lit>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(HdLit const &lit, NameGen &gen) -> std::optional<HdLit>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(BdLit const &lit, NameGen &gen) -> std::optional<BdLit>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(SymbolStore &store, Stm const &stm) -> std::optional<Stm>;

//! @}

} // namespace CppClingo::Input
