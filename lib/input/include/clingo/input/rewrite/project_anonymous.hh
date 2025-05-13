#pragma once

#include <clingo/input/program.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Project positional anonymous variables in the term.
[[nodiscard]] auto project_anonymous(Term const &term) -> std::optional<Term>;

//! Project anonymous variables in negated symbolic literals.
[[nodiscard]] auto project_anonymous(Lit const &lit) -> std::optional<Lit>;

//! Project anonymous variables in (nested) negated symbolic literals.
[[nodiscard]] auto project_anonymous(HdLit const &lit) -> std::optional<HdLit>;

//! Project anonymous variables in (nested) negated symbolic literals.
[[nodiscard]] auto project_anonymous(BdLit const &lit) -> std::optional<BdLit>;

//! Project anonymous variables in (nested) negated symbolic literals.
[[nodiscard]] auto project_anonymous(Stm const &stm) -> std::optional<Stm>;

//! @}

} // namespace CppClingo::Input
