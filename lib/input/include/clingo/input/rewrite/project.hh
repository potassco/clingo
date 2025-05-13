#pragma once

#include <clingo/input/program.hh>

#include <clingo/util/unordered_map.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Helper to gather projection related arguments.
class ProjectionMap {
  public:
    //! Constructor taking the mode which variables to project and a map with counts of variables.
    explicit ProjectionMap(ProjectionMode mode, Util::unordered_map<String, size_t> const &counts)
        : counts_{&counts}, mode_{mode} {};
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
    Util::unordered_map<String, size_t> const *counts_;
    //! The projection mode.
    ProjectionMode mode_;
};

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Term const &term, ProjectionMap project) -> std::optional<Term>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Lit const &lit, ProjectionMap project) -> std::optional<Lit>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(HdLit const &lit, ProjectionMap project) -> std::optional<HdLit>;

//! Project variables according to given projection mode and scope.
//!
//! Some literal occurrences cannot be projected preserving equivalence.
//! For example, variables in nonmonotone aggregates are only projected in classical scope.
[[nodiscard]] auto project(BdLit const &lit, ProjectionMap project, bool in_classical_scope) -> std::optional<BdLit>;

//! Project variables according to given projection mode.
//!
//! Optionally, project anonymous variables in negative scope (deprecated).
[[nodiscard]] auto project(RewriteOptions const &opts, Stm const &stm) -> std::optional<Stm>;

//! @}

} // namespace CppClingo::Input
