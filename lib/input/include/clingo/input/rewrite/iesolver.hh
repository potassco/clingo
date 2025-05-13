#pragma once

#include <clingo/input/program.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! A term of form coefficient times variable.
//!
//! If the variable is the empty string, the term consists of a coefficient only.
struct IETerm {
    //! Operator to print terms.
    friend auto operator<<(std::ostream &out, IETerm const &term) -> std::ostream &;
    //! Equality compare two terms.
    friend auto operator==(IETerm const &a, IETerm const &b) -> bool = default;
    //! Compare two terms.
    friend auto operator<=>(IETerm const &a, IETerm const &b) {
        return std::tie(a.variable, a.coefficient) <=> std::tie(b.variable, b.coefficient);
    }

    //! The integer coefficient of the term.
    Number coefficient;
    //! The variable of the term or the empty string if there is no variable.
    String variable;
};
//! A vector of terms representing a sum of terms.
using IETermVec = std::vector<IETerm>;

//! Add a term to a sum of terms.
void add_term(IETermVec &terms, IETerm term);
//! Simplify the given sum of terms.
//!
//! Returns the sum constants and removes them from the sum.
auto simplify(IETermVec &terms) -> Number;

//! An inequality of form terms >= bound.
struct IE {
    //! Operator to print inequalities.
    friend auto operator<<(std::ostream &out, IE const &ie) -> std::ostream &;

    //! The terms.
    IETermVec terms;
    //! The lower bound.
    Number bound;
};
//! A system of inequalities.
using IEVec = std::vector<IE>;

//! A interval of integers.
class IEInterval {
  public:
    //! Enum to indicate the lower and upper bound of the interval.
    enum Type : uint8_t { Lower, Upper };
    //! Create an unbounded interval.
    IEInterval() = default;
    //! Create an interval with the given bounds.
    IEInterval(std::optional<Number> lower, std::optional<Number> upper)
        : lower_{std::move(lower)}, upper_{std::move(upper)} {}

    //! Whether the given bound has a value.
    [[nodiscard]] auto has_value(Type type) const -> bool;
    //! Get the value of the given bound.
    [[nodiscard]] auto value(Type type) const -> Number const &;
    //! Set the value of the given bound.
    void set_value(Type type, Number bound);
    //! Refine the value of the given bound.
    //!
    //! The interval can only value smaller.
    //! Return true if the interval changed.
    auto refine(Type type, Number const &bound) -> bool;

    //! Refine the interval with the given one.
    //!
    //! The refinement corresponds to the intersection of the two intervals.
    //! Return true if the interval changed.
    auto refine(IEInterval const &bound) -> bool;

    //! Operator to print intervals.
    friend auto operator<<(std::ostream &out, IEInterval const &bound) -> std::ostream &;
    //! Equality compare two terms.
    friend auto operator==(IEInterval const &a, IEInterval const &b) -> bool = default;
    //! Inequality compare two terms.
    friend auto operator!=(IEInterval const &a, IEInterval const &b) -> bool = default;

  private:
    //! The lower bound of the interval.
    std::optional<Number> lower_;
    //! The upper bound of the interval.
    std::optional<Number> upper_;
};

//! A map from variables to intervals.
using IEDomain = Util::ordered_map<String, IEInterval>;

//! A (partial) solver for inequalities.
class IESolver {
  public:
    //! Construct an IESolver with an optional parent.
    IESolver(IESolver *parent = nullptr) : parent_{parent} {}
    //! Add the given inequality to the solver.
    void add(IE ie);
    //! Compute the bounds of variables in added inequalities.
    //!
    //! Returns false if the inequalities are not satisfiable.
    [[nodiscard]] auto compute(Logger &log) -> bool;
    //! Get the domains of variables.
    [[nodiscard]] auto domain() const -> IEDomain const & { return domain_; }
    //! Return true if the solver strengthens the domain of the given variable.
    [[nodiscard]] auto strengthens(String var) const -> bool;

  private:
    //! Update the bound of the given term's variable.
    //!
    //! The term must be part of an inequality with the given slack.
    //! The number of bounded terms is used to infer whether bounds can be updated.
    //!
    //! Returns true if the bound could be refined.
    auto update_bound_(IETerm const &term, Number slack, size_t num_unbounded) -> bool;
    //! Update the slack w.r.t. the given term.
    //!
    //! Returns true if the term could be used to update the slack.
    auto update_slack_(IETerm const &term, Number &slack) -> bool;

    //! The parent IESolver.
    IESolver *parent_;
    //! The computed domain of variables.
    IEDomain domain_;
    //! The inequalities used to compute bounds.
    IEVec ies_;
};

//! @}

} // namespace CppClingo::Input
