#include <forward_list>

#include <input/program.hh>

namespace Gringo::Input {

struct IETerm {
    int coefficient{0};
    String variable;
};
using IETermVec = std::vector<IETerm>;

void addIETerm(IETermVec &terms, IETerm const &term);
void subIETerm(IETermVec &terms, IETerm const &term);

struct IE {
    IETermVec terms;
    int bound;
};
using IEVec = std::vector<IE>;

class IEBound {
  public:
    enum Type { Lower, Upper };

    [[nodiscard]] auto isSet(Type type) const -> bool;
    [[nodiscard]] auto get(Type type) const -> int;
    void set(Type type, int bound);
    auto refine(Type type, int bound) -> bool;
    auto refine(IEBound const &bound) -> bool;
    [[nodiscard]] auto isBounded() const -> bool;
    [[nodiscard]] auto isImproving(IEBound const &other) const -> bool;
    friend auto operator<(IEBound const &a, IEBound const &b) -> bool;

  private:
    int lower_{0};
    int upper_{0};
    bool hasLower_{false};
    bool hasUpper_{false};
};

using IEBoundMap = Util::ordered_map<String, IEBound>;

class IESolver;

class IEContext {
  public:
    IEContext() = default;
    IEContext(IEContext const &other) = default;
    IEContext(IEContext &&other) noexcept = default;
    auto operator=(IEContext const &other) -> IEContext & = default;
    auto operator=(IEContext &&other) noexcept -> IEContext & = default;
    virtual ~IEContext() noexcept = default;

    virtual void gatherIEs(IESolver &solver) const = 0;
    virtual void addIEBound(String const &var, IEBound const &bound) = 0;
};

class IESolver {
  public:
    IESolver(IEContext &ctx, IESolver *parent = nullptr) : parent_{parent}, ctx_{ctx} {}
    void add(IE ie, bool ignoreIfFixed);
    void add(IEContext &context);
    [[nodiscard]] auto isImproving(String var, IEBound const &bound) const -> bool;
    void compute();

  private:
    enum class UpdateResult { changed, unchanged, overflow };
    using SubSolvers = std::forward_list<IESolver>;
    auto update_bound_(IETerm const &term, int64_t slack, int num_unbounded) -> UpdateResult;
    auto update_slack_(IETerm const &term, int64_t &slack, int &num_unbounded) -> bool;

    IESolver *parent_;
    IEContext &ctx_;
    SubSolvers subSolvers_;
    IEBoundMap bounds_;
    IEBoundMap fixed_;
    IEVec ies_;
};

auto IEBound::isSet(Type type) const -> bool { return type == Lower ? hasLower_ : hasUpper_; }

auto IEBound::get(Type type) const -> int { return type == Lower ? lower_ : upper_; }

void IEBound::set(Type type, int bound) {
    if (type == Lower) {
        hasLower_ = true;
        lower_ = bound;
    } else {
        hasUpper_ = true;
        upper_ = bound;
    }
}

auto IEBound::refine(Type type, int bound) -> bool {
    if (!isSet(type)) {
        set(type, bound);
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
    if (bound.isSet(Lower)) {
        ret = refine(Lower, bound.get(Lower)) || ret;
    }
    if (bound.isSet(Upper)) {
        ret = refine(Upper, bound.get(Upper)) || ret;
    }
    return ret;
}

auto IEBound::isBounded() const -> bool { return hasLower_ && hasUpper_; }

auto IEBound::isImproving(IEBound const &other) const -> bool {
    if (!other.isBounded() || !isBounded()) {
        return false;
    }
    return other.lower_ < lower_ || upper_ < other.upper_;
}

auto IESolver::isImproving(String var, IEBound const &bound) const -> bool {
    auto it = fixed_.find(var);
    if (it == fixed_.end()) {
        return bound.isBounded();
    }
    return bound.isImproving(it->second);
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

void IESolver::add(IE ie, bool ignoreIfFixed) {
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

    if (ies_.back().terms.size() == 1 && ignoreIfFixed) {
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
    ctx_.gatherIEs(*this);
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
            int64_t slack = ie.bound;
            int num_unbounded = 0;
            for (auto const &term : ie.terms) {
                // In case the slack cannot be updated due to an overflow, no bounds are calculated.
                if (!update_slack_(term, slack, num_unbounded)) {
                    return;
                }
            }
            if (num_unbounded == 0 && slack > 0) {
                // we simply set all bounds to empty intervals
                for (auto const &ie : ies_) {
                    for (auto const &term : ie.terms) {
                        bounds_[term.variable].set(IEBound::Lower, 1);
                        bounds_[term.variable].set(IEBound::Upper, 0);
                    }
                }
                changed = false;
                break;
            }
            if (num_unbounded <= 1) {
                for (auto const &term : ie.terms) {
                    auto res = update_bound_(term, slack, num_unbounded);
                    if (res == UpdateResult::changed) {
#ifdef CLINGO_DEBUG_INEQUALITIES
                        std::cerr << "  update bound using " << ie << std::endl;
                        std::cerr << "    the new bound for " << *term.variable << " is " << bounds_[term.variable]
                                  << std::endl;
#endif
                        changed = true;
                    }
                    if (res == UpdateResult::overflow) {
                        return;
                    }
                }
            }
        }
    }

    // add computed bounds and then compute bounds for nested scopes
    for (auto const &bound : bounds_) {
        if (isImproving(bound.first, bound.second)) {
            ctx_.addIEBound(bound.first, bound.second);
        }
    }
    for (auto &solver : subSolvers_) {
        solver.compute();
    }
}

void IESolver::add(IEContext &context) { subSolvers_.emplace_front(context, this); }

namespace {

template <typename I> auto floordiv(I n, I m) -> I {
    using std::div;
    auto a = div(n, m);
    if (((n < 0) ^ (m < 0)) && a.rem != 0) {
        a.quot--;
    }
    return a.quot;
}

template <typename I> auto ceildiv(I n, I m) -> I {
    using std::div;
    auto a = div(n, m);
    if (((n < 0) ^ (m < 0)) && a.rem != 0) {
        a.quot++;
    }
    return a.quot;
}

template <typename I> auto div(bool positive, I a, I b) -> I { return positive ? floordiv(a, b) : ceildiv(a, b); }

auto clamp_div(bool positive, int64_t a, int64_t b) -> int {
    if (a == std::numeric_limits<int64_t>::min() && b == -1) {
        return std::numeric_limits<int>::max();
    }
    auto c = div<int64_t>(positive, a, b);
    if (c > std::numeric_limits<int>::max()) {
        return std::numeric_limits<int>::max();
    }
    if (c < std::numeric_limits<int>::min()) {
        return std::numeric_limits<int>::min();
    }
    return static_cast<int>(c);
}

template <class T, class S> inline auto check_cast(S in, T &out) -> bool {
    if (sizeof(T) < sizeof(S) && (in < std::numeric_limits<T>::min() || in > std::numeric_limits<T>::max())) {
        return false;
    }
    out = static_cast<T>(in);
    return true;
}

template <class S> inline auto check_add(S a, S b, S &c) -> bool {
    using U = std::make_unsigned_t<S>;
    c = static_cast<S>(static_cast<U>(a) + static_cast<U>(b));
    return (a < 0 || b < 0 || c >= a) && (a >= 0 || b >= 0 || c <= a);
}

template <class S> inline auto check_sub(S a, S b, S &c) -> bool {
    using U = std::make_unsigned_t<S>;
    c = static_cast<S>(static_cast<U>(a) - static_cast<U>(b));
    return (a < 0 || b >= 0 || c >= a) && (b < 0 || c <= a);
}

template <class S> inline auto check_mul(S a, S b, S &c) -> bool {
#ifdef __GNUC__
    return !__builtin_mul_overflow(a, b, &c);
#else
    if (a > 0 && b > 0 && a > std::numeric_limits<S>::max() / b) {
        return false;
    }
    if (a < 0 && b < 0 && b < std::numeric_limits<S>::max() / a) {
        return false;
    }
    if (a > 0 && b < 0 && b < std::numeric_limits<S>::min() / a) {
        return false;
    }
    if (a < 0 && b > 0 && a < std::numeric_limits<S>::min() / b) {
        return false;
    }
    c = a * b;
    return true;
#endif
}

} // namespace

auto IESolver::update_bound_(IETerm const &term, int64_t slack, int num_unbounded) -> IESolver::UpdateResult {
    bool positive = term.coefficient > 0;
    auto type = positive ? IEBound::Upper : IEBound::Lower;
    if (num_unbounded == 0) {
        int64_t val = 0;
        if (!check_mul<int64_t>(term.coefficient, bounds_[term.variable].get(type), val)) {
            return UpdateResult::overflow;
        }
        if (!check_add(slack, val, slack)) {
            return UpdateResult::overflow;
        }
    } else if (num_unbounded > 1 || bounds_[term.variable].isSet(type)) {
        return UpdateResult::unchanged;
    }

    auto value = clamp_div(positive, slack, term.coefficient);
    if (bounds_[term.variable].refine(positive ? IEBound::Lower : IEBound::Upper, value)) {
        return UpdateResult::changed;
    }
    return UpdateResult::unchanged;
}

auto IESolver::update_slack_(IETerm const &term, int64_t &slack, int &num_unbounded) -> bool {
    auto type = term.coefficient > 0 ? IEBound::Upper : IEBound::Lower;
    if (bounds_[term.variable].isSet(type)) {
        int64_t val = 0;
        if (!check_mul<int64_t>(term.coefficient, bounds_[term.variable].get(type), val)) {
            return false;
        }
        if (!check_sub(slack, val, slack)) {
            return false;
        }
    } else {
        ++num_unbounded;
    }
    return true;
}

} // namespace Gringo::Input
