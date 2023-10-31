#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_rewrite_anonymous Rewrite Anonymous
//! @ingroup input_algo
//!
//! Functions to give anonymous variables in expressions a unique name.
//!
//! @{

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<Term>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(TheoryTerm const &term, NameGen &gen) -> std::optional<TheoryTerm>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Literal const &lit, NameGen &gen) -> std::optional<Literal>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(HeadLiteral const &lit, NameGen &gen) -> std::optional<HeadLiteral>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(BodyLiteral const &lit, NameGen &gen) -> std::optional<BodyLiteral>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(SymbolStore &store, Statement const &stm) -> std::optional<Statement>;

//! @}

} // namespace Gringo::Input
