"""
Basic functions to run tests.
"""

import clingo
import csp
from csp.parsing import transform


def parse_model(model, prp):
    """
    Combine model and assignment in one list.
    """
    m = []
    for sym in model.symbols(shown=True):
        m.append(str(sym))
    a = []
    for var, val in prp.get_assignment(model.thread_id):
        var = str(var)
        if not var.startswith("_"):
            a.append((var, val))

    return list(sorted(m)), list(sorted(a))


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

        self.prg.register_propagator(self.prp)
        self.prg.add("base", [], csp.THEORY)

    def solve(self, s):
        """
        Extend the current program with the program in the given string and
        then return its models.
        """
        csp.MIN_INT = self.minint
        csp.MAX_INT = self.maxint
        step = "step{}".format(self.step)

        with self.prg.builder() as b:
            transform(b, "#program {}.\n{}".format(step, s))
        self.prg.ground([(step, [])])

        ret = []
        self.prg.solve(on_model=lambda m: ret.append(parse_model(m, self.prp)))

        self.step += 1

        return [a + b for a, b in list(sorted(ret))]


def solve(s, minint=-20, maxint=20, threads=8, options=()):
    """
    Return the models/assignments of the program in the given string.
    """
    return Solver(minint, maxint, threads, options).solve(s)
