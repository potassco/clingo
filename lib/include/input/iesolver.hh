#include <input/program.hh>

namespace Gringo::Input {

//! A term of form coefficient times variable.
//!
//! If the variable is the empty string, the term consists of a coefficient only.
struct IETerm {
    //! Operator to print terms.
    friend auto operator<<(std::ostream &out, IETerm const &term) -> std::ostream &;
    //! Less than compare to terms.
    friend auto operator<(IETerm const &a, IETerm const &b) -> bool;

    //! The integer coefficient of the term.
    Number coefficient;
    //! The variable of the term or the empty string if there is no variable.
    String variable;
};
//! A vector of terms representing a sum of terms.
using IETermVec = std::vector<IETerm>;

//! Add a term to a sum of terms.
void add_term(IETermVec &terms, IETerm const &term);
//! Subtract a term from a sum of terms.
void sub_term(IETermVec &terms, IETerm const &term);

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
    enum Type { Lower, Upper };
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
    friend auto operator<<(std::ostream &out, IE const &ie) -> std::ostream &;
    //! Equality compare two terms.
    friend auto operator==(IEInterval const &a, IEInterval const &b) -> bool;
    //! Inequality compare two terms.
    friend auto operator!=(IEInterval const &a, IEInterval const &b) -> bool;

  private:
    //! The lower bound of the interval.
    std::optional<Number> lower_;
    //! The upper bound of the interval.
    std::optional<Number> upper_;
};

//! A map from variables to intervals.
using IEDomain = Util::ordered_map<String, IEInterval>;

class IESolver {
  public:
    //! Construct an IESolver with an optional parent.
    IESolver(IESolver *parent = nullptr) : parent_{parent} {}
    //! Add the given inequality to the solver.
    //!
    //! @todo: I belive the ignore if fixed had something to do with existing assignments and intervals.
    //!
    //! For example, given X=1, we do not want to add the interval X=1..1.
    //! Similarly, given X=1..2, we do not want to add the interval X=1..2.
    //!
    //! It seems like fixed ranges should be handled specially.
    //! Any literal of form X=u..t and X R u
    //! for variables X, constants u and t, and relations R among <, <=, =, >, >=
    //! should be subject to refinement given the computed bounds.
    void add(IE ie);
    [[nodiscard]] auto compute(Logger &log) -> bool;
    [[nodiscard]] auto domain() const -> IEDomain const & { return domain_; }

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

} // namespace Gringo::Input
