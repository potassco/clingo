#ifndef GRINGO_INPUT_IESOLVER
#define GRINGO_INPUT_IESOLVER

#include <cstdlib>
#include <vector>
#include <map>
#include <iostream>
#include <gringo/term.hh>

namespace Gringo {

class VarTerm;

namespace Input {

struct IETerm {
    int coefficient{0};
    VarTerm const *variable{nullptr};
};
using IETermVec = std::vector<IETerm>;

struct IE {
    IETermVec terms;
    int bound;
};
using IEVec = std::vector<IE>;

class IEBound {
public:
    enum Type { Lower, Upper };
    bool hasBound(Type type) const {
        return type == Lower ? hasLower_ : hasUpper_;
    }

    int bound(Type type) const {
        return type == Lower ? lower_ : upper_;
    }

    void setBound(Type type, int bound) {
        if (type == Lower) {
            hasLower_ = true;
            lower_ = bound;
        }
        else {
            hasUpper_ = true;
            upper_ = bound;
        }
    }

    bool refineBound(Type type, int bound) {
        if (!hasBound(type)) {
            setBound(type, bound);
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

private:
    int lower_{0};
    int upper_{0};
    bool hasLower_{false};
    bool hasUpper_{false};

};
using IEBoundMap = std::map<VarTerm const *, IEBound>;

class IESolver {
public:
    void add(IE ie) {
        ies_.emplace_back(std::move(ie));
    }

    IEBoundMap const *compute_bounds() {
        bounds_.clear();
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto const &ie : ies_) {
                int slack = ie.bound;
                int num_unbounded = 0;
                for (auto const &term : ie.terms) {
                    update_slack_(term.coefficient > 0, term, slack, num_unbounded);
                }
                if (num_unbounded == 0 && slack > 0) {
                    return nullptr;
                }
                if (num_unbounded <= 1) {
                    for (auto const &term : ie.terms) {
                        changed = update_bound_(term.coefficient > 0, slack, num_unbounded, term) || changed;
                    }
                }
            }
        }
        return &bounds_;
    }

private:
    template<typename I>
    static I floordiv_(I n, I m) {
        using std::div;
        auto a = div(n, m);
        if (((n < 0) ^ (m < 0)) && a.rem != 0) {
            a.quot--;
        }
        return a.quot;
    }

    template<typename I>
    static I ceildiv_(I n, I m) {
        using std::div;
        auto a = div(n, m);
        if (((n < 0) ^ (m < 0)) && a.rem != 0) {
            a.quot++;
        }
        return a.quot;
    }

    static int div_(bool positive, int a, int b) {
        return positive ? ceildiv_(a, b) : floordiv_(a, b);
    }

    bool update_bound_(bool positive, int slack, int num_unbounded, IETerm const &term) {
        auto type = positive ? IEBound::Upper : IEBound::Lower;
        if (num_unbounded == 0) {
            slack += term.coefficient * bounds_[term.variable].bound(type);
        }
        else if (num_unbounded > 1 || bounds_[term.variable].hasBound(type)) {
            return false;
        }

        auto value = div_(positive, slack, term.coefficient);
        return bounds_[term.variable].refineBound(positive ? IEBound::Lower : IEBound::Upper, value);
    }

    void update_slack_(bool positive, IETerm const &term, int &slack, int &num_unbounded) {
        auto type = positive ? IEBound::Upper : IEBound::Lower;
        if (bounds_[term.variable].hasBound(type)) {
            slack -= term.coefficient * bounds_[term.variable].bound(type);
        }
        else {
            ++num_unbounded;
        }
    }

    IEBoundMap bounds_;
    IEVec ies_;
};

} } // namespace Input Gringo

#endif // GRINGO_INPUT_IESOLVER
