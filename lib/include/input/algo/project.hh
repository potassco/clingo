#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Enumeration to select variables to project.
//!
//! @see Projection
enum class ProjectionMode {
    disabled = 0,  //!< Disable projection.
    anonymous = 1, //!< Only project anonymous variables.
    pure = 2,      //!< Project pure variables.
};

//! Helper to gather projection related arguments.
class Projection {
  public:
    //! Constructor taking the mode which variables to project and a map with counts of variables.
    explicit Projection(ProjectionMode mode, std::unordered_map<std::string, size_t> const &counts)
        : counts_{counts}, mode_{mode} {};
    //! Return whether a the given variable should be projected.
    //!
    //! Only variables with a count of exactly one can be projected while the mode adds further restrictions.
    [[nodiscard]] auto projectable(std::string const &var, bool anonymous) const -> bool;
    //! Return the variable counts.
    [[nodiscard]] auto counts() const -> std::unordered_map<std::string, size_t> const &;
    //! Return the mode.
    [[nodiscard]] auto mode() const -> ProjectionMode;

  private:
    //! The variable counts.
    std::unordered_map<std::string, size_t> const &counts_;
    //! The projection mode.
    ProjectionMode mode_;
};

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Term const &term, Projection project) -> std::optional<Term>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(Literal const &lit, Projection project) -> std::optional<Literal>;

//! Project variables according to given projection mode.
[[nodiscard]] auto project(HeadLiteral const &lit, Projection project) -> std::optional<HeadLiteral>;

//! Project variables according to given projection mode and scope.
//!
//! Some literal occurrences cannot be projected preserving equivalence.
//! For example, variables in nonmonotone aggregates are only projected in classical scope.
[[nodiscard]] auto project(BodyLiteral const &lit, Projection project, bool in_classical_scope)
    -> std::optional<BodyLiteral>;

//! Project variables according to given projection mode.
//!
//! Optionally, project anonymous variables in negative scope (deprecated).
[[nodiscard]] auto project(Statement const &stm, ProjectionMode mode, bool project_anonymous)
    -> std::optional<Statement>;

} // namespace Gringo::Input
