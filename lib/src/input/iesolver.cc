#include <forward_list>

#include <input/program.hh>

namespace Gringo::Input {

//! A term of form coefficient times variable.
//!
//! If the variable is the empty string, the term consists of a coefficient only.
struct IETerm {
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

    //! Whether the given bound has a value.
    [[nodiscard]] auto has_value(Type type) const -> bool;
    //! Get the value of the given bound.
    [[nodiscard]] auto value(Type type) const -> Number const &;
    //! Set the value of the given bound.
    void set_value(Type type, Number bound);
    //! Refine the value of the given bound.
    //!
    //! The interval can only get smaller.
    //! Return true if the interval changed.
    auto refine(Type type, Number const &bound) -> bool;

    //! Refine the interval with the given one.
    //!
    //! The refinement corresponds to the intersection of the two intervals.
    //! Return true if the interval changed.
    auto refine(IEInterval const &bound) -> bool;
    //! Check if the interval is finite.
    [[nodiscard]] auto is_bounded() const -> bool;
    //! Check if the interval has a tighter lower or upper bound as compared to the other one.
    [[nodiscard]] auto is_improving(IEInterval const &other) const -> bool;

  private:
    //! The lower bound of the interval.
    std::optional<Number> lower_{0};
    //! The upper bound of the interval.
    std::optional<Number> upper_{0};
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
    void add(IE ie, bool ignore_if_fixed);
    [[nodiscard]] auto is_improving(String var, IEInterval const &bound) const -> bool;
    void compute();
    template <class F> void with_domain(F visit) {
        for (auto const &bound : domain_) {
            if (bound.second.is_bounded() && is_improving(bound.first, bound.second)) {
                std::invoke(visit, bound.first, bound.second);
            }
        }
    }

  private:
    auto update_bound_(IETerm const &term, Number slack, size_t num_unbounded) -> bool;
    auto update_slack_(IETerm const &term, Number &slack) -> bool;

    IESolver *parent_;
    IEDomain domain_;
    IEDomain fixed_;
    IEVec ies_;
};

auto IEInterval::has_value(Type type) const -> bool { return type == Lower ? lower_.has_value() : upper_.has_value(); }

auto IEInterval::value(Type type) const -> Number const & { return type == Lower ? *lower_ : *upper_; }

void IEInterval::set_value(Type type, Number bound) {
    if (type == Lower) {
        lower_ = std::move(bound);
    } else {
        upper_ = std::move(bound);
    }
}

auto IEInterval::refine(Type type, Number const &bound) -> bool {
    if (!has_value(type)) {
        set_value(type, bound);
        return true;
    }
    if (type == Lower && bound > *lower_) {
        lower_ = bound;
        return true;
    }
    if (type == Upper && bound < *upper_) {
        upper_ = bound;
        return true;
    }
    return false;
}

auto IEInterval::refine(IEInterval const &bound) -> bool {
    bool ret = false;
    if (bound.has_value(Lower)) {
        ret = refine(Lower, bound.value(Lower)) || ret;
    }
    if (bound.has_value(Upper)) {
        ret = refine(Upper, bound.value(Upper)) || ret;
    }
    return ret;
}

auto IEInterval::is_bounded() const -> bool { return lower_ && upper_; }

auto IEInterval::is_improving(IEInterval const &other) const -> bool {
    if (has_value(Lower) != other.has_value(Lower) && has_value(Lower)) {
        return true;
    }
    if (has_value(Upper) != other.has_value(Upper) && has_value(Upper)) {
        return true;
    }
    if (has_value(Lower) && other.has_value(Lower) && *other.lower_ < *lower_) {
        return true;
    }
    return has_value(Upper) && other.has_value(Upper) && *upper_ < *other.upper_;
}

auto IESolver::is_improving(String var, IEInterval const &bound) const -> bool {
    auto it = fixed_.find(var);
    if (it == fixed_.end()) {
        return bound.is_bounded();
    }
    return bound.is_improving(it->second);
}

namespace {

template <class It, class Merge> auto merge_adjancent(It first, It last, Merge m) -> It {
    if (first == last) {
        return last;
    }

    auto result = first;
    while (++first != last) {
        if (!m(*result, *first) && ++result != first) {
            *result = std::move(*first);
        }
    }
    return ++result;
}

} // namespace

void IESolver::add(IE ie, bool ignore_if_fixed) {
    auto &terms = ie.terms;

    // remove terms not associated with a variable
    auto last = std::partition(terms.begin(), terms.end(),
                               [](auto &term) { return term.variable != "" && term.coefficient != 0; });
    for (auto end = terms.end(), current = last; current != end; ++current) {
        ie.bound -= current->coefficient;
    }
    terms.erase(last, terms.end());

    // sort according to variables
    std::sort(terms.begin(), terms.end(), [](auto const &a, auto const &b) { return a.variable < b.variable; });

    // combine adjacent terms referring to the same variable
    terms.erase(merge_adjancent(terms.begin(), terms.end(),
                                [](auto &a, auto &b) {
                                    if (a.variable == b.variable) {
                                        a.coefficient += b.coefficient;
                                        return true;
                                    }
                                    return false;
                                }),
                terms.end());
    ies_.emplace_back(std::move(ie));

    if (ies_.back().terms.size() == 1 && ignore_if_fixed) {
        auto term = ies_.back().terms.back();
        if (term.coefficient == 1) {
            fixed_[term.variable].refine(IEInterval::Lower, ies_.back().bound);
        } else if (term.coefficient == -1) {
            fixed_[term.variable].refine(IEInterval::Upper, -ies_.back().bound);
        }
    }
}

#ifdef CLINGO_DEBUG_INEQUALITIES

namespace {

std::ostream &operator<<(std::ostream &out, IEInterval const &bound) {
    out << "[";
    if (bound.isSet(IEInterval::Lower)) {
        out << bound.get(IEInterval::Lower);
    } else {
        out << "-inf";
    }
    out << ",";
    if (bound.isSet(IEInterval::Upper)) {
        out << bound.get(IEInterval::Upper);
    } else {
        out << "+inf";
    }
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, IE const &ie) {
    bool comma = false;
    if (ie.terms.empty()) {
        out << "0";
    }
    for (auto const &term : ie.terms) {
        if (comma) {
            out << " + ";
        }
        comma = true;
        out << term.coefficient << "*" << term.variable->name;
    }
    out << " >= " << ie.bound;
    return out;
}

} // namespace

#endif

void IESolver::compute() {
#ifdef CLINGO_DEBUG_INEQUALITIES
    for (auto &ie : ies_) {
        std::cerr << ie << std::endl;
    }
#endif
    // initialize bound computation and incorporate bounds from parent
    domain_.clear();
    if (parent_ != nullptr) {
        for (const auto &bound : parent_->domain_) {
            fixed_[bound.first].refine(bound.second);
            domain_[bound.first].refine(bound.second);
        }
    }

    // compute bounds
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto const &ie : ies_) {
            Number slack = ie.bound;
            size_t num_unbounded = 0;
            for (auto const &term : ie.terms) {
                if (!update_slack_(term, slack)) {
                    ++num_unbounded;
                }
            }
            if (num_unbounded == 0 && slack > 0) {
                // we simply set all bounds to empty intervals
                for (auto const &ie : ies_) {
                    for (auto const &term : ie.terms) {
                        domain_[term.variable].set_value(IEInterval::Lower, 1);
                        domain_[term.variable].set_value(IEInterval::Upper, 0);
                    }
                }
                changed = false;
                break;
            }
            if (num_unbounded <= 1) {
                for (auto const &term : ie.terms) {
                    if (update_bound_(term, slack, num_unbounded)) {
#ifdef CLINGO_DEBUG_INEQUALITIES
                        std::cerr << "  update bound using " << ie << std::endl;
                        std::cerr << "    the new bound for " << *term.variable << " is " << domain_[term.variable]
                                  << std::endl;
#endif
                        changed = true;
                    }
                }
            }
        }
    }
}

auto IESolver::update_bound_(IETerm const &term, Number slack, size_t num_unbounded) -> bool {
    bool positive = term.coefficient > 0;
    auto type = positive ? IEInterval::Upper : IEInterval::Lower;
    if (num_unbounded == 0) {
        slack += term.coefficient * domain_[term.variable].value(type);
    } else if (num_unbounded > 1 || domain_[term.variable].has_value(type)) {
        return false;
    }

    if (positive) {
        // floordiv
        slack /= term.coefficient;
    } else {
        // ceildiv
        slack *= -1;
        slack /= term.coefficient;
        slack *= -1;
    }
    return domain_[term.variable].refine(positive ? IEInterval::Lower : IEInterval::Upper, std::move(slack));
}

auto IESolver::update_slack_(IETerm const &term, Number &slack) -> bool {
    auto type = term.coefficient > 0 ? IEInterval::Upper : IEInterval::Lower;
    if (domain_[term.variable].has_value(type)) {
        slack *= -1;
        slack += term.coefficient * domain_[term.variable].value(type);
        return true;
    }
    return false;
}

} // namespace Gringo::Input
