#include <algorithm>
#include <clingo/input/rewrite/iesolver.hh>

namespace CppClingo::Input {

namespace {

//! Helper to merge adjancent elements in a sequence.
template <class It, class Merge> auto merge_adjancent(It first, It last, Merge m) -> It {
    if (first == last) {
        return last;
    }

    auto result = first;
    while (++first != last) {
        // NOLINTNEXTLINE(bugprone-inc-dec-in-conditions)
        if (!m(*result, *first) && ++result != first) {
            *result = std::move(*first);
        }
    }
    return ++result;
}

} // namespace

void add_term(IETermVec &terms, IETerm term) {
    terms.emplace_back(std::move(term));
}

auto simplify(IETermVec &terms) -> Number {
    auto bound = Number{0};
    // remove terms not associated with a variable
    auto last = std::ranges::partition(terms, [](auto &term) {
                    return !term.variable.empty() && term.coefficient != 0;
                }).begin();
    for (auto end = terms.end(), current = last; current != end; ++current) {
        bound += current->coefficient;
    }
    terms.erase(last, terms.end());

    // sort according to variables
    std::ranges::sort(terms);

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
    return bound;
}

auto operator<<(std::ostream &out, IETerm const &term) -> std::ostream & {
    if (term.coefficient != 1 || term.variable.empty()) {
        out << term.coefficient;
    }
    if (term.coefficient != 0 && !term.variable.empty()) {
        out << "*" << term.variable;
    }
    return out;
}

auto IEInterval::has_value(Type type) const -> bool {
    return type == Lower ? lower_.has_value() : upper_.has_value();
}

auto IEInterval::value(Type type) const -> Number const & {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return type == Lower ? *lower_ : *upper_;
}

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
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    if (type == Lower && bound > *lower_) {
        lower_ = bound;
        return true;
    }
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
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

auto operator<<(std::ostream &out, IEInterval const &bound) -> std::ostream & {
    out << "[";
    if (bound.has_value(IEInterval::Lower)) {
        out << bound.value(IEInterval::Lower);
    } else {
        out << "-inf";
    }
    out << ",";
    if (bound.has_value(IEInterval::Upper)) {
        out << bound.value(IEInterval::Upper);
    } else {
        out << "+inf";
    }
    out << "]";
    return out;
}

auto operator<<(std::ostream &out, IE const &ie) -> std::ostream & {
    bool comma = false;
    if (ie.terms.empty()) {
        out << "0";
    }
    for (auto const &term : ie.terms) {
        if (comma) {
            out << " + ";
        }
        comma = true;
        out << term;
    }
    out << " >= " << ie.bound;
    return out;
}

void IESolver::add(IE ie) {
    auto &terms = ie.terms;

    // sort according to variables
    std::ranges::sort(terms);

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

    // remove terms not associated with a variable
    auto last = std::ranges::partition(terms, [](auto &term) {
                    return !term.variable.empty() && term.coefficient != 0;
                }).begin();
    for (auto end = terms.end(), current = last; current != end; ++current) {
        ie.bound -= current->coefficient;
    }
    terms.erase(last, terms.end());

    // add the preprocessed constraint
    if (!terms.empty() || ie.bound < 0) {
        ies_.emplace_back(std::move(ie));
    }
}

auto IESolver::compute(Logger &log) -> bool {
    if (ies_.empty()) {
        return true;
    }
    if (log.enabled(MessageCode::trace)) {
        CLINGO_REPORT(log, trace) << "computing bounds";
        CLINGO_REPORT(log, trace) << "  using the following inequalities:";
        for (auto &ie : ies_) {
            CLINGO_REPORT(log, trace) << "    " << ie;
        }
    }
    // initialize bound computation and incorporate bounds from parent
    if (parent_ != nullptr) {
        for (const auto &bound : parent_->domain_) {
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
                domain_.clear();
                CLINGO_REPORT(log, trace) << "  the inequalities are unsatisfiable";
                return false;
            }
            if (num_unbounded <= 1) {
                for (auto const &term : ie.terms) {
                    if (update_bound_(term, slack, num_unbounded)) {
                        CLINGO_REPORT(log, trace)
                            << "  set range of " << term.variable << " to " << domain_[term.variable] << " using";
                        CLINGO_REPORT(log, trace) << "    " << ie;
                        changed = true;
                    }
                }
            }
        }
    }
    if (log.enabled(MessageCode::trace)) {
        CLINGO_REPORT(log, trace) << "  obtained the following bounds:";
        for (auto const &ie : domain_) {
            CLINGO_REPORT(log, trace) << "    " << ie.first << ": " << ie.second;
        }
    }
    return true;
}

auto IESolver::strengthens(String var) const -> bool {
    if (parent_ == nullptr) {
        return true;
    }
    auto jt = parent_->domain_.find(var);
    if (jt == parent_->domain_.end()) {
        return true;
    }
    auto it = domain_.find(var);
    return it != domain_.end() && it->second != jt->second;
}

auto IESolver::update_bound_(IETerm const &term, Number slack, size_t num_unbounded) -> bool {
    assert(term.coefficient != 0);
    bool positive = term.coefficient > 0;
    auto type = positive ? IEInterval::Upper : IEInterval::Lower;
    // remove contribution of this term from slack
    if (num_unbounded == 0) {
        slack += term.coefficient * domain_[term.variable].value(type);
        // there is another term that is unbounded
    } else if (num_unbounded > 1 || domain_[term.variable].has_value(type)) {
        return false;
    }

    if (positive) {
        // ceildiv
        slack *= -1;
        slack /= term.coefficient;
        slack *= -1;
    } else {
        // floordiv
        slack /= term.coefficient;
    }
    return domain_[term.variable].refine(positive ? IEInterval::Lower : IEInterval::Upper, slack);
}

auto IESolver::update_slack_(IETerm const &term, Number &slack) -> bool {
    auto type = term.coefficient > 0 ? IEInterval::Upper : IEInterval::Lower;
    if (domain_[term.variable].has_value(type)) {
        slack -= term.coefficient * domain_[term.variable].value(type);
        return true;
    }
    return false;
}

} // namespace CppClingo::Input
