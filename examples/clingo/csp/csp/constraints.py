"""
Module implementing constraints.
"""

import csp
from .util import TodoList, IntervalSet
from .parsing import simplify


class AbstractConstraintState(object):
    """
    Abstract class to capture the state of constraints.
    """
    def __init__(self):
        self.inactive_level = 0

    @property
    def marked_inactive(self):
        """
        Returns true if the constraint is marked inactive.
        """
        return self.inactive_level > 0

    @marked_inactive.setter
    def marked_inactive(self, level):
        """
        Mark a constraint as inactive on the given level.
        """
        assert not self.marked_inactive
        self.inactive_level = level+1

    def mark_active(self):
        """
        Mark a constraint as active.
        """
        self.inactive_level = 0

    def removable(self, level):
        """
        A constraint is removable if it has been marked inactive on a lower
        level.
        """
        return self.marked_inactive and self.inactive_level <= level


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
    removable = True

    def __init__(self, literal, elements, rhs):
        self.literal = literal
        self.elements = elements
        self._rhs = rhs

    def rhs(self, state):  # pylint: disable=unused-argument
        """
        Returns the rhs.
        """
        return self._rhs

    def attach(self, state):
        """
        Attache the constraint to a state.
        """
        # TODO: move the code in the state here
        state.add_constraint(self)

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
    removable = False

    def __init__(self):
        self.literal = csp.TRUE_LIT
        self.elements = []
        self.adjust = 0

    def rhs(self, state):
        """
        Returns the current lower bound of the `state` or None.
        """
        return state.minimize_bound

    def attach(self, state):
        """
        Attache the constraint to a state.
        """
        # TODO: move the code in the state here
        state.add_constraint(self)


class ConstraintState(AbstractConstraintState):
    """
    Capture the lower and upper bound of constraints.
    """
    def __init__(self, constraint):
        AbstractConstraintState.__init__(self)
        self.constraint = constraint
        self.lower_bound = 0
        self.upper_bound = 0

    @property
    def literal(self):
        """
        Return the literal of the associated constraint.
        """
        return self.constraint.literal

    @property
    def tagged_removable(self):
        """
        True if the constraint can be temporarily removed.

        This property will only be false if the constraint state is associated
        with a minimize constraint.
        """
        return self.constraint.removable

    def _weight_estimate(self, state):
        """
        Estimate the size of the translation in terms of the number of literals
        necessary for the weight constraint.
        """
        estimate = 0
        slack = self.constraint.rhs(state)-self.lower_bound
        for co, var in self.constraint.elements:
            vs = state.var_state(var)
            if co > 0:
                diff = slack+co*vs.lower_bound
                value = diff//co
                assert value >= vs.lower_bound
                estimate += min(value+1, vs.upper_bound)-vs.lower_bound
            else:
                diff = slack+co*vs.upper_bound
                value = -(diff//-co)
                assert value <= vs.upper_bound
                estimate += vs.upper_bound-max(value-1, vs.lower_bound)
        return estimate

    def _weight_translate(self, cc, state, slack):
        """
        Translate the constraint to weight a constraint.
        """
        # translate small enough constraint
        # Note: this magic number can be dangerous if there is a huge number of
        # variables.
        wlits = []
        for co, var in self.constraint.elements:
            vs = state.var_state(var)
            if co > 0:
                diff = slack+co*vs.lower_bound
                value = diff//co
                assert value >= vs.lower_bound
                for i in range(vs.lower_bound, min(value+1, vs.upper_bound)):
                    wlits.append((-state.get_literal(vs, i, cc), co))
            else:
                diff = slack+co*vs.upper_bound
                value = -(diff//-co)
                assert value <= vs.upper_bound
                for i in range(max(value-1, vs.lower_bound), vs.upper_bound):
                    wlits.append((state.get_literal(vs, i, cc), -co))

        # Note: For strict constraints, we can actually use the equivalence
        # here and only add one weight constraint instead of two. In the
        # current system design, this requires storing the weight
        # constraints and detect complementary constraints. It might be a
        # good idea in general to add translation constraints later because
        # we can run into the problem of successivly adding variables and
        # constraints here.
        ret = cc.add_weight_constraint(self.literal, wlits, slack, 1)

        state.remove_constraint(self.constraint)
        return ret, True

    def _rec_estimate(self, cc, state, elements, estimate, i, lower, upper):
        if lower < 0:
            return estimate+1
        assert upper < 0 and i < len(elements)
        co, var = elements[i]
        vs = state.var_state(var)
        if co > 0:
            diff_upper = lower+co*vs.lower_bound
            diff_lower = upper+co*vs.upper_bound
            value_upper = min(vs.upper_bound-1, diff_upper//co)
            value_lower = max(vs.lower_bound-1, diff_lower//co)
            assert vs.lower_bound <= value_upper+1
            assert value_lower <= vs.upper_bound
            estimate += value_upper-value_lower+1
            if estimate >= 0:
                return estimate
            for next_value in range(value_upper+1, value_lower, -1):
                next_upper = upper+co*(vs.upper_bound-next_value)
                next_lower = lower-co*(next_value-vs.lower_bound)
                # Note: It should be possible to tune this further. In each
                # step we relax the constraint by decreasing var. Hence, the
                # following estimate call will count at least as many clauses
                # as the previous one.
                estimate = self._rec_estimate(cc, state, elements, estimate-1, i+1, next_lower, next_upper)
                if estimate >= 0:
                    return estimate
        else:
            diff_upper = upper+co*vs.lower_bound
            diff_lower = lower+co*vs.upper_bound
            value_upper = min(vs.upper_bound, -(diff_upper//-co)-1)
            value_lower = max(vs.lower_bound, -(diff_lower//-co)-1)
            assert value_lower <= vs.upper_bound
            assert vs.lower_bound <= value_upper+1
            estimate += value_upper-value_lower+1
            if estimate >= 0:
                return estimate
            for next_value in range(value_lower, value_upper+1):
                next_upper = upper+co*(vs.lower_bound-next_value)
                next_lower = lower-co*(next_value-vs.upper_bound)
                estimate = self._rec_estimate(cc, state, elements, estimate-1, i+1, next_lower, next_upper)
                if estimate >= 0:
                    return estimate

        return estimate

    def _rec_translate(self, cc, state, elements, clause, i, lower, upper):
        if lower < 0:
            return True if csp.LITERALS_ONLY else cc.add_clause(clause)
        assert upper < 0 and i < len(elements)
        co, var = elements[i]
        vs = state.var_state(var)
        if co > 0:
            diff_upper = lower+co*vs.lower_bound
            diff_lower = upper+co*vs.upper_bound
            value_upper = min(vs.upper_bound-1, diff_upper//co)
            value_lower = max(vs.lower_bound-1, diff_lower//co)
            assert vs.lower_bound <= value_upper+1
            assert value_lower <= vs.upper_bound
            for next_value in range(value_upper+1, value_lower, -1):
                next_upper = upper+co*(vs.upper_bound-next_value)
                next_lower = lower-co*(next_value-vs.lower_bound)
                clause.append(state.get_literal(vs, next_value-1, cc))
                if not self._rec_translate(cc, state, elements, clause, i+1, next_lower, next_upper):
                    return False
                clause.pop()
        else:
            diff_upper = upper+co*vs.lower_bound
            diff_lower = lower+co*vs.upper_bound
            value_upper = min(vs.upper_bound, -(diff_upper//-co)-1)
            value_lower = max(vs.lower_bound, -(diff_lower//-co)-1)
            assert value_lower <= vs.upper_bound
            assert vs.lower_bound <= value_upper+1
            for next_value in range(value_lower, value_upper+1):
                next_upper = upper+co*(vs.lower_bound-next_value)
                next_lower = lower-co*(next_value-vs.upper_bound)
                clause.append(-state.get_literal(vs, next_value, cc))
                if not self._rec_translate(cc, state, elements, clause, i+1, next_lower, next_upper):
                    return False
                clause.pop()

        return True

    def translate(self, cc, state):
        """
        Translate a constraint to clauses or weight constraints.
        """
        ass = cc.assignment

        if self.constraint.tagged:
            return True, False

        rhs = self.constraint.rhs(state)
        if ass.is_false(self.literal) or self.upper_bound <= rhs:
            state.remove_constraint(self.constraint)
            return True, True

        lower = rhs-self.lower_bound
        upper = rhs-self.upper_bound

        # Note: otherwise propagation is broken
        assert lower >= 0

        # translation to clauses
        if not csp.SORT_ELEMENTS:
            elements = sorted(self.constraint.elements, key=lambda x: -abs(x[0]))
        else:
            elements = self.constraint.elements

        if self._rec_estimate(cc, state, elements, -csp.MAGIC_CLAUSE, 0, lower, upper) < 0:
            ret = self._rec_translate(cc, state, elements, [-self.literal], 0, lower, upper)
            if csp.LITERALS_ONLY:
                return ret, False
            state.remove_constraint(self.constraint)
            return ret, True

        # translation to weight constraints
        if self._weight_estimate(state) < csp.MAGIC_WEIGHT_CONSTRAINT:
            return self._weight_translate(cc, state, lower)

        return True, False

    def undo(self, co, diff):
        """
        Undo the last updates of the bounds of the constraint by the given
        difference.
        """
        if co*diff > 0:
            self.lower_bound -= co*diff
        else:
            self.upper_bound -= co*diff

    def update(self, co, diff):
        """
        Update the bounds of the constraint by the given difference.
        """
        assert co*diff != 0
        if co*diff < 0:
            self.upper_bound += co*diff
            return False
        self.lower_bound += co*diff
        return True

    def _check_state(self, state):
        lower = upper = 0
        for co, var in self.constraint.elements:
            vs = state.var_state(var)
            if co > 0:
                lower += co*vs.lower_bound
                upper += co*vs.upper_bound
            else:
                lower += co*vs.upper_bound
                upper += co*vs.lower_bound

        assert lower <= upper
        assert lower == self.lower_bound
        assert upper == self.upper_bound

    def _calculate_reason(self, state, cc, slack, vs, co):
        ass = cc.assignment
        ret = True
        found = 0

        if co > 0:
            current = vs.lower_bound
            # the direct reason literal
            lit = state.get_literal(vs, current-1, cc)
            assert ass.is_false(lit)
            if csp.REFINE_REASONS and slack+co < 0 and ass.decision_level > 0:  # pylint: disable=chained-comparison
                delta = -((slack+1)//-co)
                value = max(current+delta, vs.min_bound)
                if value < current:
                    # refine reason literal
                    vl = vs.value_ge(value-1)
                    if vl is not None and vl[0]+1 < current:
                        found = 1
                        slack -= co*(vl[0]+1-current)
                        current = vl[0]+1
                        assert slack < 0
                        lit = vl[1]
                        assert ass.is_false(lit)

                    # introduce reason literal
                    # Note: It is important to imply literals by the smallest
                    # available literal to keep the state consistent.
                    # Furthermore, we only introduce literals implied on the
                    # current decision level to avoid backtracking.
                    if csp.REFINE_INTRODUCE and ass.level(lit) == ass.decision_level and value < current:
                        state.statistics.introduced_reason += 1
                        found = 1
                        slack -= co*(value-current)
                        assert slack < 0
                        refined = state.get_literal(vs, value-1, cc)
                        assert not ass.is_true(refined)
                        ret = ass.is_false(refined) or cc.add_clause([lit, -refined])
                        lit = refined
        else:
            # symmetric case
            current = vs.upper_bound
            lit = -state.get_literal(vs, current, cc)
            assert ass.is_false(lit)
            if csp.REFINE_REASONS and slack-co < 0 and ass.decision_level > 0:  # pylint: disable=chained-comparison
                delta = (slack+1)//co
                value = min(current+delta, vs.max_bound)
                if value > current:
                    vl = vs.value_le(value)
                    if vl is not None and vl[0] > current:
                        found = 1
                        slack -= co*(vl[0]-current)
                        current = vl[0]
                        assert slack < 0
                        lit = -vl[1]
                        assert ass.is_false(lit)

                    if csp.REFINE_INTRODUCE and ass.level(lit) == ass.decision_level and value > current:
                        state.statistics.introduced_reason += 1
                        found = 1
                        slack -= co*(value-current)
                        assert slack < 0
                        refined = -state.get_literal(vs, value, cc)
                        assert not ass.is_true(refined)
                        ret = ass.is_false(refined) or cc.add_clause([lit, -refined])
                        lit = refined

        state.statistics.refined_reason += found
        assert not ret or ass.is_false(lit)
        return ret, slack, lit

    def propagate(self, state, cc):
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
        rhs = self.constraint.rhs(state)

        # Note: this has a noticible cost because of the shortcuts below
        if csp.CHECK_STATE and not self.marked_inactive:
            self._check_state(state)
        assert not ass.is_false(self.literal)

        # skip constraints that cannot become false
        if rhs is None or self.upper_bound <= rhs:
            state.mark_inactive(self)
            return True
        slack = rhs-self.lower_bound

        # this is necessary to correctly handle empty constraints (and do
        # propagation of false constraints)
        if slack < 0:
            reason = []

            # add reason literals
            for co, var in self.constraint.elements:
                vs = state.var_state(var)

                # calculate reason literal
                ret, slack, lit = self._calculate_reason(state, cc, slack, vs, co)
                if not ret:
                    return False

                # append the reason literal
                if not ass.is_fixed(lit):
                    reason.append(lit)

            # append the consequence
            reason.append(-self.literal)

            state.mark_inactive(self)
            return cc.add_clause(reason, tag=self.constraint.tagged)

        if not ass.is_true(self.literal):
            return True

        for co_r, var_r in self.constraint.elements:
            vs_r = state.var_state(var_r)
            lit_r = 0

            # calculate the firet value that would violate the constraint
            if co_r > 0:
                delta_r = -((slack+1)//-co_r)
                value_r = vs_r.lower_bound+delta_r
                assert slack-co_r*delta_r < 0 <= slack-co_r*(delta_r-1)
                # values above the upper bound are already true
                if value_r > vs_r.upper_bound:
                    continue
                # get the literal of the value
                if vs_r.has_literal(value_r-1):
                    lit_r = state.get_literal(vs_r, value_r-1, cc)
            else:
                delta_r = (slack+1)//co_r
                value_r = vs_r.upper_bound+delta_r
                assert slack-co_r*delta_r < 0 <= slack-co_r*(delta_r+1)
                # values below the lower bound are already false
                if value_r < vs_r.lower_bound:
                    continue
                # get the literal of the value
                if vs_r.has_literal(value_r):
                    lit_r = -state.get_literal(vs_r, value_r, cc)

            # build the reason if the literal has not already been propagated
            if lit_r == 0 or not ass.is_true(lit_r):
                slack_r = slack-co_r*delta_r
                assert slack_r < 0
                reason = []
                # add the constraint itself
                if not ass.is_fixed(-self.literal):
                    reason.append(-self.literal)
                for co_a, var_a in self.constraint.elements:
                    if var_a == var_r:
                        continue
                    vs_a = state.var_state(var_a)

                    # calculate reason literal
                    ret, slack_r, lit_a = self._calculate_reason(state, cc, slack_r, vs_a, co_a)
                    if not ret:
                        return False

                    # append the reason literal
                    if not ass.is_fixed(lit_a):
                        reason.append(lit_a)

                # append the consequence
                guess = reason or self.constraint.tagged
                if co_r > 0:
                    ret, lit_r = state.update_literal(vs_r, value_r-1, cc, not guess or None)
                    if not ret:
                        return False
                else:
                    ret, lit_r = state.update_literal(vs_r, value_r, cc, guess and None)
                    if not ret:
                        return False
                    lit_r = -lit_r
                reason.append(lit_r)

                # propagate the clause
                if not cc.add_clause(reason, tag=self.constraint.tagged):
                    return False

                # minimize constraints cannot be propagated on decision level 0
                assert ass.is_true(lit_r) or self.constraint.tagged and ass.decision_level == 0

        return True

    def check_full(self, state):
        """
        This function checks if a constraint is satisfied w.r.t. the final
        values of its integer variables.

        This function should only be called total assignments.
        """
        if self.constraint.rhs(state) is None:
            return True

        lhs = 0
        for co, var in self.constraint.elements:
            vs = state.var_state(var)
            assert vs.is_assigned
            lhs += co*vs.lower_bound

        if self.marked_inactive:
            assert lhs <= self.upper_bound
        else:
            assert lhs == self.lower_bound
            assert lhs == self.upper_bound

        return lhs <= self.constraint.rhs(state)

    def copy(self):
        """
        Return a copy of the constraint state to be used with another state.
        """
        cs = ConstraintState(self.constraint)
        cs.inactive_level = self.inactive_level
        cs.lower_bound = self.lower_bound
        cs.upper_bound = self.upper_bound
        return cs


class Distinct(object):
    """
    Record holding a distinct constraint.
    """
    def __init__(self, literal, elements):
        self.literal = literal
        self.elements = elements

    def attach(self, state):
        """
        Attach the distinct constraint to a state.
        """
        # TODO: move the code in the state here
        state.add_distinct(self)


class DistinctState(AbstractConstraintState):
    """
    Capture the state of a distinct constraint.
    """
    def __init__(self, constraint):
        AbstractConstraintState.__init__(self)
        self.constraint = constraint
        self.dirty = TodoList()
        self.todo = TodoList()
        self.map_upper = {}
        self.map_lower = {}
        self.assigned = {}

    def copy(self):
        """
        Return a copy of the constraint state to be used with another state.
        """
        ds = DistinctState(self.constraint)
        ds.inactive_level = self.inactive_level
        for value, indices in self.map_upper.items():
            ds.map_upper[value] = indices[:]
        for value, indices in self.map_lower.items():
            ds.map_lower[value] = indices[:]
        ds.assigned = self.assigned.copy()
        return ds

    @property
    def literal(self):
        """
        Return the literal of the associated constraint.
        """
        return self.constraint.literal

    @property
    def tagged_removable(self):
        """
        Distinct constraints are removable.

        Currently, they are only removed if the constraint is false.
        """
        return True

    def init(self, state, i):
        """
        Recalculates the bounds of the i-th element of the constraint assuming
        that the bounds of this element are not currently in the bound maps.
        """
        # calculate new values
        value, elements = self.constraint.elements[i]
        upper = lower = value
        for co, var in elements:
            if co > 0:
                upper += co*state.var_state(var).upper_bound
                lower += co*state.var_state(var).lower_bound
            else:
                upper += co*state.var_state(var).lower_bound
                lower += co*state.var_state(var).upper_bound
        # set new values
        self.assigned[i] = (lower, upper)
        self.map_upper.setdefault(upper, []).append(i)
        self.map_lower.setdefault(lower, []).append(i)

    def update(self, i, _):
        """
        Add an element whose bound has changed to the todo list and mark it as
        dirty.

        If i is greater zero, than the lower bound changed; otherwise the upper
        bound.
        """
        self.dirty.add(abs(i)-1)
        self.todo.add(i)
        return True

    def undo(self, i, _):
        """
        Clear the todo list and mark the given element as dirty.
        """
        self.dirty.add(abs(i)-1)
        self.todo.clear()

    def _update(self, state):
        """
        Recalculate all elements marked dirty.
        """
        for i in self.dirty:
            lower, upper = self.assigned[i]
            self.map_lower[lower].remove(i)
            self.map_upper[upper].remove(i)
            self.init(state, i)
        self.dirty.clear()

    def translate(self, cc, state):
        # TODO: small distinct constraints should be translated to weight
        #       constraints
        return True, False

    def _propagate(self, cc, state, s, i, j):
        """
        Propagate a distinct constraint assuming that element i is assigned and
        one of element element j's bounds as determined by s match the value of
        element i.

        The reasons generated by this function are not necessarily unit.
        Implementing proper unit propagation for arbitrary linear terms would
        be quite involved. This implementation still works because it
        guarantees conflict detection and the added constraints might become
        unit later. The whole situation could be simplified by restricting the
        syntax of distinct constraints.
        """
        # case s > 0:
        #   example: x != y+z
        #     x <= 10 & not x <= 9 & y <= 5 & z <= 5 => y <= 4 | z <= 4
        #   example: x != -y
        #     x <= 9 & not x <= 8 &     y >= -9  =>     y >= -8
        #     x <= 9 & not x <= 8 & not y <= -10 => not y <= -9
        # case s < 0:
        #   example: x != y
        #     x = 9 & y >= 9 => y >= 10
        #     x <= 9 & not x <= 8 & not y <= 8 => not y <= 9
        #   example: x != -y
        #     x <= 9 & not x <= 8 & y <= -9 => y <= -10
        ass = cc.assignment

        reason = []
        is_fact = len(self.constraint.elements[j][1]) == 1

        # assigned index
        for _, var in self.constraint.elements[i][1]:
            vs = state.var_state(var)
            assert vs.is_assigned
            reason.append(-state.get_literal(vs, vs.upper_bound, cc))
            assert ass.is_false(reason[-1])
            if not ass.is_fixed(reason[-1]):
                is_fact = False
            reason.append(state.get_literal(vs, vs.lower_bound-1, cc))
            assert ass.is_false(reason[-1])
            if not ass.is_fixed(reason[-1]):
                is_fact = False

        # bounded index
        for co, var in self.constraint.elements[j][1]:
            vs = state.var_state(var)
            if s*co > 0:
                reason.append(-state.get_literal(vs, vs.upper_bound, cc))
                assert ass.is_false(reason[-1])
                if not ass.is_fixed(reason[-1]):
                    is_fact = False
                # add consequence
                ret, lit = state.update_literal(vs, vs.upper_bound-1, cc, is_fact or None)
                if not ret:
                    return False
                reason.append(lit)
                if ass.is_true(reason[-1]):
                    return True
            else:
                reason.append(state.get_literal(vs, vs.lower_bound-1, cc))
                assert ass.is_false(reason[-1])
                if not ass.is_fixed(reason[-1]):
                    is_fact = False
                # add consequence
                ret, lit = state.update_literal(vs, vs.lower_bound, cc, not is_fact and None)
                if not ret:
                    return False
                reason.append(-lit)
                if ass.is_true(reason[-1]):
                    return True

        return cc.add_clause(reason)

    def propagate(self, state, cc):
        """
        Prepagates the distinct constraint.

        See `_propagate` for exmamples what is propagated.
        """
        self._update(state)

        for i in self.todo:
            j = abs(i)-1
            lower, upper = self.assigned[j]
            if lower == upper:
                for k in self.map_upper[upper]:
                    if j != k and not self._propagate(cc, state, 1, j, k):
                        return False
                for k in self.map_lower[lower]:
                    if j != k and not self._propagate(cc, state, -1, j, k):
                        return False
            elif i < 0:
                for k in self.map_upper[upper]:
                    if self.assigned[k][0] == self.assigned[k][1]:
                        if k+1 in self.todo or -k-1 in self.todo:
                            break
                        if not self._propagate(cc, state, 1, k, j):
                            return False
                        break
            else:
                for k in self.map_lower[lower]:
                    if self.assigned[k][0] == self.assigned[k][1]:
                        if k+1 in self.todo or -k-1 in self.todo:
                            break
                        if not self._propagate(cc, state, -1, k, j):
                            return False
                        break
        self.todo.clear()

        return True

    def check_full(self, state):
        """
        This function checks if a constraint is satisfied w.r.t. the final
        values of its integer variables.

        This function should only be called total assignments.
        """
        values = set()
        for value, elements in self.constraint.elements:
            for co, var in elements:
                vs = state.var_state(var)
                assert vs.is_assigned
                value += co*vs.upper_bound

            if value in values:
                return False
            values.add(value)

        return True


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
            if csp.SORT_ELEMENTS:
                constraint.elements.sort(key=lambda cv: -abs(cv[0]))
            for _, var in constraint.elements:
                self._propagator.add_variable(var)
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

        if len(elems) > 2:
            for term in elems:
                for _, var in term[1]:
                    self._propagator.add_variable(var)

            self._propagator.add_constraint(self, Distinct(literal, elems))
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
            if csp.SORT_ELEMENTS:
                self.minimize.elements.sort(key=lambda cv: -abs(cv[0]))

        # add clauses
        for clause in self._clauses:
            if not self._init.add_clause(clause) or not self._init.propagate():
                return False

        return True
