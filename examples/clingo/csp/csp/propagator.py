"""
Propagator for CSP constraints.
"""

from collections import OrderedDict

import clingo
from .parsing import AbstractConstraintBuilder, simplify, parse_theory
from .util import measure_time_decorator, IntervalSet
from .base import Config, Statistics, InitClauseCreator, ControlClauseCreator
from .solver import State
from .constraints import SumConstraint, DistinctConstraint, MinimizeConstraint


class ConstraintBuilder(AbstractConstraintBuilder):
    """
    CSP builder to use with the parse_theory function.
    """
    def __init__(self, cc, propagator, minimize):
        self._cc = cc
        self._propagator = propagator
        self._minimize = minimize

    @property
    def cc(self):
        """
        Return a ClauseCreator.
        """
        return self._cc

    def add_show(self):
        """
        Inform the builder that there is a show statement.
        """
        self._propagator.show()

    def show_signature(self, name, arity):
        """
        Show variables with the given signature.
        """
        self._propagator.show_signature(name, arity)

    def show_variable(self, var):
        """
        Show the given variable.
        """
        self._propagator.show_variable(var)

    def add_variable(self, var):
        """
        Get the integer representing a variable.
        """
        assert isinstance(var, clingo.Symbol)
        return self._propagator.add_variable(var)

    def add_constraint(self, lit, elems, rhs, strict):
        """
        Add a constraint.
        """
        if not strict and self.cc.assignment.is_false(lit):
            return

        if len(elems) == 1:
            co, var = elems[0]
            self._propagator.add_simple(self.cc, lit, co, var, rhs, strict)
        else:
            assert not strict
            if self._propagator.config.sort_constraints:
                elems.sort(key=lambda cv: -abs(cv[0]))
            self._propagator.add_constraint(self.cc, SumConstraint(lit, elems, rhs))

    def add_minimize(self, co, var):
        """
        Add a term to the minimize constraint.
        """
        if self._minimize is None:
            self._minimize = MinimizeConstraint()

        if co == 0:
            return

        self._minimize.elements.append((co, var))

    def add_distinct(self, literal, elems):
        """
        Add a distinct constraint.

        Binary distinct constraints will be represented with a sum constraint.
        """
        if self.cc.assignment.is_false(literal):
            return

        if len(elems) > 2:
            self._propagator.add_constraint(self.cc, DistinctConstraint(literal, elems))
            return

        for i, (rhs_i, elems_i) in enumerate(elems):
            for rhs_j, elems_j in elems[i+1:]:
                rhs = rhs_i - rhs_j

                celems = []
                celems.extend(elems_i)
                celems.extend((-co_j, var_j) for co_j, var_j in elems_j)

                if not celems:
                    if rhs == 0:
                        self.cc.add_clause([-literal])
                        return
                    continue

                a = self.cc.add_literal()
                b = self.cc.add_literal()

                self.cc.add_clause([a, b, -literal])
                self.cc.add_clause([-a, -b])

                self.add_constraint(a, celems, rhs-1, False)
                self.add_constraint(b, [(-co, var) for co, var in celems], -rhs-1, False)

    def add_dom(self, literal, var, elements):
        """
        Add a domain for the given variable.

        The domain is represented as a set of left-closed intervals.
        """
        if self.cc.assignment.is_false(literal):
            return

        intervals = IntervalSet(elements)
        self._propagator.add_dom(self.cc, literal, var, list(intervals))

    def prepare_minimize(self):
        """
        Prepare the minimize constraint.
        """

        # simplify minimize
        if self._minimize is not None:
            adjust, self._minimize.elements = simplify(self._minimize.elements, True)
            self._minimize.adjust += adjust
            if self._propagator.config.sort_constraints:
                self._minimize.elements.sort(key=lambda cv: -abs(cv[0]))

        return self._minimize


class Propagator(object):
    """
    A propagator for CSP constraints.
    """
    def __init__(self):
        self._l2c = {}                     # map literals to constraints
        self._states = []                  # map thread id to states
        self._var_map = OrderedDict()      # map from variable names to indices
        self._minimize = None              # minimize constraint
        self._minimize_bound = None        # bound of the minimize constraint
        self._stats_step = Statistics()    # statistics of the current call
        self._stats_accu = Statistics()    # accumulated statistics
        self._translated_minimize = False  # whether a minimize constraint has been translated
        self.config = Config()             # configuration
        self._show = False                 # whether there is a show statement
        self._show_variable = set()        # variables to show
        self._show_signature = set()       # signatures to show

    def _state(self, thread_id):
        """
        Get the state associated with the given `thread_id`.
        """
        while len(self._states) <= thread_id:
            self._states.append(State(self._l2c, self.config.state_config(thread_id)))
        return self._states[thread_id]

    @property
    def statistics(self):
        """
        Return statistics object.
        """
        return self._stats_step

    def on_statistics(self, step, akku):
        """
        Callback to update `step` and `akku`mulated statistics.
        """
        for s in self._states:
            self._stats_step.tstats.append(s.statistics)
        self._stats_accu.accu(self._stats_step)
        self.add_statistics(step, self._stats_step)
        self.add_statistics(akku, self._stats_accu)
        self._stats_step.reset()

    def add_statistics(self, stats_map, stats):
        """
        Add collected statistics in `stats` to the clingo.StatisticsMap `stats_map`.
        """
        def thread_stats(tstat):  # pylint: disable=missing-docstring
            p, c, u = tstat.time_propagate, tstat.time_check, tstat.time_undo
            return OrderedDict([
                ("Time in seconds", OrderedDict([
                    ("Total", p+c+u),
                    ("Propagation", p),
                    ("Check", c),
                    ("Undo", u)])),
                ("Refined reason", tstat.refined_reason),
                ("Introduced reason", tstat.introduced_reason),
                ("Literals introduced ", tstat.literals)])
        stats_map["Clingcon"] = OrderedDict([
            ("Init time in seconds", OrderedDict([
                ("Total", stats.time_init),
                ("Simplify", stats.time_simplify),
                ("Translate", stats.time_translate)])),
            ("Problem", OrderedDict([
                ("Constraints", stats.num_constraints),
                ("Variables", stats.num_variables),
                ("Clauses", stats.num_clauses),
                ("Literals", stats.num_literals)])),
            ("Translate", OrderedDict([
                ("Constraints removed", stats.translate_removed),
                ("Constraints added", stats.translate_added),
                ("Clauses", stats.translate_clauses),
                ("Weight constraints", stats.translate_wcs),
                ("Literals", stats.translate_literals)])),
            ("Thread", map(thread_stats, stats.tstats[:len(self._states)]))])

    def add_variable(self, var):
        """
        Add a variable to the program.
        """
        assert isinstance(var, clingo.Symbol)
        if var not in self._var_map:
            idx = self._state(0).add_variable(self.config.min_int, self.config.max_int)
            self._var_map[var] = idx
            self._stats_step.num_variables += 1
        return self._var_map[var]

    def show(self):
        """
        Enable show statement.

        If the show statement has not been enabled, then all variables are
        shown.
        """
        self._show = True

    def show_variable(self, var):
        """
        Show the given variable.
        """
        self._show_variable.add(var)

    def show_signature(self, name, arity):
        """
        Show variables with the given signature.
        """
        self._show_signature.add((name, arity))

    def add_dom(self, cc, literal, var, domain):
        """
        Add a domain for the given variable.
        """
        return self._state(0).add_dom(cc, literal, var, domain)

    def add_simple(self, cc, clit, co, var, rhs, strict):
        """
        Add a constraint that can be represented by an order literal.
        """
        return self._state(0).add_simple(cc, clit, co, var, rhs, strict)

    def _add_constraint(self, cc, constraint):
        """
        Add a constraint to the program that has already been added to the
        master state.
        """
        lit = constraint.literal
        cc.add_watch(lit)
        self._l2c.setdefault(lit, []).append(constraint)

    def add_constraint(self, cc, constraint):
        """
        Add a constraint to the program.
        """
        self._state(0).add_constraint(constraint)
        self._stats_step.num_constraints += 1
        self._add_constraint(cc, constraint)

    @measure_time_decorator("statistics.time_init")
    def init(self, init):
        """
        Initializes the propagator extracting constraints from the theory data.

        The function handles reinitialization for multi-shot solving and
        multi-threaded solving.
        """
        init.check_mode = clingo.PropagatorCheckMode.Fixpoint
        cc = InitClauseCreator(init, self.statistics)

        # remove minimize constraint
        minimize = self.remove_minimize()

        # remove solve step local and fixed literals
        for state in self._states:
            state.update(cc)

        # add constraints
        builder = ConstraintBuilder(cc, self, minimize)
        parse_theory(builder, init.theory_atoms)

        # gather bounds of states in master
        master = self._state(0)
        for state in self._states[1:]:
            if not master.update_bounds(cc, state):
                return

        # propagate the newly added constraints
        if not self._simplify(cc, master):
            return

        # remove unnecessary literals after simplification
        if not master.cleanup_literals(cc):
            return

        # translate (simple enough) constraints
        if not self._translate(cc, master, builder.prepare_minimize()):
            return

        # copy order literals from master to other states
        del self._states[init.number_of_threads:]
        for i in range(1, init.number_of_threads):
            self._state(i).copy_state(master)

    @measure_time_decorator("statistics.time_simplify")
    def _simplify(self, cc, master):
        """
        Propagate constraints refining bounds.
        """
        try:
            return master.simplify(cc, self.config.check_state)
        finally:
            # Note: During simplify propagate and check are called, which can
            # produce large timings for the master thread.
            master.statistics.time_propagate = 0
            master.statistics.time_check = 0

    @measure_time_decorator("statistics.time_translate")
    def _translate(self, cc, master, minimize):
        """
        Translates constraints and take care of handling the minimize
        constraint.
        """
        # add minimize constraint
        # Note: the minimize constraint is added after simplification to avoid
        # propagating tagged clauses, which is not supported at the moment.
        if minimize is not None:
            # Note: fail if translation was requested earlier
            if self._translated_minimize and not self.config.translate_minimize:
                raise RuntimeError("translation of minimize constraints is disabled but was enabled before")
            self.add_minimize(cc, minimize)

        # translate (simple enough) constraints
        cc.set_state(InitClauseCreator.StateTranslate)
        ret, added = master.translate(cc, self._l2c, self.statistics, self.config)
        if not ret:
            return False
        for constraint in added:
            self._add_constraint(cc, constraint)
        cc.set_state(InitClauseCreator.StateInit)

        # mark minimize constraint as translated if necessary
        if self.config.translate_minimize and self._minimize is not None:
            self._translated_minimize = True
            self._minimize = None

        return True

    def propagate(self, control, changes):
        """
        Delegates propagation to the respective state.
        """
        state = self._state(control.thread_id)
        state.propagate(ControlClauseCreator(control, state.statistics), changes)

    def check(self, control):
        """
        Delegates checking to the respective state and makes sure that all
        order variables are assigned if the assigment is total.
        """
        size = len(control.assignment)
        state = self._state(control.thread_id)
        dl = control.assignment.decision_level
        if self.has_minimize and self._minimize_bound is not None:
            bound = self._minimize_bound + self._minimize.adjust
            state.update_minimize(self._minimize, dl, bound)

        if not state.check(ControlClauseCreator(control, state.statistics), self.config.check_state):
            return

        # Note: Makes sure that all variables are assigned in the end. But even
        # if the assignment is total, we do not have to introduce fresh
        # variables if variables have been introduced during check. In this
        # case, there is a guaranteed follow-up propagate call because all
        # newly introduced variables are watched.
        if size == len(control.assignment) and control.assignment.is_total:
            state.check_full(control, self.config.check_solution)

    def undo(self, thread_id, assign, changes):
        # pylint: disable=unused-argument
        """
        Delegates undoing to the respective state.
        """
        self._state(thread_id).undo()

    def _shown(self, var):
        """
        Determine if the given variable should be shown.
        """
        if not self._show:
            return True

        if var in self._show_variable:
            return True

        if var.type == clingo.SymbolType.Function and (var.name, len(var.arguments)) in self._show_signature:
            return True

        return False

    def get_assignment(self, thread_id, show_all=False):
        """
        Get the assigment from the state associated with `thread_id`.

        Should be called on total assignments.
        """
        return self._state(thread_id).get_assignment((var, idx) for var, idx in self._var_map.items() if show_all or self._shown(var))

    def get_value(self, var, thread_id):
        """
        Get the value of the given variable in the state associated with
        `thread_id`.

        Should be called on total assignments.
        """
        return self._state(thread_id).get_value(self._var_map[var])

    @property
    def has_minimize(self):
        """
        Check if the propagator has a minimize constraint.
        """
        return self._minimize is not None

    def add_minimize(self, cc, minimize):
        """
        Add a minimize constraint to the program.
        """
        self._minimize = minimize
        self.add_constraint(cc, minimize)

    def remove_minimize(self):
        """
        Removes the minimize constraint from the lookup lists.
        """
        minimize, self._minimize = self._minimize, None
        if minimize is not None:
            lit = minimize.literal
            self._l2c[lit].remove(minimize)
            self._state(0).remove_constraint(minimize)
            self._stats_step.num_constraints -= 1
        return minimize

    def get_minimize_value(self, thread_id):
        """
        Evaluates the minimize constraint w.r.t. the given thread.

        Should be called on total assignments.
        """
        assert self.has_minimize
        bound = 0
        for co, var in self._minimize.elements:
            bound += co * self._state(thread_id).get_value(var)
        return bound - self._minimize.adjust

    def update_minimize(self, bound):
        """
        Set the `bound` of the minimize constraint.
        """
        assert self.has_minimize
        self._minimize_bound = bound
