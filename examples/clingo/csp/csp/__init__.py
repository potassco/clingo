"""
This module provides a propagator for CSP constraints. It can also be used as a
stand-alone application.
"""

from collections import OrderedDict
from itertools import chain
from timeit import default_timer as timer
import clingo
from .parsing import parse_theory, simplify
from .util import lerp, remove_if, TodoList, SortedDict, IntervalSet

CHECK_SOLUTION = True
CHECK_STATE = False
MAX_INT = 2**32
MIN_INT = -(2**32)
TRUE_LIT = 1
THEORY = """\
#theory cp {
    var_term  { };
    sum_term {
    -  : 3, unary;
    ** : 2, binary, right;
    *  : 1, binary, left;
    /  : 1, binary, left;
    \\ : 1, binary, left;
    +  : 0, binary, left;
    -  : 0, binary, left
    };
    dom_term {
    -  : 4, unary;
    ** : 3, binary, right;
    *  : 2, binary, left;
    /  : 2, binary, left;
    \\ : 2, binary, left;
    +  : 1, binary, left;
    -  : 1, binary, left;
    .. : 0, binary, left
    };
    &sum/1 : sum_term, {<=,=,!=,<,>,>=}, sum_term, any;
    &diff/1 : sum_term, {<=}, sum_term, any;
    &minimize/0 : sum_term, directive;
    &maximize/0 : sum_term, directive;
    &distinct/0 : sum_term, head;
    &dom/0 : dom_term, {=}, var_term, head
}.
"""


def measure_time(attribute):
    """
    Decorator to time function calls for propagator statistics.
    """
    def time_wrapper(func):
        def wrapper(self, *args, **kwargs):
            start = timer()
            ret = func(self, *args, **kwargs)
            value = getattr(self.statistics, attribute)
            setattr(self.statistics, attribute, value + timer() - start)
            return ret
        return wrapper
    return time_wrapper


class ThreadStatistics(object):
    """
    Thread specific statistics.
    """
    def __init__(self):
        self.time_propagate = 0
        self.time_check = 0
        self.time_undo = 0

    def reset(self):
        """
        Reset all statistics to they starting values.
        """
        self.time_propagate = 0
        self.time_check = 0
        self.time_undo = 0

    def accu(self, stat):
        """
        Accumulate statistics in `stat`.
        """
        self.time_propagate += stat.time_propagate
        self.time_check += stat.time_check
        self.time_undo += stat.time_undo


class Statistics(object):
    """
    Propagator specific statistics.
    """
    def __init__(self):
        self.num_variables = 0
        self.num_constraints = 0
        self.time_init = 0
        self.tstats = []

    def reset(self):
        """
        Reset all statistics to they starting values.
        """
        self.num_variables = 0
        self.num_constraints = 0
        self.time_init = 0
        for s in self.tstats:
            s.reset()

    def accu(self, stats):
        """
        Accumulate statistics in `stat`.
        """
        self.num_variables = max(self.num_variables, stats.num_variables)
        self.num_constraints = max(self.num_constraints, stats.num_constraints)
        self.time_init += stats.time_init

        for _ in range(len(self.tstats), len(stats.tstats)):
            self.tstats.append(ThreadStatistics())
        for stats_a, stats_b in zip(self.tstats, stats.tstats):
            stats_a.accu(stats_b)


class InitClauseCreator(object):
    """
    Class to add solver literals, create clauses, and access the current
    assignment using the `PropagateInit` object.
    """

    def __init__(self, init):
        self._solver = init

    def add_literal(self):
        """
        Adds a new literal.
        """
        x = self._solver.add_literal()
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
        return self._solver.propagate()

    def add_clause(self, clause, tag=False, lock=False):
        """
        Add the given `clause` to the sovler.

        If tag is True, the clause applies only in the current solving step.
        Parameter `lock` is ignored as clauses added with the init object are
        problem clauses.
        """
        # pylint: disable=unused-argument
        assert not tag

        return self._solver.add_clause(clause) and self.propagate()

    @property
    def assignment(self):
        """
        Get he assignment.
        """
        return self._solver.assignment


class ControlClauseCreator(object):
    """
    Class to add solver literals, create clauses, and access the current
    assignment using the `PropagateControl` object.
    """

    def __init__(self, control):
        self._solver = control

    def add_literal(self):
        """
        Adds a new literal.
        """
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


class Constraint(object):
    """
    Class to capture sum constraints of form `a_0*x_0 + ... + a_n * x_n <= rhs`.

    Members
    =======
    literal  -- Solver literal associated with the constraint.
    elements -- List of integer/string pairs representing coefficient and
                variable.
    rhs      -- Integer bound of the constraint.

    Class Variables
    ======
    tagged   -- True if constraint applies only during current solving step.
    """

    tagged = False

    def __init__(self, literal, elements, rhs):
        """
        Create a constraint and initialize all members.
        """
        self.literal = literal
        self.elements = elements
        self._rhs = rhs

    def rhs(self, state):  # pylint: disable=unused-argument
        """
        Returns the rhs.
        """
        return self._rhs

    def __str__(self):
        return "{} <= {}".format(self.elements, self._rhs)


class Minimize(object):
    """
    Class to capture minimize constraints of form `a_0*x_0 + ... + a_n * x_n <= rhs`.

    Members
    =======
    literal  -- Solver literal associated with the constraint.
    elements -- List of integer/string pairs representing coefficient and
                variable.

    Class Variables
    ======
    tagged   -- True if constraint applies only during current solving step.
    """

    tagged = True

    def __init__(self):
        self.literal = TRUE_LIT
        self.elements = []
        self.adjust = 0

    def rhs(self, state):
        """
        Returns the current lower bound of the `state` or None.
        """
        return state.minimize_bound


class VarState(object):
    """
    Class to facilitate handling order literals associated with an integer
    variable.

    The class maintains a stack of lower and upper bounds, which initially
    contain the smallest and largest allowed integer. These stacks must always
    contain at least one value.

    Members
    =======
    var -- The name of the integer variable.
    """
    def __init__(self, var):
        """
        Create an initial state for the given variable.

        Initially, the state has a lower bound of `MIN_INT` and an upper bound
        of `MAX_INT` and is associated with no variables.
        """
        self.var = var
        self._upper_bound = [MAX_INT]
        self._lower_bound = [MIN_INT]
        self._literals = SortedDict()

    def push_lower(self):
        """
        Grows the stack of lower bounds by one copying the top value.
        """
        self._lower_bound.append(self.lower_bound)

    def push_upper(self):
        """
        Grows the stack of upper bounds by one copying the top value.
        """
        self._upper_bound.append(self.upper_bound)

    def pop_lower(self):
        """
        Remove one item from the stack of lower bounds.

        Must be called on a stack of size greater than one.
        """
        assert len(self._lower_bound) > 1
        self._lower_bound.pop()

    def pop_upper(self):
        """
        Remove one item from the stack of upper bounds.

        Must be called on a stack of size greater than one.
        """
        assert len(self._upper_bound) > 1
        self._upper_bound.pop()

    @property
    def lower_bound(self):
        """
        Get the current lower bound on top of the stack.
        """
        assert self._lower_bound
        return self._lower_bound[-1]

    @lower_bound.setter
    def lower_bound(self, value):
        """
        Set the current lower bound on top of the stack.

        Must not be called on an empty stack.
        """
        assert self._lower_bound
        self._lower_bound[-1] = value

    @property
    def min_bound(self):
        """
        Get the smallest lower bound of the state.
        """
        assert self._lower_bound
        return self._lower_bound[0]

    @property
    def upper_bound(self):
        """
        Get the current upper bound on top of the stack.

        Must not be called on an empty stack.
        """
        assert self._upper_bound
        return self._upper_bound[-1]

    @upper_bound.setter
    def upper_bound(self, value):
        """
        Set the current upper bound on top of the stack.

        Must not be called on an empty stack.
        """
        assert self._upper_bound
        self._upper_bound[-1] = value

    @property
    def max_bound(self):
        """
        Get the largest upper bound of the state.
        """
        assert self._upper_bound
        return self._upper_bound[0]

    @property
    def is_assigned(self):
        """
        Determine if the variable is assigned, i.e., the current lower bound
        equals the current upper bound.
        """
        return self.upper_bound == self.lower_bound

    def has_literal(self, value):
        """
        Determine if the given `value` is associated with an order literal.

        The value must lie in the range `[MIN_INT,MAX_INT)`.
        """
        assert MIN_INT <= value < MAX_INT
        return value in self._literals

    def get_literal(self, value):
        """
        Get the literal associated with the given `value`.

        The value must be associated with a literal.
        """
        assert MIN_INT <= value < MAX_INT
        return self._literals[value]

    def prev_value(self, value):
        """
        Get the the first value preceeding `value` that is associated with a
        literal or None if there is no such value.

        The value must be associated with a literal.
        """
        assert self.has_literal(value)
        i = self._literals.bisect_left(value)
        if i > 0:
            return self._literals.peekitem(i-1)[0]
        return None

    def succ_value(self, value):
        """
        Get the the first value succeeding `value` that is associated with a
        literal or None if there is no such value.

        The value must be associated with a literal.
        """
        assert self.has_literal(value)
        i = self._literals.bisect_right(value)
        if i < len(self._literals):
            return self._literals.peekitem(i)[0]
        return None

    def set_literal(self, value, lit):
        """
        Set the literal of the given `value`.

        The value must lie in the range `[MIN_INT,MAX_INT)`.
        """
        assert MIN_INT <= value < MAX_INT
        self._literals[value] = lit

    def unset_literal(self, value):
        """
        Unset the literal of the given `value`.

        The value must lie in the range `[MIN_INT,MAX_INT)`.
        """
        assert MIN_INT <= value < MAX_INT
        del self._literals[value]

    def clear(self):
        """
        Remove all literals associated with this state.
        """
        self._upper_bound = [MAX_INT]
        self._lower_bound = [MIN_INT]
        self._literals.clear()

    def __repr__(self):
        return "{}=[{},{}]".format(self.var, self.lower_bound, self.upper_bound)


class ConstraintState(object):
    """
    Capture the lower and upper bound of constraints.
    """
    def __init__(self, lower_bound, upper_bound):
        self.lower_bound = lower_bound
        self.upper_bound = upper_bound


class Level(object):
    """
    Simple class that captures state local to a decision level.

    Members
    =======
    level      -- The decision level.
    undo_upper -- Set of `VarState` objects that have been assigned an upper
                  bound.
    undo_lower -- Set of `VarState` objects that have been assigned a lower
                  bound.
    """
    def __init__(self, level):
        """
        Construct an empty state for the given decision `level`.
        """
        self.level = level
        # Note: A trail-like data structure would also be possible but then
        # assignments would have to be undone.
        self.undo_upper = TodoList()
        self.undo_lower = TodoList()

    def clear(self):
        """
        Clear upper and lower bound stacks.
        """
        self.undo_upper.clear()
        self.undo_lower.clear()

    def __repr__(self):
        return "{}:l={}/u={}".format(self.level, self.undo_lower, self.undo_upper)


class State(object):
    """
    Class to store and propagate thread-specific state.

    Public Members
    ==============
    statistics        -- A ThreadStatistics object holding statistics.

    Private Members
    ===============
    _vars             -- List of VarState objects for variables occurring in
                         constraints.
    _var_state        -- Map from variable names to `VarState` objects.
    _litmap           -- Map from order literals to a list of `VarState/value`
                         pairs. If there is an order literal for `var<=value`,
                         then the pair `(vs,value)` is contained in the map
                         where `vs` is the VarState of `var`.
    _levels           -- For each decision level propagated, there is a `Level`
                         object in this list until `undo` is called.
    _vl2c             -- Map from variable names to a list of constraints. The
                         map contains a variable/constraint pair if increasing
                         the lower bound of the variable, increases the slack
                         of the constraint.
    _vu2c             -- Map from variable names to a list of constraints. The
                         map contains a variable/constraint pair if decreasing
                         the upper bound of the variable, increases the slack
                         of the constraint.
    _l2c              -- Map from literals to a list of constraints. The map
                         contains a literal/constraint pair if the literal is
                         associated with the constraint.
    _todo             -- Set of constraints that have to be propagated on the
                         current decision level.
    _facts_integrated -- A list of integer triples. Stores for each decision
                         level, how many true/false facts have already been
                         integrated. Unlike with `_levels`, such triples are
                         only introduced if necessary.
                         `check_full`.
    _lerp_last        -- Offset to speed up `check_full`.
    _trail_offset     -- Offset to speed up `simplify`.
    _minimize_bound   -- Current bound of the minimize constraint (if any).
    _minimize_level   -- The minimize constraint might not have been fully
                         propagated below this level. See `update_minimize`.
    _cstate           -- A dictionary mapping constraints to their states.
    """
    def __init__(self, l2c, vl2c, vu2c):
        """
        Construct an empty state initialising `_lc2` with `l2c`, `_vl2c` with
        `vl2c`, and `_vu2c` with `vu2c`.

        The state is ready to propagate decision level zero after a call to
        `init_domain`.
        """
        self._vars = []
        self._var_state = {}
        self._litmap = {}
        self._levels = [Level(0)]
        self._vl2c = vl2c
        self._vu2c = vu2c
        self._l2c = l2c
        self._todo = TodoList()
        self._facts_integrated = [(0, 0, 0)]
        self._lerp_last = 0
        self._trail_offset = 0
        self._minimize_bound = None
        self._minimize_level = 0
        self.statistics = ThreadStatistics()
        self._cstate = {}

    @property
    def minimize_bound(self):
        """
        Get the current bound of the minimize constraint.
        """
        return self._minimize_bound

    def update_minimize(self, minimize, dl, bound):
        """
        Updates the bound of the minimize constraint in this state.
        """
        if self._minimize_bound is None or bound < self._minimize_bound:
            self._minimize_bound = bound
            self._minimize_level = dl
            self._todo.add(minimize)
        elif dl < self._minimize_level:
            self._minimize_level = dl
            self._todo.add(minimize)

    def get_assignment(self):
        """
        Get the current assignment to all variables.

        This function should be called on the state corresponding to the thread
        where a model has been found.
        """
        return [(vs.var, vs.lower_bound) for vs in self._vars]

    def get_value(self, var):
        """
        Get the current value of a variable.

        This function should be called on the state corresponding to the thread
        where a model has been found.
        """
        return (self._var_state[var].lower_bound
                if var in self._var_state else
                MIN_INT)

    def _push_level(self, level):
        """
        Add a new decision level specific state if necessary.

        Has to be called in `propagate`.
        """
        assert self._levels
        if self._levels[-1].level < level:
            self._levels.append(Level(level))

    def _pop_level(self):
        """
        Remove the decision level specific states added last.

        Has to be called in `undo`.
        """
        assert len(self._levels) > 1
        self._levels.pop()

    def _state(self, var):
        """
        Get the state associated with variable `var`.
        """
        return self._var_state[var]

    @property
    def _level(self):
        """
        Get the state associated with the current decision level.

        Should only be used in `propagate`, `undo`, and `check`. When `check`
        is called, the current decision level can be higher than that of the
        `Level` object returned. Hence, the decision level specific state can
        only be modified for facts because only then changes also apply for
        smaller decision levels.
        """
        return self._levels[-1]

    def _get_literal(self, vs, value, cc):
        """
        Returns the literal associated with the `vs.var/value` pair.

        Values smaller below the smallest lower bound are associated with the
        false literal and values greater or equal to the largest upper bound
        with the true literal.

        This function creates a new literal using `cc` if there is no literal
        for the given value.
        """
        if value < vs.min_bound:
            return -TRUE_LIT
        if value >= vs.max_bound:
            return TRUE_LIT
        if not vs.has_literal(value):
            lit = cc.add_literal()
            # Note: By default clasp's heuristic makes literals false. By
            # flipping the literal for non-negative values, assignments close
            # to zero are preferred. This way, we might get solutions with
            # small numbers first.
            if value >= 0:
                lit = -lit
            vs.set_literal(value, lit)
            self._litmap.setdefault(lit, []).append((vs, value))
            cc.add_watch(lit)
            cc.add_watch(-lit)
        return vs.get_literal(value)

    def _remove_literal(self, vs, lit, value):
        """
        Removes order literal `lit` for `vs.var<=value` from `_litmap`.
        """
        assert lit not in (TRUE_LIT, -TRUE_LIT)
        vec = self._litmap[lit]
        assert (vs, value) in vec
        vec.remove((vs, value))
        if not vec:
            assert -lit not in self._litmap
            del self._litmap[lit]

    def _update_literal(self, vs, value, cc, truth):
        """
        This function is an extended version of `_get_literal` that can update
        an existing order literal for `vs.var/value` if truth is either true or
        false.

        The return value is best explained with pseudo code:
        ```
        # literal is not updated
        if truth is None:
          return True, _get_literal(vs, value, control)
        lit = TRUE_LIT if truth else -TRUE_LIT
        if value < vs.min_bound:
          old = -TRUE_LIT
        elif value >= vs.max_bound:
          old = TRUE_LIT
        elif vs.has_literal(value):
          old = vs.get_literal(value)
        else:
          old = None
        # literal has not been updated
        if old == lit:
          return True, lit
        # set the new literal
        vs.set_literal(value, lit)
        # fix the old literal and return new literal
        return cc.add_literal([old if truth else -old]), lit
        ```

        Additionally, if the the order literal is updated (`old` is not
        `None`), then the replaced value is also removed from `_litmap`.
        """
        if truth is None:
            return True, self._get_literal(vs, value, cc)
        lit = TRUE_LIT if truth else -TRUE_LIT
        if value < vs.min_bound or value >= vs.max_bound:
            old = self._get_literal(vs, value, cc)
            if old == lit:
                return True, lit
            return cc.add_clause([old if truth else -old]), lit
        if not vs.has_literal(value):
            vs.set_literal(value, lit)
            self._litmap.setdefault(lit, []).append((vs, value))
            return True, lit
        old = vs.get_literal(value)
        if old == lit:
            return True, lit
        # Note: If a literal is associated with both true and false, then we
        # get a top level conflict making further data structure updates
        # unnecessary.
        if old != -lit:
            vs.set_literal(value, lit)
            self._remove_literal(vs, old, value)
            self._litmap.setdefault(lit, []).append((vs, value))
        return cc.add_clause([old if truth else -old]), lit

    # initialization
    def add_variable(self, var):
        """
        Adds `VarState` objects for each variable in `variables`.
        """
        vs = VarState(var)
        self._var_state[var] = vs
        self._vars.append(vs)

    def add_constraint(self, constraint):
        """
        Adds the given constraint to the propagation queue and initializes its
        state.
        """
        lower = upper = 0
        for co, var in constraint.elements:
            vs = self._state(var)
            if co > 0:
                lower += vs.lower_bound*co
                upper += vs.upper_bound*co
            else:
                lower += vs.upper_bound*co
                upper += vs.lower_bound*co
        self._cstate[constraint] = ConstraintState(lower, upper)

        self._todo.add(constraint)

    def simplify(self, init):
        """
        Simplify the state using fixed literals in the trail up to the given
        offset and the enqued constraints in the todo list.

        Note that this functions assumes that newly added constraints have been
        enqueued before.
        """
        # Note: Propagation won't add anything to the trail because atm
        # there are no order literals which could be propagated. This
        # might change in the multi-shot case when order literals have
        # been added in a previous step which are then implied by the
        # newly added constraints.
        ass = init.assignment
        trail = ass.trail

        # Note: The initial propagation below, will not introduce any order
        # literals other than true or false.
        while True:
            if not init.propagate():
                return False

            trail_offset = len(trail)
            if self._trail_offset == trail_offset and not self._todo:
                return True

            if not self.propagate(init, trail[self._trail_offset:trail_offset]):
                return False
            self._trail_offset = trail_offset

            if not self.check(init):
                return False

    # propagation
    @measure_time("time_propagate")
    def propagate(self, cc, changes):
        """
        Propagates constraints and order literals.

        Constraints that became true are added to the todo list and bounds of
        variables are adjusted according to the truth of order literals.
        """
        ass = cc.assignment

        # open a new decision level if necessary
        self._push_level(ass.decision_level)

        # propagate order literals that became true/false
        for lit in changes:
            self._todo.extend(self._l2c.get(lit, []))
            if not self._update_domain(cc, lit):
                return False

        return True

    def _propagate_variable(self, cc, vs, value, lit, sign):
        """
        Propagates the preceeding or succeeding order literal of lit.

        Whether the target literal is a preceeding or succeeding literal is
        determined by `sign`. The target order literal is given by
        `(vs.var,value)` and must exist.

        For example, if `sign==1`, then lit is an order literal for some
        integer value smaller than `value`. The function propagates the clause
        `lit` implies `vs.get_literal(value)`.

        Furthermore, if `lit` is a fact, the target literal is simplified to a
        fact, too.
        """

        ass = cc.assignment
        assert ass.is_true(lit)
        assert vs.has_literal(value)

        # get the literal to propagate
        # Note: this explicetly does not use _get_literal
        con = sign * vs.get_literal(value)

        # on-the-fly simplify
        if ass.is_fixed(lit) and not ass.is_fixed(con):
            ret, con = self._update_literal(vs, value, cc, sign > 0)
            if not ret:
                return False
            con = sign * con

        # propagate the literal
        if not ass.is_true(con):
            if not cc.add_clause([-lit, con]):
                return False

        return True

    def _update_domain(self, cc, lit):
        """
        If `lit` is an order literal, this function updates the lower or upper
        bound associated to the variable of the literal (if necessary).
        Furthermore, the preceeding or succeeding order literal is propagated
        if it exists.
        """
        ass = cc.assignment
        assert ass.is_true(lit)

        lvl = self._level

        # update and propagate upper bound
        if lit in self._litmap:
            start = self._get_integrated(ass.decision_level)[0] if lit == TRUE_LIT else None
            for vs, value in self._litmap[lit][start:]:
                # update upper bound
                if vs.upper_bound > value:
                    diff = value - vs.upper_bound
                    if lvl.undo_upper.add(vs):
                        vs.push_upper()
                    vs.upper_bound = value
                    for co, constraint in self._vl2c.get(vs.var, []):
                        assert co*diff < 0
                        self._cstate[constraint].upper_bound += co*diff
                    for co, constraint in self._vu2c.get(vs.var, []):
                        assert co*diff > 0
                        self._cstate[constraint].lower_bound += co*diff
                        self._todo.add(constraint)

                # make succeeding literal true
                succ = vs.succ_value(value)
                if succ is not None and not self._propagate_variable(cc, vs, succ, lit, 1):
                    return False

        # update and propagate lower bound
        if -lit in self._litmap:
            start = self._get_integrated(ass.decision_level)[1] if lit == TRUE_LIT else None
            for vs, value in self._litmap[-lit][start:]:
                # update lower bound
                if vs.lower_bound < value+1:
                    diff = value+1-vs.lower_bound
                    if lvl.undo_lower.add(vs):
                        vs.push_lower()
                    vs.lower_bound = value+1
                    for co, constraint in self._vu2c.get(vs.var, []):
                        assert co*diff < 0
                        self._cstate[constraint].upper_bound += co*diff
                    for co, constraint in self._vl2c.get(vs.var, []):
                        assert co*diff > 0
                        self._cstate[constraint].lower_bound += co*diff
                        self._todo.add(constraint)

                # make preceeding literal false
                prev = vs.prev_value(value)
                if prev is not None and not self._propagate_variable(cc, vs, prev, lit, -1):
                    return False

        return True

    def _generate_reason(self, c, cc):
        """
        Generate a reason for a constraint and count the guessed literals in
        it.
        """
        clit = c.literal
        ass = cc.assignment
        lbs = []                          # lower bound literals
        num_guess = 1 if c.tagged else 0  # number of assignments above level 0
        for co, var in c.elements:
            vs = self._state(var)
            if co > 0:
                # note that any literal associated with a value smaller than
                # the lower bound is false
                lit = self._get_literal(vs, vs.lower_bound-1, cc)
            else:
                # note that any literal associated with a value greater or
                # equal than the upper bound is true
                lit = -self._get_literal(vs, vs.upper_bound, cc)

            assert ass.is_false(lit)
            if not ass.is_fixed(lit):
                num_guess += 1
            lbs.append(lit)

        if not ass.is_fixed(clit):
            num_guess += 1
        lbs.append(-clit)

        return num_guess, lbs

    def _check_state(self, c):
        lower = upper = 0
        for co, var in c.elements:
            vs = self._state(var)
            if co > 0:
                lower += co*vs.lower_bound
                upper += co*vs.upper_bound
            else:
                lower += co*vs.upper_bound
                upper += co*vs.lower_bound

        assert lower <= upper
        assert lower == self._cstate[c].lower_bound
        assert upper == self._cstate[c].upper_bound

    def propagate_constraint(self, c, cc):
        """
        This function propagates a constraint that became active because its
        associated literal became true or because the bound of one of its
        variables changed.

        The function calculates the slack of the constraint w.r.t. to the
        lower/upper bounds of its values. The order values are then propagated
        in such a way that the slack is non-negative. The trick here is that we
        can use the ordering of variables to restrict the number of
        propagations. For example, for positive coefficients, we just have to
        enforce the smallest order variable that would make the slack
        non-negative.

        The function returns False if propagation fails, True otherwise.
        """
        ass = cc.assignment
        rhs = c.rhs(self)

        # Note: this has a noticible cost because of the shortcuts below
        if CHECK_STATE:
            self._check_state(c)
        assert not ass.is_false(c.literal)

        # skip constraints that cannot become false
        if rhs is None or self._cstate[c].upper_bound <= rhs:
            return True
        slack = rhs-self._cstate[c].lower_bound

        # this is necessary to correctly handle empty constraints (and do
        # propagation of false constraints)
        if slack < 0:
            _, lbs = self._generate_reason(c, cc)
            return cc.add_clause(lbs, tag=c.tagged)

        if not ass.is_true(c.literal):
            return True

        num_guess, lbs = self._generate_reason(c, cc)

        for i, (co, var) in enumerate(c.elements):
            vs = self._state(var)

            # adjust the number of guesses if the current literal is a guess
            adjust = 1 if not ass.is_fixed(lbs[i]) else 0

            # if all literals are assigned on level 0 then we can associate the
            # value with a true/false literal, taken care of by the extra
            # argument truth to _update_literal
            if co > 0:
                truth = num_guess == adjust or None
                diff = slack+co*vs.lower_bound
                value = diff//co
                assert value >= vs.lower_bound
                # Note: all order literals above the upper bound are true already
                if value >= vs.upper_bound:
                    continue
                ret, lit = self._update_literal(vs, value, cc, truth)
                if not ret:
                    return False
            else:
                truth = num_guess > adjust and None
                diff = slack+co*vs.upper_bound
                value = -(diff//-co)
                assert value <= vs.upper_bound
                # Note: all order literals below the lower bound are false already
                if value <= vs.lower_bound:
                    continue
                ret, lit = self._update_literal(vs, value-1, cc, truth)
                if not ret:
                    return False
                lit = -lit

            # the value is chosen in a way that the slack is greater or equal
            # than 0 but also so that it does not exceed the coefficient (in
            # which case the propagation would not be strong enough)
            assert diff - co*value >= 0 and diff - co*value < abs(co)

            if not ass.is_true(lit):
                lbs[i], lit = lit, lbs[i]
                if not cc.add_clause(lbs, tag=c.tagged):
                    return False
                lbs[i] = lit

        return True

    def integrate_domain(self, cc, literal, var, domain):
        """
        Integrates the given domain for varibale var.

        Consider x in {[1,3), [4,6), [7,9)}. We can simply add the binary
        constraints:
        - right to left
          - true => x < 9
          - x < 7 => x < 6
          - x < 4 => x < 3
        - left to right
          - true => x >= 1
          - x >= 3 => x >= 4
          - x >= 6 => x >= 7
        """
        ass = cc.assignment
        if ass.is_false(literal):
            return True
        if ass.is_true(literal):
            literal = TRUE_LIT
        vs = self._state(var)

        py = None
        for x, y in domain:
            ly = TRUE_LIT if py is None else -self._get_literal(vs, py-1, cc)
            true = literal == TRUE_LIT and ass.is_true(ly)
            ret, lx = self._update_literal(vs, x-1, cc, not true and None)
            if not ret or not cc.add_clause([-literal, -ly, -lx]):
                return False
            py = y

        px = None
        for x, y in reversed(domain):
            ly = TRUE_LIT if px is None else self._get_literal(vs, px-1, cc)
            true = literal == TRUE_LIT and ass.is_true(ly)
            ret, lx = self._update_literal(vs, y-1, cc, true or None)
            if not ret or not cc.add_clause([-literal, -ly, lx]):
                return False
            px = x

        return True

    def integrate_simple(self, cc, constraint, strict):
        """
        This function integrates singleton constraints into the state.

        We explicitely handle the strict case here to avoid introducing
        unnecessary literals.
        """
        # pylint: disable=protected-access

        ass = cc.assignment
        clit = constraint.literal

        # the constraint is never propagated
        if not strict and ass.is_false(clit):
            return True

        assert len(constraint.elements) == 1
        co, var = constraint.elements[0]
        vs = self._state(var)

        if co > 0:
            truth = ass.value(clit)
            value = constraint.rhs(self)//co
        else:
            truth = ass.value(-clit)
            value = -(constraint.rhs(self)//-co)-1

        # in this case we can use the literal of the constraint as order variable
        if strict and vs.min_bound <= value < vs.max_bound and not vs.has_literal(value):
            lit = clit
            if co < 0:
                lit = -lit
            if truth is None:
                cc.add_watch(lit)
                cc.add_watch(-lit)
            elif truth:
                lit = TRUE_LIT
            else:
                lit = -TRUE_LIT
            vs.set_literal(value, lit)
            self._litmap.setdefault(lit, []).append((vs, value))

        # otherwise we just update the existing order literal
        else:
            ret, lit = self._update_literal(vs, value, cc, truth)
            if not ret:
                return False
            if co < 0:
                lit = -lit
            if not cc.add_clause([-clit, lit]):
                return False
            if strict and not cc.add_clause([-lit, clit]):
                return False

        return True

    @measure_time("time_undo")
    def undo(self):
        """
        This function undos decision level specific state.

        This includes undoing changed bounds of variables clearing constraints
        that where not propagated on the current decision level.
        """
        lvl = self._level

        for vs in lvl.undo_lower:
            value = vs.lower_bound
            vs.pop_lower()
            diff = value - vs.lower_bound
            for co, constraint in self._vl2c.get(vs.var, []):
                assert co*diff > 0
                self._cstate[constraint].lower_bound -= co*diff
            for co, constraint in self._vu2c.get(vs.var, []):
                assert co*diff < 0
                self._cstate[constraint].upper_bound -= co*diff

        lvl.undo_lower.clear()
        for vs in lvl.undo_upper:
            value = vs.upper_bound
            vs.pop_upper()
            diff = value - vs.upper_bound
            for co, constraint in self._vu2c.get(vs.var, []):
                assert co*diff > 0
                self._cstate[constraint].lower_bound -= co*diff
            for co, constraint in self._vl2c.get(vs.var, []):
                assert co*diff < 0
                self._cstate[constraint].upper_bound -= co*diff

        self._pop_level()
        # Note: To make sure that the todo list is cleared when there is
        #       already a conflict during propagate.
        self._todo.clear()

    # checking
    @property
    def _num_facts(self):
        """
        The a pair of intergers corresponding to the numbers of order literals
        associated with the true and false literal.
        """
        t = len(self._litmap.get(TRUE_LIT, []))
        f = len(self._litmap.get(-TRUE_LIT, []))
        return t, f

    def _get_integrated(self, level):
        """
        Returns the a pair of intergers corresponding to the numbers of order
        literals associated with the true and false literal that have already
        been propagated on the given level.
        """
        assert level >= 0 and self._facts_integrated and self._facts_integrated[0][0] == 0
        while self._facts_integrated[-1][0] > level:
            self._facts_integrated.pop()
        _, t, f = self._facts_integrated[-1]
        return t, f

    def _set_integrated(self, level, num):
        """
        Sets the a pair of intergers corresponding to the numbers of order
        literals associated with the true and false literal that have already
        been propagated on the given level.
        """
        old = self._get_integrated(level)
        if old != num:
            if self._facts_integrated[-1][0] < level:
                # Note: this case can only happen if facts are learned on
                # levels above 0. This can only happen if the propagator is
                # extended.
                self._facts_integrated.append((level, num[0], num[1]))
            else:
                assert self._facts_integrated[-1][0] == level
                self._facts_integrated[-1] = (level, num[0], num[1])

    @measure_time("time_check")
    def check(self, cc):
        """
        This functions propagates facts that have not been integrated on the
        current level and propagates constraints gathered during `propagate`.
        """

        ass = cc.assignment

        # Note: We have to loop here because watches for the true/false
        # literals do not fire again.
        while True:
            # Note: This integrates any facts that have not been integrated yet
            # on the current decision level.
            if not self._update_domain(cc, 1):
                return False
            num_facts = self._num_facts
            self._set_integrated(ass.decision_level, num_facts)

            # propagate affected constraints
            todo, self._todo = self._todo, TodoList()
            for c in todo:
                lit = c.literal
                if not ass.is_false(lit):
                    if not self.propagate_constraint(c, cc):
                        return False

            if num_facts == self._num_facts:
                return True

    def _check_constraint(self, c):
        """
        This function checks if a constraint is satisfied w.r.t. the final
        values of its integer variables.

        This function should only be called total assignments.
        """
        if c.rhs(self) is None:
            return True

        lhs = 0
        for co, var in c.elements:
            lhs += co*self.get_value(var)

        return lhs <= c.rhs(self)

    def check_full(self, control):
        """
        This function selects a variable that is not fully assigned w.r.t. the
        current assignment and introduces an additional order literal for it.

        This function should only be called total assignments.
        """
        post = range(self._lerp_last, len(self._vars))
        pre = range(0, self._lerp_last)
        for i in chain(post, pre):
            vs = self._vars[i]
            if not vs.is_assigned:
                self._lerp_last = i
                value = lerp(vs.lower_bound, vs.upper_bound)
                self._get_literal(vs, value, control)
                return

        if CHECK_SOLUTION:
            for lit, constraints in self._l2c.items():
                if control.assignment.is_true(lit):
                    for c in constraints:
                        assert self._check_constraint(c)

    # reinitialization
    def update(self, cc):
        """
        This function resets a state and should be called when a new solve step
        is started.

        This function removes all solve step local variables from the state,
        maps fixed global literals to the true/false literal, and resets the
        minimize constraint.
        """
        ass = cc.assignment

        self._minimize_bound = None
        self._minimize_level = 0

        remove_invalid = []
        remove_fixed = []
        for lit, vss in self._litmap.items():
            if abs(lit) == TRUE_LIT:
                continue

            if not ass.has_literal(lit):
                remove_invalid.append((lit, vss))
            elif ass.is_fixed(lit):
                remove_fixed.append((lit, vss))

        # remove solve step local variables
        # Note: Iteration order does not matter.
        for lit, vss in remove_invalid:
            for vs, value in vss:
                vs.unset_literal(value)
            del self._litmap[lit]

        # Note: Map bounds associated with top level facts to true/false.
        # Because we do not know if the facts have already been propagated, we
        # simply append them and do not touch the counts for integrated facts.
        for old, vss in sorted(remove_fixed):
            for vs, value in vss:
                lit = TRUE_LIT if ass.is_true(old) else -TRUE_LIT
                self._litmap.setdefault(lit, []).append((vs, value))
                vs.set_literal(value, lit)
            del self._litmap[old]

    def _cleanup_literals(self, cc, lit, pred):
        """
        Remove (var,value) pairs associated with `lit` that match `pred`.
        """
        assert lit in (TRUE_LIT, -TRUE_LIT)
        if lit in self._litmap:
            variables = self._litmap[lit]

            # adjust the number of facts that have been integrated
            idx = 0 if lit == TRUE_LIT else 1
            nums = list(self._get_integrated(0))
            for x in variables[:nums[idx]]:
                if pred(x):
                    nums[idx] -= 1
            self._set_integrated(0, nums)

            # remove values matching pred
            i = remove_if(variables, pred)
            assert i > 0
            for vs, value in variables[i:]:
                old = vs.get_literal(value)
                if old != lit:
                    # Note: This case cannot be triggered if propagation works
                    # correctly because facts can only be propagated on level
                    # 0. But to be on the safe side in view of theory
                    # extensions, this makes the old literal equal to lit
                    # before removing the old literal.
                    if not cc.add_clause([-lit, old], lock=True):
                        return False
                    if not cc.add_clause([-old, lit], lock=True):
                        return False
                    self._remove_literal(vs, old, value)
                vs.unset_literal(value)
            del variables[i:]

        return True

    def cleanup_literals(self, cc):
        """
        Remove all order literals associated with facts that are above the
        upper or below the lower bound.
        """
        # make sure that all top level literals are assigned to the fact literal
        self.update(cc)

        # cleanup
        return (self._cleanup_literals(cc, TRUE_LIT, lambda x: x[1] != x[0].upper_bound) and
                self._cleanup_literals(cc, -TRUE_LIT, lambda x: x[1] != x[0].lower_bound-1))

    def update_bounds(self, cc, other):
        """
        Integrate the lower and upper bounds from State `other`.

        The function might add clauses via `cc` to fix literals that have to be
        updated. This can lead to a conflict if states have conflicting
        lower/upper bounds.

        Precondition: update should be called before this function to really
                      integrate all bounds.
        """
        # pylint: disable=protected-access

        # update upper bounds
        for vs_b, _ in other._litmap.get(TRUE_LIT, []):
            vs_a = self._var_state[vs_b.var]
            if vs_b.upper_bound < vs_a.upper_bound:
                ret, _ = self._update_literal(vs_a, vs_b.upper_bound, cc, True)
                if not ret:
                    return False

        # update lower bounds
        for vs_b, _ in other._litmap.get(-TRUE_LIT, []):
            vs_a = self._var_state[vs_b.var]
            if vs_a.lower_bound < vs_b.lower_bound:
                ret, _ = self._update_literal(vs_a, vs_b.lower_bound-1, cc, False)
                if not ret:
                    return False

        return self._update_domain(cc, 1)

    def copy_state(self, master):
        """
        Copy order literals and propagation state from the given `master` state
        to the current state.
        """
        # pylint: disable=protected-access

        # adjust integrated facts
        self._set_integrated(0, master._get_integrated(0))

        # make sure we have an empty var state for each variable
        for vs in master._vars[len(self._vars):]:
            self.add_variable(vs.var)
        for vs, vs_master in zip(self._vars, master._vars):
            assert vs.var == vs_master.var
            vs.clear()

        # copy the map from literals to var states
        self._litmap.clear()
        for lit, vss in master._litmap.items():
            for vs_master, value in vss:
                vs = self._state(vs_master.var)
                vs.set_literal(value, lit)
                self._litmap.setdefault(lit, []).append((vs, value))

        # adjust the bounds
        for vs, vs_master in zip(self._vars, master._vars):
            vs.lower_bound = vs_master.lower_bound
            vs.upper_bound = vs_master.upper_bound

        # copy constraint state
        for c, cs in master._cstate.items():
            self._cstate[c] = ConstraintState(cs.lower_bound, cs.upper_bound)

        # adjust levels
        lvl, lvl_master = self._level, master._level
        lvl.clear()
        for vs in lvl_master.undo_lower:
            lvl.undo_lower.add(self._state(vs.var))
        for vs in lvl_master.undo_upper:
            lvl.undo_upper.add(self._state(vs.var))


class CSPBuilder(object):
    """
    CSP builder to use with the parse_theory function.
    """
    def __init__(self, init, propagator, minimize):
        self._init = init
        self._propagator = propagator
        self._clauses = []
        self.minimize = minimize

    def solver_literal(self, literal):
        """
        Map the literal to a solver literal.
        """
        return self._init.solver_literal(literal)

    def add_clause(self, clause):
        """
        Add a clause to be added later.

        Note that the clause is added later because adding clauses and
        variables alternatingly is inefficient in clasp.
        """
        self._clauses.append(clause)
        return True

    def add_literal(self):
        """
        Adds a new literal.
        """
        return self._init.add_literal()

    def add_watch(self, lit):
        """
        Watch the given solver literal.
        """
        self._init.add_watch(lit)

    def propagate(self):
        """
        (Pretend to) call unit propagation on the solver.
        """
        return True

    @property
    def assignment(self):
        """
        Return the current assignment.
        """
        return self._init.assignment

    def add_constraint(self, lit, elems, rhs, strict):
        """
        Add a constraint.
        """
        if not strict and self.assignment.is_false(lit):
            return

        constraint = Constraint(lit, elems, rhs)
        if len(elems) == 1:
            _, var = elems[0]
            self._propagator.add_variable(var)
            self._propagator.add_simple(self, constraint, strict)
        else:
            assert not strict
            self._propagator.add_constraint(self, constraint)

    def add_minimize(self, co, var):
        """
        Add a term to the minimize constraint.
        """
        if self.minimize is None:
            self.minimize = Minimize()

        if var is not None:
            self._propagator.add_variable(var)

        if co == 0:
            return

        self.minimize.elements.append((co, var))

    def add_distinct(self, literal, elems):
        """
        Adds a simplistic translation for a distinct constraint.

        For each x_i, x_j with i < j in elems add
          `literal => &sum { x_i } != x_j`
        where each x_i is of form (rhs_i, term_i) and term_i is a list of
        coefficients and variables; x_i corresponds to the linear term
          `term - rhs`.
        """
        if self.assignment.is_false(literal):
            return

        for i, (rhs_i, elems_i) in enumerate(elems):
            for rhs_j, elems_j in elems[i+1:]:
                rhs = rhs_i - rhs_j

                celems = []
                celems.extend(elems_i)
                celems.extend((-co_j, var_j) for co_j, var_j in elems_j)

                if not celems:
                    if rhs == 0:
                        self.add_clause([-literal])
                        return
                    continue

                a = self.add_literal()
                b = self.add_literal()

                self.add_clause([a, b, -literal])
                self.add_clause([-a, -b])

                self.add_constraint(a, celems, rhs-1, False)
                self.add_constraint(b, [(-co, var) for co, var in celems], -rhs-1, False)

    def add_dom(self, literal, var, elements):
        """
        Add a domain for the given variable.

        The domain is represented as a set of left-closed intervals.
        """
        if self.assignment.is_false(literal):
            return

        self._propagator.add_variable(var)
        intervals = IntervalSet(elements)
        self._propagator.add_dom(self, literal, var, list(intervals.items()))

    def finalize(self):
        """
        Finish constraint translation.
        """

        # simplify minimize
        if self.minimize is not None:
            adjust, self.minimize.elements = simplify(self.minimize.elements, True)
            self.minimize.adjust += adjust

        # add clauses
        for clause in self._clauses:
            if not self._init.add_clause(clause) or not self._init.propagate():
                return False

        return True


class Propagator(object):
    """
    A propagator for CSP constraints.
    """
    def __init__(self):
        self._l2c = {}                   # map literals to constraints
        self._states = []                # map thread id to states
        self._vars = TodoList()          # list of variables
        self._vl2c = {}                  # map variables affecting lower bound of constraints
        self._vu2c = {}                  # map variables affecting upper bound of constraints
        self._minimize = None            # minimize constraint
        self._minimize_bound = None      # bound of the minimize constraint
        self._stats_step = Statistics()  # statistics of the current call
        self._stats_accu = Statistics()  # accumulated statistics

    def _state(self, thread_id):
        """
        Get the state associated with the given `thread_id`.
        """
        while len(self._states) <= thread_id:
            self._states.append(State(self._l2c, self._vl2c, self._vu2c))
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
        def thread_stats(tstat):
            p, c, u = tstat.time_propagate, tstat.time_check, tstat.time_undo
            return OrderedDict([
                ("Time total(s)", p+c+u),
                ("Time propagation(s)", p),
                ("Time check(s)", c),
                ("Time undo(s)", u)])
        stats_map["Clingcon"] = OrderedDict([
            ("Time init(s)", stats.time_init),
            ("Variables", stats.num_variables),
            ("Constraints", stats.num_constraints),
            ("Thread", map(thread_stats, stats.tstats[:len(self._states)]))])

    def add_variable(self, var):
        """
        Add a variable to the program.
        """
        if var not in self._vars:
            self._vars.add(var)
            self._state(0).add_variable(var)

    def add_dom(self, cc, literal, var, domain):
        """
        Add a domain for the given variable.
        """
        return self._state(0).integrate_domain(cc, literal, var, domain)

    def add_simple(self, cc, constraint, strict):
        """
        Add a constraint that can be represented by an order literal.
        """
        return self._state(0).integrate_simple(cc, constraint, strict)

    def add_constraint(self, init, constraint):
        """
        Add a constraint to the program.
        """
        lit = constraint.literal
        init.add_watch(lit)
        self._l2c.setdefault(lit, []).append(constraint)
        for co, var in constraint.elements:
            self.add_variable(var)
            if co > 0:
                self._vl2c.setdefault(var, []).append((co, constraint))
            else:
                self._vu2c.setdefault(var, []).append((co, constraint))

        self._state(0).add_constraint(constraint)
        self._stats_step.num_constraints += 1

    @measure_time("time_init")
    def init(self, init):
        """
        Initializes the propagator extracting constraints from the theory data.

        The function handles reinitialization for multi-shot solving and
        multi-threaded solving.
        """
        init.check_mode = clingo.PropagatorCheckMode.Fixpoint
        cc = InitClauseCreator(init)

        # remove minimize constraint
        minimize = self.remove_minimize()

        # remove solve step local and fixed literals
        for state in self._states:
            state.update(cc)

        # gather bounds of states in master
        master = self._state(0)
        for state in self._states[1:]:
            if not master.update_bounds(cc, state):
                return

        # add constraints
        builder = CSPBuilder(init, self, minimize)
        if not parse_theory(builder, init.theory_atoms).finalize():
            return

        self._stats_step.num_variables = len(self._vars)

        # propagate the newly added constraints
        if not master.simplify(cc):
            return

        # remove unnecessary literals after simplification
        if not master.cleanup_literals(cc):
            return

        # add minimize constraint
        # Note: the constraint is added in the end to avoid propagating tagged
        # clauses, which is not supported at the moment.
        if builder.minimize is not None:
            self._stats_step.num_constraints += 1
            self.add_minimize(cc, builder.minimize)

        # copy order literals from master to other states
        del self._states[init.number_of_threads:]
        for i in range(1, init.number_of_threads):
            self._state(i).copy_state(master)

    def propagate(self, control, changes):
        """
        Delegates propagation to the respective state.
        """
        state = self._state(control.thread_id)
        state.propagate(ControlClauseCreator(control), changes)

    def check(self, control):
        """
        Delegates checking to the respective state and makes sure that all
        order variables are assigned if the assigment is total.
        """
        size = len(control.assignment)
        state = self._state(control.thread_id)
        dl = control.assignment.decision_level
        if self.has_minimize:
            bound = self._minimize_bound + self._minimize.adjust if self._minimize_bound else None
            state.update_minimize(self._minimize, dl, bound)

        if not state.check(ControlClauseCreator(control)):
            return

        # Note: Makes sure that all variables are assigned in the end. But even
        # if the assignment is total, we do not have to introduce fresh
        # variables if variables have been introduced during check. In this
        # case, there is a guaranteed follow-up propagate call because all
        # newly introduced variables are watched.
        if size == len(control.assignment) and control.assignment.is_total:
            state.check_full(control)

    def undo(self, thread_id, assign, changes):
        # pylint: disable=unused-argument
        """
        Delegates undoing to the respective state.
        """
        self._state(thread_id).undo()

    def get_assignment(self, thread_id):
        """
        Get the assigment from the state associated with `thread_id`.

        Should be called on total assignments.
        """
        return self._state(thread_id).get_assignment()

    def get_value(self, symbol, thread_id):
        """
        Get the value of the given variable in the state associated with
        `thread_id`.

        Should be called on total assignments.
        """
        return self._state(thread_id).get_value(symbol)

    @property
    def has_minimize(self):
        """
        Check if the propagator has a minimize constraint.
        """
        return self._minimize is not None

    def add_minimize(self, init, minimize):
        """
        Add a minimize constraint to the program.
        """
        self._minimize = minimize
        self.add_constraint(init, minimize)

    def remove_minimize(self):
        """
        Removes the minimize constraint from the lookup lists.
        """
        minimize, self._minimize = self._minimize, None
        if minimize is not None:
            lit = minimize.literal
            self._l2c.setdefault(lit, []).remove(minimize)
            for co, var in minimize.elements:
                if co > 0:
                    self._vl2c.setdefault(var, []).remove((co, minimize))
                else:
                    self._vu2c.setdefault(var, []).remove((co, minimize))
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
