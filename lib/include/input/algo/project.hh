#pragma once

#include <util/unordered_map.hh>

#include <input/algo/rewrite_base.hh>

namespace Gringo::Input {

//! @addtogroup input_rewrite
//! @{

//! Helper to gather projection related arguments.
class ProjectionMap {
  public:
    //! Constructor taking the mode which variables to project and a map with counts of variables.
    explicit ProjectionMap(ProjectionMode mode, Util::unordered_map<String, size_t> const &counts)
        : counts_{counts}, mode_{mode} {};
    //! Return whether a the given variable should be projected.
    //!
    //! Only variables with a count of exactly one can be projected while the mode adds further restrictions.
    [[nodiscard]] auto projectable(String const &var, bool anonymous) const -> bool;
    //! Return the variable counts.
    [[nodiscard]] auto counts() const -> Util::unordered_map<String, size_t> const &;
    //! Return the mode.
    [[nodiscard]] auto mode() const -> ProjectionMode;

  private:
    //! The variable counts.
    Util::unordered_map<String, size_t> const &counts_;
    //! The projection mode.
    ProjectionMode mode_;
};

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Term const &term, ProjectionMap project) -> std::optional<Term>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Literal const &lit, ProjectionMap project) -> std::optional<Literal>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(HeadLiteral const &lit, ProjectionMap project) -> std::optional<HeadLiteral>;

//! Project variables according to given projection mode and scope.
//!
//! Some literal occurrences cannot be projected preserving equivalence.
//! For example, variables in nonmonotone aggregates are only projected in classical scope.
[[nodiscard]] auto project(BodyLiteral const &lit, ProjectionMap project, bool in_classical_scope)
    -> std::optional<BodyLiteral>;

//! Project variables according to given projection mode.
//!
//! Optionally, project anonymous variables in negative scope (deprecated).
[[nodiscard]] auto project(Statement const &stm, ProjectionMode mode, bool project_anonymous)
    -> std::optional<Statement>;

//! @}

} // namespace Gringo::Input
