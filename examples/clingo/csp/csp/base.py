"""
Module for basic data types.
"""

from abc import abstractmethod

import clingo
from .util import ABC, abstractproperty


TRUE_LIT = 1


class ThreadStatistics(object):
    """
    Thread specific statistics.
    """
    def __init__(self):
        self.time_propagate = 0
        self.time_check = 0
        self.time_undo = 0
        self.refined_reason = 0
        self.introduced_reason = 0
        self.literals = 0

    def reset(self):
        """
        Reset all statistics to their starting values.
        """
        self.time_propagate = 0
        self.time_check = 0
        self.time_undo = 0
        self.refined_reason = 0
        self.introduced_reason = 0
        self.literals = 0

    def accu(self, stat):
        """
        Accumulate statistics in `stat`.
        """
        self.time_propagate += stat.time_propagate
        self.time_check += stat.time_check
        self.time_undo += stat.time_undo
        self.refined_reason += stat.refined_reason
        self.introduced_reason += stat.introduced_reason
        self.literals += stat.literals


class Statistics(object):
    """
    Propagator specific statistics.
    """
    def __init__(self):
        self.num_variables = 0
        self.num_constraints = 0
        self.num_clauses = 0
        self.num_literals = 0
        self.time_init = 0
        self.time_translate = 0
        self.time_simplify = 0
        self.translate_removed = 0
        self.translate_added = 0
        self.translate_clauses = 0
        self.translate_wcs = 0
        self.translate_literals = 0
        self.tstats = []

    def reset(self):
        """
        Reset all statistics to their starting values.
        """
        self.num_variables = 0
        self.num_constraints = 0
        self.num_clauses = 0
        self.num_literals = 0
        self.time_init = 0
        self.time_translate = 0
        self.time_simplify = 0
        self.translate_removed = 0
        self.translate_added = 0
        self.translate_clauses = 0
        self.translate_wcs = 0
        self.translate_literals = 0
        for s in self.tstats:
            s.reset()

    def accu(self, stats):
        """
        Accumulate statistics in `stat`.
        """
        self.num_variables += stats.num_variables
        self.num_constraints += stats.num_constraints
        self.num_clauses += stats.num_clauses
        self.num_literals += stats.num_literals
        self.time_init += stats.time_init
        self.time_translate += stats.time_translate
        self.time_simplify += stats.time_simplify
        self.translate_removed += stats.translate_removed
        self.translate_added += stats.translate_added
        self.translate_clauses += stats.translate_clauses
        self.translate_wcs += stats.translate_wcs
        self.translate_literals += stats.translate_literals

        for _ in range(len(self.tstats), len(stats.tstats)):
            self.tstats.append(ThreadStatistics())
        for stats_a, stats_b in zip(self.tstats, stats.tstats):
            stats_a.accu(stats_b)


class StateConfig(object):
    """
    Per state configuration.
    """
    def __init__(self):
        self.propagate_chain = True
        self.refine_reasons = True
        self.refine_introduce = True
        self.split_all = False

    def copy(self):
        """
        Copy the config.
        """
        cfg = StateConfig()
        cfg.propagate_chain = self.propagate_chain
        cfg.refine_reasons = self.refine_reasons
        cfg.refine_introduce = self.refine_introduce
        cfg.split_all = self.split_all
        return cfg


class Config(object):
    """
    Global configuration.
    """
    def __init__(self):
        self.min_int = -(2**32)
        self.max_int = 2**32
        self.sort_constraints = clingo.Flag(True)
        self.clause_limit = 1000
        self.literals_only = clingo.Flag(False)
        self.weight_constraint_limit = 0
        self.distinct_limit = 1000
        self.check_solution = clingo.Flag(True)
        self.check_state = clingo.Flag(False)
        self.translate_minimize = clingo.Flag(False)
        self.default_state_config = StateConfig()
        self.states = []

    def state_config(self, thread_id):
        """
        Get state specific configuration.
        """
        while len(self.states) <= thread_id:
            self.states.append(self.default_state_config.copy())
        return self.states[thread_id]


class AbstractClauseCreator(ABC):
    """
    Class to add solver literals, create clauses, and access the current
    assignment.
    """

    @abstractmethod
    def add_literal(self):
        """
        Add a new solver literal.
        """

    @abstractmethod
    def add_watch(self, lit):
        """
        Watch the given solver literal.
        """

    @abstractmethod
    def propagate(self):
        """
        Call unit propagation on the solver.
        """

    @abstractmethod
    def add_clause(self, clause, tag=False, lock=False):
        """
        Add the given clause to the sovler.

        If tag is True, the clause applies only in the current solving step. If
        lock is True, the clause is excluded from the from clause deletion.
        """

    @abstractproperty
    def assignment(self):
        """
        Get the assignment.
        """


class InitClauseCreator(AbstractClauseCreator):
    """
    Class to add solver literals, create clauses, access the current
    assignment, and further methods, using the `PropagateInit` object.
    """
    StateInit = 0
    StateTranslate = 1

    def __init__(self, init, stats):
        self._solver = init
        self._clauses = []
        self._weight_constraints = []
        self._minimize = []
        self._state = InitClauseCreator.StateInit
        self._stats = stats

    def set_state(self, state):
        """
        Set the state to log either init literals or additionally translation
        literals.
        """
        self._state = state

    @property
    def assignment(self):
        """
        Get the assignment.
        """
        return self._solver.assignment

    def solver_literal(self, literal):
        """
        Map the literal to a solver literal.
        """
        return self._solver.solver_literal(literal)

    def add_literal(self):
        """
        Add a new literal.
        """
        x = self._solver.add_literal()
        self._stats.num_literals += 1
        if self._state == InitClauseCreator.StateTranslate:
            self._stats.translate_literals += 1
        return x

    def add_watch(self, lit):
        """
        Watch the given solver literal.
        """
        self._solver.add_watch(lit)

    def propagate(self):
        """
        Call unit propagation on the solver.
        """
        return self.commit() and self._solver.propagate()

    def add_clause(self, clause, tag=False, lock=False):
        """
        Add the given `clause` to the sovler.

        If tag is True, the clause applies only in the current solving step.
        Parameter `lock` is ignored as clauses added with the init object are
        problem clauses.
        """
        # pylint: disable=unused-argument
        assert not tag
        self._stats.num_clauses += 1
        if self._state == InitClauseCreator.StateTranslate:
            self._stats.translate_clauses += 1

        self._clauses.append(clause[:])
        return True

    def add_weight_constraint(self, lit, wlits, bound, type_):
        """
        Add a weight constraint of form `lit == (wlits <= bound)`.
        """
        # Note: The hope is that all this business here can be taken care off
        # by clasp.
        if self.assignment.is_true(lit):
            if type_ < 0:
                return True
        elif self.assignment.is_false(lit):
            if type_ > 0:
                return True

        if self._state == InitClauseCreator.StateTranslate:
            self._stats.translate_wcs += 1
        self._weight_constraints.append((lit, wlits[:], bound, type_))
        return True

    def add_minimize(self, lit, weight, level):
        """
        Add a literal to the objective function.
        """
        self._minimize.append((lit, weight, level))

    def commit(self):
        """
        Commit accumulated constraints.
        """
        for clause in self._clauses:
            if not self._solver.add_clause(clause):
                return False
        del self._clauses[:]

        for lit, wlits, bound, type_ in self._weight_constraints:
            if not self._solver.add_weight_constraint(-lit, wlits, bound+1, -type_):
                return False
        del self._weight_constraints[:]

        for lit, weight, level in self._minimize:
            self._solver.add_minimize(lit, weight, level)
        del self._minimize[:]

        return True


class ControlClauseCreator(AbstractClauseCreator):
    """
    Class to add solver literals, create clauses, and access the current
    assignment using the `PropagateControl` object.
    """

    def __init__(self, control, stats):
        self._solver = control
        self._stats = stats

    def add_literal(self):
        """
        Adds a new literal.
        """
        self._stats.literals += 1
        return self._solver.add_literal()

    def add_watch(self, lit):
        """
        Watch the given solver literal.
        """
        self._solver.add_watch(lit)

    def propagate(self):
        """
        Call unit propagation on the solver.
        """
        return self._solver.propagate()

    def add_clause(self, clause, tag=False, lock=False):
        """
        Add the given clause to the sovler.

        If tag is True, the clause applies only in the current solving step. If
        lock is True, the clause is excluded from the from clause deletion.
        """
        return self._solver.add_clause(clause, tag=tag, lock=lock) and self.propagate()

    @property
    def assignment(self):
        """
        Get the assignment.
        """
        return self._solver.assignment
