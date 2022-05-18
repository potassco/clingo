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

template<typename I>
I floordiv(I n, I m) {
    using std::div;
    auto a = div(n, m);
    if (((n < 0) ^ (m < 0)) && a.rem != 0) {
        a.quot--;
    }
    return a.quot;
}

template<typename I>
I ceildiv(I n, I m) {
    using std::div;
    auto a = div(n, m);
    if (((n < 0) ^ (m < 0)) && a.rem != 0) {
        a.quot++;
    }
    return a.quot;
}

class IEBound {
public:
    template <bool lower>
    bool hasBound() {
        return lower ? hasLower_ : hasUpper_;
    }

    template <bool lower>
    int bound() {
        return lower ? lower_ : upper_;
    }

    template <bool lower>
    void setBound(int bound) {
        if (lower) {
            std::cerr << "  lower=" << bound << std::endl;
            hasLower_ = true;
            lower_ = bound;
        }
        else {
            std::cerr << "  upper=" << bound << std::endl;
            hasUpper_ = true;
            upper_ = bound;
        }
    }

    template <bool lower>
    bool refineBound(int bound) {
        if (!hasBound<lower>()) {
            setBound<lower>(bound);
            return true;
        }
        if (lower && bound > lower_) {
            std::cerr << "  lower=" << bound << std::endl;
            lower_ = bound;
            return true;
        }
        if (!lower && bound < upper_) {
            std::cerr << "  upper=" << bound << std::endl;
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

/*
TODO: compare carefully
from typing import Callable, Dict, List, Optional, Tuple
from dataclasses import dataclass


@dataclass
class Term:
    coefficient: int
    variable: str

    def __str__(self) -> str:
        if self.coefficient == 1:
            return self.variable
        if self.coefficient == -1:
            return f'-{self.variable}'
        return f'{self.coefficient}*{self.variable}'


@dataclass
class Inequality:
    terms: List[Term]
    bound: int

    def __str__(self) -> str:
        e = ' + '.join(str(t) for t in self.terms) if self.terms else '0'
        return f'{e} >= {self.bound}'


def floor_div(a: int, b: int) -> int:
    return a // b


def ceil_div(a: int, b: int) -> int:
    return (a - 1) // b + 1


def update_bound(lb: Dict[str, int], ub: Dict[str, int], slack: int, num_unbounded: int,
                 select: Callable[[int, int], int], div: Callable[[int, int], int], term: Term) -> bool:
    if num_unbounded == 0:
        slack += term.coefficient * ub[term.variable]
    elif num_unbounded > 1 or term.variable in ub:
        return False

    value = div(slack, term.coefficient)
    if term.variable in lb:
        new_val = select(value, lb[term.variable])
        changed = new_val != lb[term.variable]
    else:
        new_val = value
        changed = True
    lb[term.variable] = new_val
    if changed:
        print(f"  bound={new_val}")

    return changed


def update_slack(lub: Dict[str, int], term: Term, slack: int, num_unbounded: int) -> Tuple[int, int]:
    if term.variable in lub:
        print(f"  slack[{term.variable}] -= {term.coefficient}  * {lub[term.variable]}")
        slack -= term.coefficient * lub[term.variable]
    else:
        num_unbounded += 1

    return slack, num_unbounded


def compute_bounds(iqs: List[Inequality]) -> Optional[Tuple[Dict[str, int], Dict[str, int]]]:
    lb: Dict[str, int] = dict()
    ub: Dict[str, int] = dict()

    changed = True
    while changed:
        print("step")
        changed = False
        for iq in iqs:
            # compute slack and number of unbounded terms
            slack, num_unbounded = iq.bound, 0
            for term in iq.terms:
                if term.coefficient > 0:
                    slack, num_unbounded = update_slack(ub, term, slack, num_unbounded)
                else:
                    slack, num_unbounded = update_slack(lb, term, slack, num_unbounded)

            print(f"  result: {num_unbounded}, {slack}")
            # the inequalities cannot be satisfied
            if num_unbounded == 0 and slack > 0:
                return None

            # propagate if there is at most one unbounded term
            if num_unbounded <= 1:
                for term in iq.terms:
                    if term.coefficient > 0:
                        changed = update_bound(lb, ub, slack, num_unbounded, max, ceil_div, term) or changed
                    else:
                        changed = update_bound(ub, lb, slack, num_unbounded, min, floor_div, term) or changed

    return lb, ub


def test():
    # X>1, Y>X, X+Y <= 100
    i1 = Inequality([Term(1, "X")], 2)
    i2 = Inequality([Term(1, "Y"), Term(-1, "X")], 1)
    i3 = Inequality([Term(-1, "X"), Term(-1, "Y")], -100)
    print('rule body:')
    print('  1<X<Y, X+Y<=100')
    print('inequalities:')
    print(f'  {i1}')
    print(f'  {i2}')
    print(f'  {i3}')
    res = compute_bounds([i1, i2, i3])
    if res:
        print("ranges:")
        lb, ub = res
        for var in sorted(lb):
            if var in ub:
                print(f'  {var} = {lb[var]}..{ub[var]}')
    else:
        print('the inequalities cannot be satisfied')

test()
*/

class IESolver {
private:
    using BoundMap = std::map<VarTerm const *, IEBound>;

public:
    void add(IE ie) {
        ies_.emplace_back(std::move(ie));
    }

    bool compute_bounds() {
        bool changed = true;
        while (changed) {
            std::cerr << "step" << std::endl;
            changed = false;
            for (auto const &ie : ies_) {
                // compute slack and number of unbounded terms
                int slack = ie.bound;
                int num_unbounded = 0;
                for (auto const &term : ie.terms) {
                    if (term.coefficient > 0) {
                        update_slack_<true>(term, slack, num_unbounded);
                    }
                    else {
                        update_slack_<false>(term, slack, num_unbounded);
                    }
                }
                std::cerr << "  result: " << num_unbounded << ", " << slack << std::endl;
                if (num_unbounded == 0 && slack > 0) {
                    return false;
                }
                if (num_unbounded <= 1) {
                    for (auto const &term : ie.terms) {
                        if (term.coefficient > 0) {
                            changed = update_bound_<true>(slack, num_unbounded, term) || changed;
                        }
                        else {
                            changed = update_bound_<false>(slack, num_unbounded, term) || changed;
                        }
                    }
                }

            }
        }
        return true;
    }

private:
    template <bool lower>
    bool div_(int a, int b) {
        // check lower/!lower
        return lower ? ceildiv(a, b) : floordiv(a, b);
    }

    template <bool lower>
    bool update_bound_(int slack, int num_unbounded, IETerm const &term) {
        if (num_unbounded == 0) {
            slack += term.coefficient * bounds_[term.variable].bound<!lower>();
        }
        else if (num_unbounded > 1 || bounds_[term.variable].hasBound<!lower>()) {
            return false;
        }

        auto value = div_<lower>(slack, term.coefficient);
        return bounds_[term.variable].refineBound<lower>(value);
    }

    template <bool lower>
    void update_slack_(IETerm const &term, int &slack, int &num_unbounded) {
        if (bounds_[term.variable].hasBound<!lower>()) {
            std::cerr << "  slack[" << term.variable->name << "] -= " <<  term.coefficient << " * " << bounds_[term.variable].bound<!lower>() << std::endl;
            slack -= term.coefficient * bounds_[term.variable].bound<!lower>();
        }
        else {
            ++num_unbounded;
        }
    }

    BoundMap bounds_;
    std::vector<IE> ies_;
};

} } // namespace Input Gringo

#endif // GRINGO_INPUT_IESOLVER
