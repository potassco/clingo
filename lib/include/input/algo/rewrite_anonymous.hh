#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Generator for auxiliary variables.
class NameGen {
  public:
    //! Constructor taking a set of variables names.
    //!
    //! The generator ensures that there are no collisions with these names.
    NameGen(VariableSet vars) : vars_{std::move(vars)} {}
    //! Generate a unique variable name.
    [[nodiscard]] auto new_name() -> std::string;

  private:
    //! Taken variable names.
    VariableSet vars_;
    //! Running number used to generate variable names.
    size_t num_ = 0;
};

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<Term>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(TheoryTerm const &term, NameGen &gen) -> std::optional<STheoryTerm>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Literal const &lit, NameGen &gen) -> std::optional<SLiteral>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(HeadLiteral const &lit, NameGen &gen) -> std::optional<SHeadLiteral>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(BodyLiteral const &lit, NameGen &gen) -> std::optional<SBodyLiteral>;

//! Give anonymous variables a unique name.
[[nodiscard]] auto rewrite_anonymous(Statement const &stm) -> std::optional<SStatement>;

} // namespace Gringo::Input
