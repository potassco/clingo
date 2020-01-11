"""
This module provides a propagator for CSP constraints. It can also be used as a
stand-alone application.
"""

import sys
from itertools import chain
from textwrap import dedent
import clingo
from clingo import ast

try:
    # Note: Can be installed to play with realistic domain sizes.
    from sortedcontainers import SortedDict
    _HAS_SC = True
except ImportError:
    _HAS_SC = False


MAX_INT = 20
MIN_INT = -20
TRUE_LIT = 1


def lerp(x, y):
    """
    Linear interpolation between integers `x` and `y` with a factor of `.5`.
    """
    # NOTE: integer division with floor
    return x + (y - x) // 2


def match(term, name, arity):
    """
    Match the given term if it is a function with signature `name/arity`.
    """
    return (term.type in (clingo.TheoryTermType.Function, clingo.TheoryTermType.Symbol) and
            term.name == name and
            len(term.arguments) == arity)


def remove_if(rng, pred):
    """
    Remove each element from `rng` for which `pred` evaluates to true by
    swapping them to the end of the sequence.

    The function returns the number of elements retained.
    """
    j = 0
    for i, x in enumerate(rng):
        if pred(x):
            continue
        if i != j:
            rng[i], rng[j] = rng[j], rng[i]
        j += 1
    return j


def _parse_constraints(init):
    """
    Parse and yield sum and diff constraints in the theory data of the given
    `PropagateInit` object.
    """

    for atom in init.theory_atoms:
        is_sum = match(atom.term, "sum", 1)
        is_diff = match(atom.term, "diff", 1)
        if is_sum or is_diff:
            body = match(atom.term.arguments[0], "body", 0)
            cons = _parse_constraint(init, atom, is_sum)
            yield cons
            if body:
                cons = Constraint(-cons.literal, [(-co, var) for co, var in cons.elements], -cons.rhs-1)
                yield cons


def _parse_constraint(init, atom, is_sum):
    """
    Construct a constraint from a theory `atom` and associated with the
    given `literal`.

    If `is_sum` is true parses a sum constraint. Otherwise, it parses a
    difference constraint as supported by clingo-dl.
    """

    elements = []

    # map literal
    literal = init.solver_literal(atom.literal)

    # combine coefficients
    seen = {}
    for i, (co, var) in enumerate(_parse_constraint_elems(atom.elements, is_sum)):
        if co == 0:
            continue
        if var not in seen:
            seen[var] = i
            elements.append((co, var))
        else:
            elements[seen[var]][0] += co

    # drop zero weights
    elements = [(co, var) for co, var in elements if co != 0]

    return Constraint(literal, elements, _parse_constraint_num(atom.guard[1]))


def _parse_constraint_elems(elems, is_sum):
    for elem in elems:
        if len(elem.terms) == 1 and not elem.condition:
            # python 3 has yield from
            for x in _parse_constraint_elem(elem.terms[0], is_sum):
                yield x
        else:
            raise RuntimeError("Invalid Syntax")


def _parse_constraint_elem(term, is_sum):
    if not is_sum:
        if match(term, "-", 2):
            var = _parse_constraint_var(term.arguments[0])
            if var != "0":
                yield 1, var
            var = _parse_constraint_var(term.arguments[1])
            if var != "0":
                yield -1, var
        else:
            raise RuntimeError("Invalid Syntax")
    else:
        if match(term, "+", 2):
            for x in _parse_constraint_elem(term.arguments[0], True):
                yield x
            for x in _parse_constraint_elem(term.arguments[1], True):
                yield x
        elif match(term, "*", 2):
            num = _parse_constraint_num(term.arguments[0])
            var = _parse_constraint_var(term.arguments[1])
            yield num, var
        else:
            raise RuntimeError("Invalid Syntax")


def _parse_constraint_num(term):
    if term.type == clingo.TheoryTermType.Number:
        return term.number
    if (term.type == clingo.TheoryTermType.Symbol and
            term.symbol.type == clingo.SymbolType.Number):
        return term.symbol.number
    if match(term, "-", 1):
        return -_parse_constraint_num(term.arguments[0])
    if match(term, "+", 1):
        return +_parse_constraint_num(term.arguments[0])
    raise RuntimeError("Invalid Syntax")


def _parse_constraint_var(term):
    return str(term)


class Constraint(object):
    """
    Class to capture sum constraints of form `a_0*x_0 + ... + a_n * x_n <= rhs`.

    Members
    =======
    literal  -- Solver literal associated with the constraint.
    elements -- List of integer/string pairs representing coefficient and
                variable.
    rhs      -- Integer bound of the constraint.
    """
    def __init__(self, literal, elements, rhs):
        """
        Create a constraint and initialize all members.
        """
        self.literal = literal
        self.elements = elements
        self.rhs = rhs

    def __str__(self):
        return "{} <= {}".format(self.elements, self.rhs)


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
        if _HAS_SC:
            self._literals = SortedDict()
        else:
            self._literals = (MAX_INT-MIN_INT)*[None]

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

        Must not be called on an empty stack.
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
        if _HAS_SC:
            return value in self._literals
        return self._literals[value - MIN_INT] is not None

    def get_literal(self, value):
        """
        Get the literal associated with the given `value`.

        The value must be associated with a literal.
        """
        assert self.has_literal(value)
        if _HAS_SC:
            return self._literals[value]
        return self._literals[value - MIN_INT]

    def prev_value(self, value):
        """
        Get the the first value preceeding `value` that is associated with a
        literal or None if there is no such value.

        The value must be associated with a literal.
        """
        assert self.has_literal(value)
        if _HAS_SC:
            i = self._literals.bisect_left(value)
            if i > 0:
                return self._literals.peekitem(i-1)[0]
        else:
            for prev in range(value-1, MIN_INT-1, -1):
                if self.has_literal(prev):
                    return prev
        return None

    def succ_value(self, value):
        """
        Get the the first value succeeding `value` that is associated with a
        literal or None if there is no such value.

        The value must be associated with a literal.
        """
        assert self.has_literal(value)
        if _HAS_SC:
            i = self._literals.bisect_right(value)
            if i < len(self._literals):
                return self._literals.peekitem(i)[0]
        else:
            for succ in range(value+1, MAX_INT):
                if self.has_literal(succ):
                    return succ
        return None

    def set_literal(self, value, lit):
        """
        Set the literal of the given `value`.

        The value must lie in the range `[MIN_INT,MAX_INT)`.
        """
        assert MIN_INT <= value < MAX_INT
        if _HAS_SC:
            self._literals[value] = lit
        else:
            self._literals[value - MIN_INT] = lit

    def unset_literal(self, value):
        """
        Unset the literal of the given `value`.

        The value must lie in the range `[MIN_INT,MAX_INT)`.
        """
        assert MIN_INT <= value < MAX_INT
        if _HAS_SC:
            del self._literals[value]
        else:
            self._literals[value - MIN_INT] = None

    def __repr__(self):
        return "{}=[{},{}]".format(self.var, self.lower_bound, self.upper_bound)


class TodoList(object):
    """
    Simple class implementing something like an OrderedSet, which is missing
    from pythons collections module.

    The container is similar to Python's set but maintains insertion order.
    """
    def __init__(self):
        """
        Construct an empty container.
        """
        self._seen = set()
        self._list = []

    def __len__(self):
        return len(self._list)

    def __contains__(self, x):
        return x in self._seen

    def __iter__(self):
        return iter(self._list)

    def __getitem__(self, val):
        return self._list[val]

    def add(self, x):
        """
        Add `x` to the container if it is not yet contained in it.

        Returns true if the element has been inserted.
        """
        if x not in self:
            self._seen.add(x)
            self._list.append(x)
            return True
        return False

    def extend(self, i):
        """
        Calls `add` for each element in sequence `i`.
        """
        for x in i:
            self.add(x)

    def clear(self):
        """
        Clears the container.
        """
        self._seen.clear()
        del self._list[:]


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


class State(object):
    """
    Class to store and propagate thread-specific state.

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

    def _get_literal(self, vs, value, control):
        """
        Returns the literal associated with the `vs.var/value` pair.

        Values smaller the MIN_INT are associated with the false literal and
        values greater or equal to MAX_INT with the true literal.

        This function creates a new literal using `control` if there is no
        literal for the given value.
        """
        if value < MIN_INT:
            return -TRUE_LIT
        if value >= MAX_INT:
            return TRUE_LIT
        if not vs.has_literal(value):
            lit = control.add_literal()
            # Note: By default clasp's heuristic makes literals false. By
            # flipping the literal for non-negative values, assignments close
            # to zero are preferred. This way, we might get solutions with
            # small numbers first.
            if value >= 0:
                lit = -lit
            vs.set_literal(value, lit)
            self._litmap.setdefault(lit, []).append((vs, value))
            control.add_watch(lit)
            control.add_watch(-lit)
        return vs.get_literal(value)

    def _remove_literal(self, control, vs, lit, value):
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
            # Note: When called with `PropagateInit`, there is no
            # `remove_watch` function.
            if hasattr(control, "remove_watch"):
                control.remove_watch(lit)
                control.remove_watch(-lit)

    def _update_literal(self, vs, value, control, truth):
        """
        This function is an extended version of `_get_literal` that can update
        an existing order literal for `vs.var/value` if truth is either true or
        false.

        The return value is best explained with pseudo code:
        ```
        # literal is not updated
        if truth is None:
          return None, _get_literal(vs, value, control)
        lit = TRUE_LIT if truth else -TRUE_LIT
        if value < MIN_INT:
          old = -TRUE_LIT
        elif value >= MAX_INT:
          old = TRUE_LIT
        elif vs.has_literal(value):
          old = vs.get_literal(value)
        else:
          old = None
        # literal has not been updated
        if old == lit:
          return None, lit
        # set the new literal
        vs.set_literal(value, lit)
        # return old and new literal
        return old, lit
        ```

        Additionally, if the the order literal is updated (`old` is not
        `None`), then the replaced value is also removed from `_litmap`.
        """
        if truth is None:
            return None, self._get_literal(vs, value, control)
        lit = TRUE_LIT if truth else -TRUE_LIT
        if value < MIN_INT or value >= MAX_INT:
            old = self._get_literal(vs, value, control)
            if old == lit:
                return None, lit
            return old, lit
        if not vs.has_literal(value):
            vs.set_literal(value, lit)
            self._litmap.setdefault(lit, []).append((vs, value))
            return None, lit
        old = vs.get_literal(value)
        if old == lit:
            return None, lit
        # Note: If a literal is associated with both true and false, then we
        # get a top level conflict making further data structure updates
        # unnecessary.
        if old != -lit:
            vs.set_literal(value, lit)
            self._remove_literal(control, vs, old, value)
            self._litmap.setdefault(lit, []).append((vs, value))
        return old, lit

    # initialization
    def init_domain(self, variables):
        """
        Adds `VarState` objects for each variable in `variables`.
        """
        for v in variables:
            vs = VarState(v)
            self._var_state[v] = vs
            self._vars.append(vs)

    def simplify(self, init, trail_offset):
        """
        Simplify the state using fixed literals in the trail up to the given
        offset.
        """
        # Note: Propagation won't add anything to the trail because atm
        # there are no order literals which could be propagated. This
        # might change in the multi-shot case when order literals have
        # been added in a previous step which are then implied by the
        # newly added constraints.
        trail = init.assignment.trail

        if not self.propagate(init, trail[self._trail_offset:trail_offset]):
            return False
        self._trail_offset = max(self._trail_offset, trail_offset)

        if not self.check(init):
            return False

        return True

    # propagation
    def propagate(self, control, changes):
        """
        Propagates constraints and order literals.

        Constraints that became true are added to the todo list and bounds of
        variables are adjusted according to the truth of order literals.
        """
        ass = control.assignment

        # open a new decision level if necessary
        self._push_level(ass.decision_level)

        # propagate order literals that became true/false
        for lit in changes:
            self._todo.extend(self._l2c.get(lit, []))
            if not self._update_domain(control, lit):
                return False

        return True

    def _propagate_variable(self, control, vs, value, lit, sign):
        """
        Propagates the preceeding or succeeding order literal of lit.

        Whether the target literal is a preceeding or succeeding literal is
        determined by `sign`. The target order literal is given by
        `(vs.var,value)` and must exist.

        For example, if `sign==1`, then lit is an order literal for some
        integer value smaller than `value`. The function propagates the clause
        `lit` implies `_get_literal(vs, value, control)`.

        Furthermore, if `lit` is a fact, the target literal is simplified to a
        fact, too.
        """

        assert vs.has_literal(value)
        ass = control.assignment
        assert ass.is_true(lit)

        # get the literal to propagate
        con = sign * self._get_literal(vs, value, control)

        # on-the-fly simplify
        if ass.is_fixed(lit) and not ass.is_fixed(con):
            o, con = self._update_literal(vs, value, control, sign > 0)
            o, con = sign * o, sign * con
            if not control.add_clause([o], lock=True):
                return False

        # propagate the literal
        if not ass.is_true(con) and not control.add_clause([-lit, con]):
            return False

        return True

    def _update_domain(self, control, lit):
        """
        If `lit` is an order literal, this function updates the lower or upper
        bound associated to the variable of the literal (if necessary).
        Furthermore, the preceeding or succeeding order literal is propagated
        if it exists.
        """
        ass = control.assignment
        assert ass.is_true(lit)

        lvl = self._level

        # update and propagate upper bound
        if lit in self._litmap:
            start = self._get_integrated(ass.decision_level)[0] if lit == TRUE_LIT else None
            for vs, value in self._litmap[lit][start:]:
                # update upper bound
                if vs.upper_bound > value:
                    if lvl.undo_upper.add(vs):
                        vs.push_upper()
                    vs.upper_bound = value
                    self._todo.extend(self._vu2c.get(vs.var, []))

                # make succeeding literal true
                succ = vs.succ_value(value)
                if succ is not None and not self._propagate_variable(control, vs, succ, lit, 1):
                    return False

        # update and propagate lower bound
        if -lit in self._litmap:
            start = self._get_integrated(ass.decision_level)[1] if lit == TRUE_LIT else None
            for vs, value in self._litmap[-lit][start:]:
                # update lower bound
                if vs.lower_bound < value+1:
                    if lvl.undo_lower.add(vs):
                        vs.push_lower()
                    vs.lower_bound = value+1
                    self._todo.extend(self._vl2c.get(vs.var, []))

                # make preceeding literal false
                prev = vs.prev_value(value)
                if prev is not None and not self._propagate_variable(control, vs, prev, lit, -1):
                    return False

        return True

    def propagate_constraint(self, l, c, control):
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

        The function yields clauses for propagating order literals. Because
        some order literals might be replaced with facts, some clauses have to
        be locked to permanently fix the truth values of the previous order
        literals. For this reason a pairs of clauses and Booleans are yielded
        where the second argument determines whether the clauses should be
        locked.
        """
        ass = control.assignment
        assert not ass.is_false(l)

        # NOTE: recalculation could be avoided by maintaing per clause state
        #       (but the necessary data structure would be quite complicated)
        slack = c.rhs  # sum of lower bounds (also saw this called slack)
        lbs = []       # lower bound literals
        num_guess = 0  # number of assignments above level 0
        for i, (co, var) in enumerate(c.elements):
            vs = self._state(var)
            if co > 0:
                slack -= co*vs.lower_bound
                # note that any literal associated with a value smaller than
                # the lower bound is false
                lit = self._get_literal(vs, vs.lower_bound-1, control)
            else:
                slack -= co*vs.upper_bound
                # note that any literal associated with a value greater or
                # equal than the upper bound is true
                lit = -self._get_literal(vs, vs.upper_bound, control)

            assert ass.is_false(lit)
            if not ass.is_fixed(lit):
                num_guess += 1
            lbs.append(lit)
        if not ass.is_fixed(l):
            num_guess += 1

        lbs.append(-l)

        # this is necessary to correctly handle empty constraints (and do
        # propagation of false constraints)
        if slack < 0:
            yield lbs, False

        if not ass.is_true(l):
            return

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
                old, lit = self._update_literal(vs, value, control, truth)
                if old is not None:
                    yield [old], True
            else:
                truth = num_guess > adjust and None
                diff = slack+co*vs.upper_bound
                value = -(diff//-co)
                old, lit = self._update_literal(vs, value-1, control, truth)
                lit = -lit
                if old is not None:
                    yield [-old], True

            # the value is chosen in a way that the slack is greater or equal
            # than 0 but also so that it does not exceed the coefficient (in
            # which case the propagation would not be strong enough)
            assert diff - co*value >= 0 and diff - co*value < abs(co)

            if not ass.is_true(lit):
                lbs[i], lit = lit, lbs[i]
                yield lbs, False
                lbs[i] = lit

    def undo(self):
        """
        This function undos decision level specific state.

        This includes undoing changed bounds of variables clearing constraints
        that where not propagated on the current decision level.
        """
        lvl = self._level
        for vs in lvl.undo_lower:
            vs.pop_lower()
        lvl.undo_lower.clear()
        for vs in lvl.undo_upper:
            vs.pop_upper()
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

    def check(self, control):
        """
        This functions propagates facts that have not been integrated on the
        current level and propagates constraints gathered during `propagate`.
        """
        ass = control.assignment

        # Note: We have to loop here because watches for the true/false
        # literals do not fire again.
        while True:
            # Note: This integrates any facts that have not been integrated yet
            # on the current decision level.
            if not self._update_domain(control, 1):
                return False
            num_facts = self._num_facts
            self._set_integrated(ass.decision_level, num_facts)

            # propagate affected constraints
            for c in self._todo:
                lit = c.literal
                if not ass.is_false(lit):
                    for clause, lock in self.propagate_constraint(lit, c, control):
                        # Note: This is a hack because the PropagateInit object
                        #       does not have a lock parameter.
                        args = {}
                        if lock:
                            args["lock"] = True
                        if not control.add_clause(clause, **args) or not control.propagate():
                            self._todo.clear()
                            return False
            self._todo.clear()

            if num_facts == self._num_facts:
                break

        return True

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

    # reinitialization
    def remove_literals(self, init):
        """
        This function removes all solve step local variables from the state and
        maps fixed global literals to the true/false literal.
        """
        ass = init.assignment

        remove_invalid = []
        remove_fixed = []
        for lit, vss in self._litmap.items():
            if abs(lit) == TRUE_LIT:
                continue
            elif not ass.has_literal(lit):
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

    def _cleanup_literals(self, init, lit, pred):
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
                    if not init.add_clause([-lit, old]) or not init.add_clause([-old, lit]):
                        return False
                    self._remove_literal(init, vs, old, value)
                vs.unset_literal(value)
            del variables[i:]

        return True

    def cleanup_literals(self, init):
        """
        Remove all order literals associated with facts that are above the
        upper or below the lower bound.
        """
        return (self._cleanup_literals(init, TRUE_LIT, lambda x: x[1] != x[0].upper_bound) and
                self._cleanup_literals(init, -TRUE_LIT, lambda x: x[1] != x[0].lower_bound-1))

    def update_bounds(self, init, other):
        """
        Integrate the lower and upper bounds from State `other`.

        The function might add clauses via `init` to fix literals that have to
        be updated. This can lead to a conflict if states have conflicting
        lower/upper bounds.
        """
        # pylint: disable=protected-access

        # update upper bounds
        for vs_b, _ in other._litmap.get(TRUE_LIT, []):
            vs_a = self._var_state[vs_b.var]
            if vs_b.upper_bound < vs_a.upper_bound:
                old, _ = self._update_literal(vs_a, vs_b.upper_bound, init, True)
                if old is not None and not init.add_clause([old]):
                    return False

        # update lower bounds
        for vs_b, _ in other._litmap.get(-TRUE_LIT, []):
            vs_a = self._var_state[vs_b.var]
            if vs_a.lower_bound < vs_b.lower_bound:
                old, _ = self._update_literal(vs_a, vs_b.lower_bound-1, init, False)
                if old is not None and not init.add_clause([-old]):
                    return False

        return self._update_domain(init, 1)

    def copy_domain(self, init, master):
        """
        Copy order literals from the given `master` state to the current state.
        """
        # pylint: disable=protected-access

        for lit, vss in sorted(master._litmap.items()):
            # Note: fixed literals are handled in `update_bounds` and `remove_literals`.
            if not init.assignment.is_fixed(lit):
                for vs_m, value in vss:
                    vs = self._var_state[vs_m.var]
                    vs.set_literal(lit, value)
                    self._litmap.setdefault(lit, []).append((vs, value))


class Propagator(object):
    """
    A propagator for CSP constraints.
    """
    def __init__(self):
        self._l2c = {}     # map literals to constraints
        self._states = []  # map thread id to states
        self._vars = []    # list of variables
        self._vl2c = {}    # map variables affecting lower bound of constraints
        self._vu2c = {}    # map variables affecting upper bound of constraints

    def _state(self, thread_id):
        """
        Get the state associated with the given `thread_id`.
        """
        while len(self._states) <= thread_id:
            self._states.append(State(self._l2c, self._vl2c, self._vu2c))
        return self._states[thread_id]

    def _add_constraints(self, init):
        """
        Add constraints in theory data to propagator.

        Returns lists of the added constraints and newly added variables.
        """
        constraints = []
        variables = []
        variables_seen = set(self._vars)

        for c in _parse_constraints(init):
            constraints.append(c)
            self._l2c.setdefault(c.literal, []).append(c)
            init.add_watch(c.literal)
            for co, var in c.elements:
                if var not in variables_seen:
                    variables_seen.add(var)
                    variables.append(var)
                if co > 0:
                    self._vl2c.setdefault(var, []).append(c)
                else:
                    self._vu2c.setdefault(var, []).append(c)

        return constraints, variables

    def init(self, init):
        """
        Initializes the propagator extracting constraints from the theory data.

        The function handles reinitialization for multi-shot solving and
        multi-threaded solving.
        """
        init.check_mode = clingo.PropagatorCheckMode.Fixpoint

        # get newly added constraints and variables
        constraints, variables = self._add_constraints(init)

        # remove solve step local and fixed literals
        for state in self._states:
            state.remove_literals(init)

        # gather bounds of states in master
        if self._states:
            master = self._states[0]
            for state in self._states[1:]:
                if not master.update_bounds(init, state):
                    return

        # adjust number of threads if necessary
        del self._states[init.number_of_threads:]
        if self._vars:
            for i in range(len(self._states), init.number_of_threads):
                state = self._state(i)
                state.init_domain(self._vars)
                state.copy_domain(self._states[0])

        # add newly introduced variables
        self._vars.extend(variables)
        for i in range(init.number_of_threads):
            state = self._state(i)
            state.init_domain(variables)

        # propagate bounds in master to other states
        master = self._states[0]
        for state in self._states[1:]:
            if not state.update_bounds(init, master):
                return

        ass = init.assignment
        trail = ass.trail
        trail_offset = 0

        # propagate the newly added constraints
        # Note: consequences of previously added constraints are stored in the
        # bounds propagated above.
        for state in self._states:
            # pylint: disable=protected-access
            state._todo.extend(constraints)

        # Note: The initial propagation below, will not introduce any order
        # literals other than true or false.
        while True:
            if not init.propagate():
                return

            # Note: Initially, the trail is guaranteed to have at least size 1.
            # This ensures that order literals will be propagated.
            if trail_offset == len(trail):
                break
            next_trail_offset = len(trail)

            for state in self._states:
                if not state.simplify(init, next_trail_offset):
                    return

            trail_offset = next_trail_offset

        # remove unnecessary literals after simplification
        for state in self._states:
            if not state.cleanup_literals(init):
                return

    def propagate(self, control, changes):
        """
        Delegates propagation to the respective state.
        """
        state = self._state(control.thread_id)
        state.propagate(control, changes)

    def check(self, control):
        """
        Delegates checking to the respective state and makes sure that all
        order variables are assigned if the assigment is total.
        """
        size = control.assignment.size
        state = self._state(control.thread_id)

        if not state.check(control):
            return

        # Note: Makes sure that all variables are assigned in the end. But even
        # if the assignment is total, we do not have to introduce fresh
        # variables if variables have been introduced during check. In this
        # case, there is a guaranteed follow-up propagate call because all
        # newly introduced variables are watched.
        if size == control.assignment.size and control.assignment.is_total:
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
        return self._state(thread_id).get_value(str(symbol))


class Transformer(object):
    """
    Transforms `clingo.ast.AST` objects by visiting all child nodes.

    Implement `visit_<type>` where `<type>` is the type of the nodes to be
    visited.
    """
    def visit_children(self, x, *args, **kwargs):
        """
        Visit all child nodes of the current node.
        """
        for key in x.child_keys:
            setattr(x, key, self.visit(getattr(x, key), *args, **kwargs))
        return x

    def visit(self, x, *args, **kwargs):
        """
        Default visit method to dispatch calls to child nodes.
        """
        if isinstance(x, ast.AST):
            attr = "visit_" + str(x.type)
            if hasattr(self, attr):
                return getattr(self, attr)(x, *args, **kwargs)
            return self.visit_children(x, *args, **kwargs)
        if isinstance(x, list):
            return [self.visit(y, *args, **kwargs) for y in x]
        if x is None:
            return x
        raise TypeError("unexpected type")


class HeadBodyTransformer(Transformer):
    """
    Transforms sum/diff theory atoms in heads and bodies of rules by turning
    the name of each theory atom into a function with head or body as argument.
    """
    # pylint: disable=invalid-name

    def visit_Rule(self, rule):
        """
        Visit rules adding a parameter indicating whether the head or body is
        being visited.
        """
        rule.head = self.visit(rule.head, loc="head")
        rule.body = self.visit(rule.body, loc="body")
        return rule

    def visit_TheoryAtom(self, atom, loc="body"):
        """
        Modify sum and diff in theory atoms by adding loc as parameter to the
        name of theory atom.
        """
        t = atom.term
        if t.name in ["sum", "diff"] and t.arguments == []:
            atom.term = ast.Function(t.location, t.name, [ast.Function(t.location, loc, [], False)], False)
        return atom


class Application(object):
    """
    Application class that can be used with `clingo.clingo_main` to solve CSP
    problems.
    """
    def __init__(self):
        self.program_name = "csp"
        self.version = "1.0"
        self._propagator = Propagator()
        self._bound_symbol = None  # CSP variable to minimize
        self._bound_value = None   # current/initial value of CSP variable

    def print_model(self, model, default_printer):
        """
        Print the current model and the assignment to integer variables.
        """
        ass = self._propagator.get_assignment(model.thread_id)

        default_printer()

        print("Valid assignment for constraints found:")
        print(" ".join("{}={}".format(n, v) for n, v in ass))

        if self._bound_symbol is not None:
            print("CSP Optimization: {}".format(self._get_bound(model)))

        sys.stdout.flush()

    def _parse_min(self, value):
        """
        Parse and set minimum value for integer variables.
        """
        # Note: just done like this because otherwise the data-structure for
        #       literals in `VarState` objects would have become a little more
        #       difficult. This should not be done in a real application.
        # pylint: disable=global-statement
        global MIN_INT
        MIN_INT = int(value)
        return True

    def _parse_max(self, value):
        """
        Parse and set maximum value for integer variables.
        """
        # Note: just done like this because otherwise the data-structure for
        #       literals in `VarState` objects would have become a little more
        #       difficult. This should not be done in a real application.
        # pylint: disable=global-statement
        global MAX_INT
        MAX_INT = int(value)
        return True

    def _parse_minimize(self, value):
        """
        Parse and set variable to minimize and an optional initial value.
        """
        term = clingo.parse_term("({},)".format(value))
        args = term.arguments
        size = len(args)
        if size == 0 or size > 2 or (size > 1 and args[1].type != clingo.SymbolType.Number):
            return False
        self._bound_symbol = args[0]
        if size > 1:
            self._bound_value = args[1].number
        return True

    def register_options(self, options):
        """
        Register CSP related options.
        """
        group = "CSP Options"
        options.add(group, "min-int", "Minimum integer [-20]", self._parse_min, argument="<i>")
        options.add(group, "max-int", "Maximum integer [20]", self._parse_max, argument="<i>")
        options.add(group, "minimize-variable", dedent("""\
            Minimize the given variable
                  <arg>   : <variable>[,<initial>]
                  <variable>: the variable to minimize
                  <initial> : upper bound for the variable,
            """), self._parse_minimize)

    def validate_options(self):
        """
        Validate options making sure that the minimum is larger than the
        maximum integer.
        """
        if MIN_INT > MAX_INT:
            raise RuntimeError("min-int must not be larger than max-int")

    def _get_bound(self, model):
        """
        Get the current value of the variable being minimized.
        """
        return self._propagator.get_value(self._bound_symbol, model.thread_id)

    def main(self, prg, files):
        """
        Entry point of the application registering the propagator and
        implementing the standard ground and solve functionality.
        """
        with prg.builder() as b:
            t = HeadBodyTransformer()
            for f in files:
                clingo.parse_program(open(f).read(),
                                     lambda stm: b.add(t.visit(stm)))
        prg.register_propagator(self._propagator)
        prg.add("base", [], dedent("""\
            #theory cp {
                constant  { - : 1, unary };
                diff_term {
                - : 0, binary, left
                };
                sum_term {
                - : 2, unary;
                * : 1, binary, left;
                + : 0, binary, left
                };
                &sum/1 : sum_term, {<=}, constant, any;
                &diff/1 : diff_term, {<=}, constant, any
            }.
            """))

        prg.ground([("base", [])])
        if self._bound_symbol is None:
            prg.solve()
        else:
            # Note: This is to mirror the implementation in clingo-dl. In
            # principle this could be dealt with differently with order
            # variables.
            with prg.builder() as b:
                clingo.parse_program("#program __bound(s,b). &sum { 1*s } <= b.",
                                     lambda stm: b.add(HeadBodyTransformer().visit(stm)))
#            prg.add("__bound", ("s", "b"), "&sum { 1*s } <= b.")

            found = True
            while found:
                if self._bound_value is not None:
                    prg.ground((("__bound", (
                        self._bound_symbol,
                        clingo.Number(self._bound_value - 1))),))

                found = False
                for model in prg.solve(yield_=True):
                    self._bound_value = self._get_bound(model)
                    found = True
                    break


if __name__ == "__main__":
    sys.exit(int(clingo.clingo_main(Application(), sys.argv[1:])))
