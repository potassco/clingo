"""
Basic functions to run tests.
"""

import clingo
import csp
from csp.parsing import transform


class Solver(object):
    """
    Simplistic solver for multi-shot solving.
    """
    def __init__(self, minint=-20, maxint=20, threads=8, options=()):
        self.prp = csp.Propagator()
        self.prp.config.min_int = minint
        self.prp.config.max_int = maxint
        self.prp.config.check_solution = True
        self.prp.config.check_state = True
        self.prg = clingo.Control(['0', '-t', str(threads)] + list(options), message_limit=0)
        self.step = 0
        self.optimize = False
        self.bound = None

        self.prg.register_propagator(self.prp)
        self.prg.add("base", [], csp.THEORY)

    def _parse_model(self, model, optimize):
        """
        Combine model and assignment in one list.
        """
        m = []
        for sym in model.symbols(shown=True):
            s = str(sym)
            if not s.startswith("_"):
                m.append(s)

        a = []
        for var, val in self.prp.get_assignment(model.thread_id):
            var = str(var)
            if not var.startswith("_"):
                a.append((var, val))

        if self.prp.has_minimize and optimize:
            assert self.prp.has_minimize
            value = self.prp.get_minimize_value(model.thread_id)
            if self.bound is None or self.bound > value:
                self.bound = value
                self.prp.update_minimize(self.bound-1)

        return list(sorted(m)), list(sorted(a))

    def solve(self, s, optimize=True, bound=None):
        """
        Extend the current program with the program in the given string and
        then return its models in sorted list.
        """
        step = "step{}".format(self.step)

        with self.prg.builder() as b:
            transform(b, "#program {}.\n{}".format(step, s), True)
        self.prg.ground([(step, [])])

        self.bound = bound
        if bound is not None:
            self.prp.update_minimize(bound)

        ret = []
        self.prg.solve(on_model=lambda m: ret.append(self._parse_model(m, optimize)))

        self.step += 1
        return [a + b for a, b in list(sorted(ret))]


def solve(s, minint=-20, maxint=20, threads=8, options=()):
    """
    Return the (optimal) models/assignments of the program in the given string.
    """
    solver = Solver(minint, maxint, threads, options)
    ret = solver.solve(s)
    if solver.bound is not None:
        ret = solver.solve("", False, solver.bound)

    budgets = ((0, 0, False), (1000, 0, False), (0, 1000, True), (0, 1000, False))
    for weight_constraint_limit, clause_limit, literals_only in budgets:
        for sort_constraints in (True, False):
            for propagate_chain in (True, False):
                for refine_introduce in (True, False):
                    for refine_reasons in (True, False):
                        solver = Solver(minint, maxint, threads, options)
                        solver.prp.config.weight_constraint_limit = weight_constraint_limit
                        solver.prp.config.clause_limit = clause_limit
                        solver.prp.config.literals_only = literals_only
                        solver.prp.config.sort_constraints = sort_constraints
                        solver.prp.config.default_state_config.refine_reasons = refine_reasons
                        solver.prp.config.default_state_config.refine_introduce = refine_introduce
                        solver.prp.config.default_state_config.propagate_chain = propagate_chain
                        ret_alt = solver.solve(s)
                        if solver.bound is not None:
                            ret_alt = solver.solve("", False, solver.bound)
                        msg = "weight_constraint_limit={}, clause_limit={}, literals_only={}, sort_constraints={}, propagate_chain={}, refine_introduce={}, refine_reasons={}".format(
                            weight_constraint_limit, clause_limit, literals_only, sort_constraints, propagate_chain, refine_introduce, refine_reasons)
                        assert set(frozenset(m) for m in ret) == set(frozenset(m) for m in ret_alt), msg

    return ret
