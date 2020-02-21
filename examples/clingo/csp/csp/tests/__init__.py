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
        self.minint = minint
        self.maxint = maxint
        self.prp = csp.Propagator()
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
        self.prp.config.min_int = self.minint
        self.prp.config.max_int = self.maxint
        csp.CHECK_STATE = True
        csp.CHECK_SOLUTION = True
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
        return solver.solve("", False, solver.bound)
    return ret
