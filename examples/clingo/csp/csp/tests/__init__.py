"""
Basic functions to run tests.
"""

import collections
import clingo
from csp import Propagator, transform, THEORY


ConfGlobalEntry = collections.namedtuple(
    'ConfGlobalEntry',
    ['distinct_limit',
     'weight_constraint_limit',
     'clause_limit',
     'literals_only',
     'sort_constraints',
     'translate_minimize'])
CONF_GLOBAL = [
    ConfGlobalEntry(0, 0, 0, False, False, False),       # basic
    ConfGlobalEntry(0, 0, 0, False, True, False),        # sort constraints
    ConfGlobalEntry(1000, 1000, 0, False, False, True),  # translate
    ConfGlobalEntry(1000, 1000, 0, True, False, True),   # translate literals only
    ConfGlobalEntry(1000, 0, 1000, False, False, True)]  # translate to weight constraints

ConfLocalEntry = collections.namedtuple(
    'ConfLocalEntry',
    ['refine_reasons',
     'refine_introduce',
     'propagate_chain'])
# Note: this should simply contain the opposite of the default config
CONF_LOCAL = [ConfLocalEntry(False, False, False)]


class Solver(object):
    """
    Simplistic solver for multi-shot solving.
    """
    def __init__(self, minint=-20, maxint=20, threads=8, options=()):
        self.prp = Propagator()
        self.prp.config.min_int = minint
        self.prp.config.max_int = maxint
        self.prp.config.check_solution = True
        self.prp.config.check_state = True
        self.prg = clingo.Control(['0', '-t', str(threads)] + list(options), message_limit=0)
        self.step = 0
        self.optimize = False
        self.bound = None

        self.prg.register_propagator(self.prp)
        self.prg.add("base", [], THEORY)

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

        if model.cost:
            self.bound = model.cost[0]

        return sorted(m), sorted(a)

    def solve(self, s, optimize=True, bound=None):
        """
        Extend the current program with the program in the given string and
        then return its models in sorted list.
        """
        # pylint: disable=unsubscriptable-object,cell-var-from-loop,no-member

        step = "step{}".format(self.step)

        with self.prg.builder() as b:
            transform(b, "#program {}.\n{}".format(step, s), True)
        self.prg.ground([(step, [])])

        self.bound = bound
        old_mode = None
        if bound is not None:
            if self.prp.config.translate_minimize:
                old_mode = self.prg.configuration.solve.opt_mode
                mode = "opt" if optimize else "enum"
                self.prg.configuration.solve.opt_mode = "{},{}".format(mode, bound)
            else:
                self.prp.update_minimize(bound)

        ret = []
        self.prg.solve(on_model=lambda m: ret.append(self._parse_model(m, optimize)))
        ret.sort()

        opt = self.prg.statistics["summary"]["models"]["optimal"]
        if not self.prp.has_minimize and not opt == 1:
            for conf in CONF_LOCAL:
                self.prp.config.default_state_config.refine_reasons = conf.refine_reasons
                self.prp.config.default_state_config.refine_introduce = conf.refine_introduce
                self.prp.config.default_state_config.propagate_chain = conf.propagate_chain
                self.prp.config.threads = []
                ret_alt = []
                self.prg.solve(on_model=lambda m: ret_alt.append(self._parse_model(m, optimize)))
                ret_alt.sort()
                assert ret == ret_alt

        if old_mode is not None:
            self.prg.configuration.solve.opt_mode = mode

        self.step += 1
        return [m + a for m, a in ret]


def _solve(solver, s):
    ret = solver.solve(s)
    if solver.bound is not None:
        ret = solver.solve("", False, solver.bound)
    return ret


def solve(s, minint=-20, maxint=20, threads=8, options=()):
    """
    Return the (optimal) models/assignments of the program in the given string.
    """
    solver = Solver(minint, maxint, threads, options)
    ret = _solve(solver, s)

    has_minimize = solver.prp.has_minimize

    for conf in CONF_GLOBAL:
        if conf.translate_minimize and not has_minimize:
            continue
        solver = Solver(minint, maxint, threads, options)
        solver.prp.config.weight_constraint_limit = conf.weight_constraint_limit
        solver.prp.config.clause_limit = conf.clause_limit
        solver.prp.config.literals_only.value = conf.literals_only
        solver.prp.config.sort_constraints.value = conf.sort_constraints
        solver.prp.config.translate_minimize.value = conf.translate_minimize
        ret_alt = _solve(solver, s)
        msg = "weight_constraint_limit={}, clause_limit={}, literals_only={}, sort_constraints={}, translate_minimize={}".format(
            conf.weight_constraint_limit, conf.clause_limit, conf.literals_only, conf.sort_constraints, conf.translate_minimize)
        assert ret == ret_alt, msg

    return ret
