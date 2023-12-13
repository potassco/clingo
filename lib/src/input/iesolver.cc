#include <forward_list>

#include <input/program.hh>

namespace Gringo::Input {

struct IETerm {
    Number coefficient;
    String variable;
};
using IETermVec = std::vector<IETerm>;

void add_term(IETermVec &terms, IETerm const &term);
void sub_term(IETermVec &terms, IETerm const &term);

struct IE {
    IETermVec terms;
    Number bound;
};
using IEVec = std::vector<IE>;

class IEBound {
  public:
    enum Type { Lower, Upper };

    [[nodiscard]] auto has_value(Type type) const -> bool;
    [[nodiscard]] auto value(Type type) const -> Number const &;
    void set_value(Type type, Number bound);
    auto refine(Type type, Number const &bound) -> bool;
    auto refine(IEBound const &bound) -> bool;
    [[nodiscard]] auto is_bounded() const -> bool;
    [[nodiscard]] auto is_improving(IEBound const &other) const -> bool;
    friend auto operator<(IEBound const &a, IEBound const &b) -> bool;

  private:
    std::optional<Number> lower_{0};
    std::optional<Number> upper_{0};
};

using IEBoundMap = Util::ordered_map<String, IEBound>;

class IESolver;

class IEContext {
  public:
    virtual ~IEContext() noexcept = default;

    virtual void gather(IESolver &solver) const = 0;
    virtual void add_bound(String const &var, IEBound const &bound) = 0;
};

class IESolver {
  public:
    IESolver(IEContext &ctx, IESolver *parent = nullptr) : parent_{parent}, ctx_{ctx} {}
    void add(IE ie, bool ignore_if_fixed);
    void add(IEContext &context);
    [[nodiscard]] auto is_improving(String var, IEBound const &bound) const -> bool;
    void compute();

  private:
    using SubSolvers = std::forward_list<IESolver>;
    auto update_bound_(IETerm const &term, Number slack, size_t num_unbounded) -> bool;
    auto update_slack_(IETerm const &term, Number &slack) -> bool;

    IESolver *parent_;
    IEContext &ctx_;
    SubSolvers sub_solvers_;
    IEBoundMap bounds_;
    IEBoundMap fixed_;
    IEVec ies_;
};

auto IEBound::has_value(Type type) const -> bool { return type == Lower ? lower_.has_value() : upper_.has_value(); }

auto IEBound::value(Type type) const -> Number const & { return type == Lower ? *lower_ : *upper_; }

void IEBound::set_value(Type type, Number bound) {
    if (type == Lower) {
        lower_ = std::move(bound);
    } else {
        upper_ = std::move(bound);
    }
}

auto IEBound::refine(Type type, Number const &bound) -> bool {
    if (!has_value(type)) {
        set_value(type, bound);
        return true;
    }
    if (type == Lower && bound > lower_) {
        lower_ = bound;
        return true;
    }
    if (type == Upper && bound < upper_) {
        upper_ = bound;
        return true;
    }
    return false;
}

auto IEBound::refine(IEBound const &bound) -> bool {
    bool ret = false;
    if (bound.has_value(Lower)) {
        ret = refine(Lower, bound.value(Lower)) || ret;
    }
    if (bound.has_value(Upper)) {
        ret = refine(Upper, bound.value(Upper)) || ret;
    }
    return ret;
}

auto IEBound::is_bounded() const -> bool { return lower_ && upper_; }

auto IEBound::is_improving(IEBound const &other) const -> bool {
    if (!other.is_bounded() || !is_bounded()) {
        return false;
    }
    return other.lower_ < lower_ || upper_ < other.upper_;
}

auto IESolver::is_improving(String var, IEBound const &bound) const -> bool {
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
                               [](auto &term) { return term.variable != nullptr && term.coefficient != 0; });
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
            fixed_[term.variable].refine(IEBound::Lower, ies_.back().bound);
        } else if (term.coefficient == -1) {
            fixed_[term.variable].refine(IEBound::Upper, -ies_.back().bound);
        }
    }
}

#ifdef CLINGO_DEBUG_INEQUALITIES

namespace {

std::ostream &operator<<(std::ostream &out, IEBound const &bound) {
    out << "[";
    if (bound.isSet(IEBound::Lower)) {
        out << bound.get(IEBound::Lower);
    } else {
        out << "-inf";
    }
    out << ",";
    if (bound.isSet(IEBound::Upper)) {
        out << bound.get(IEBound::Upper);
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
    bounds_.clear();
    ctx_.gather(*this);
    if (parent_ != nullptr) {
        for (const auto &bound : parent_->bounds_) {
            fixed_[bound.first].refine(bound.second);
            bounds_[bound.first].refine(bound.second);
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
                        bounds_[term.variable].set_value(IEBound::Lower, 1);
                        bounds_[term.variable].set_value(IEBound::Upper, 0);
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
                        std::cerr << "    the new bound for " << *term.variable << " is " << bounds_[term.variable]
                                  << std::endl;
#endif
                        changed = true;
                    }
                }
            }
        }
    }

    // add computed bounds and then compute bounds for nested scopes
    for (auto const &bound : bounds_) {
        if (is_improving(bound.first, bound.second)) {
            ctx_.add_bound(bound.first, bound.second);
        }
    }
    for (auto &solver : sub_solvers_) {
        solver.compute();
    }
}

void IESolver::add(IEContext &context) { sub_solvers_.emplace_front(context, this); }

auto IESolver::update_bound_(IETerm const &term, Number slack, size_t num_unbounded) -> bool {
    bool positive = term.coefficient > 0;
    auto type = positive ? IEBound::Upper : IEBound::Lower;
    if (num_unbounded == 0) {
        slack += term.coefficient * bounds_[term.variable].value(type);
    } else if (num_unbounded > 1 || bounds_[term.variable].has_value(type)) {
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
    return bounds_[term.variable].refine(positive ? IEBound::Lower : IEBound::Upper, std::move(slack));
}

auto IESolver::update_slack_(IETerm const &term, Number &slack) -> bool {
    auto type = term.coefficient > 0 ? IEBound::Upper : IEBound::Lower;
    if (bounds_[term.variable].has_value(type)) {
        slack *= -1;
        slack += term.coefficient * bounds_[term.variable].value(type);
        return true;
    }
    return false;
}

} // namespace Gringo::Input
