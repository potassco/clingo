"""
Module implementing constraints.
"""

import abc

import clingo
from .solver import AbstractConstraint, AbstractConstraintState
from .base import TRUE_LIT
from .util import TodoList, IntervalSet, abstractproperty
from .parsing import simplify


class SumConstraint(AbstractConstraint):
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
        self.literal = literal
        self.elements = elements
        self.rhs = rhs

    def create_state(self):
        """
        Create thread specific state for the constraint.
        """
        return SumConstraintState(self)


class MinimizeConstraint(AbstractConstraint):
    """
    Class to capture minimize constraints of form `a_0*x_0 + ... + a_n * x_n <= rhs`.

    Members
    =======
    literal  -- Solver literal associated with the constraint.
    elements -- List of integer/string pairs representing coefficient and
                variable.
    """

    def __init__(self):
        self.literal = TRUE_LIT
        self.elements = []
        self.adjust = 0

    def create_state(self):
        """
        Create thread specific state for the constraint.
        """
        return MinimizeConstraintState(self)


class DistinctConstraint(AbstractConstraint):
    """
    Record holding a distinct constraint.
    """
    def __init__(self, literal, elements):
        self.literal = literal
        self.elements = elements

    def create_state(self):
        """
        Create thread specific state for the constraint.
        """
        return DistinctState(self)


class AbstractSumConstraintState(AbstractConstraintState):
    """
    Implements propagation for sum and minimize constraints.
    """
    def __init__(self):
        AbstractConstraintState.__init__(self)
        self.lower_bound = 0
        self.upper_bound = 0

    @abstractproperty
    def elements(self):
        """
        The elements of the constraint.
        """

    @abstractproperty
    def literal(self):
        """
        The literal of the constraint.
        """

    @abstractproperty
    def tagged(self):
        """
        Whether clauses have to be tagged.
        """

    @abc.abstractmethod
    def rhs(self, state):
        """
        The bound of the constraint.
        """

    def attach(self, state):
        """
        Attach the constraint state to the given state.
        """
        self.lower_bound = self.upper_bound = 0
        for co, var in self.elements:
            vs = state.var_state(var)
            state.add_var_watch(var, co, self)
            if co > 0:
                self.lower_bound += vs.lower_bound*co
                self.upper_bound += vs.upper_bound*co
            else:
                self.lower_bound += vs.upper_bound*co
                self.upper_bound += vs.lower_bound*co

    def detach(self, state):
        """
        Detach the constraint frow the given state.
        """
        for co, var in self.elements:
            state.remove_var_watch(var, co, self)

    def undo(self, i, diff):
        """
        Undo the last updates of the bounds of the constraint by the given
        difference.
        """
        if i*diff > 0:
            self.lower_bound -= i*diff
        else:
            self.upper_bound -= i*diff

    def update(self, i, diff):
        """
        Update the bounds of the constraint by the given difference.
        """
        assert i*diff != 0
        if i*diff < 0:
            self.upper_bound += i*diff
            return False
        self.lower_bound += i*diff
        return True

    def _check_state(self, state):
        lower = upper = 0
        for co, var in self.elements:
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

    def _calculate_reason(self, state, cc, slack, vs, co, config):
        # pylint: disable=bad-option-value,chained-comparison
        ass = cc.assignment
        ret = True
        found = 0

        if co > 0:
            current = vs.lower_bound
            # the direct reason literal
            lit = state.get_literal(vs, current-1, cc)
            assert ass.is_false(lit)
            if config.refine_reasons and slack+co < 0 and ass.decision_level > 0:
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
                    if config.refine_introduce and ass.level(lit) == ass.decision_level and value < current:
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
            if config.refine_reasons and slack-co < 0 and ass.decision_level > 0:
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

                    if config.refine_introduce and ass.level(lit) == ass.decision_level and value > current:
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

    def propagate(self, state, cc, config, check_state):
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
        rhs = self.rhs(state)

        # Note: this has a noticible cost because of the shortcuts below
        if check_state and not self.marked_inactive:
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
            for co, var in self.elements:
                vs = state.var_state(var)

                # calculate reason literal
                ret, slack, lit = self._calculate_reason(state, cc, slack, vs, co, config)
                if not ret:
                    return False

                # append the reason literal
                if not ass.is_fixed(lit):
                    reason.append(lit)

            # append the consequence
            reason.append(-self.literal)

            state.mark_inactive(self)
            return cc.add_clause(reason, tag=self.tagged)

        if not ass.is_true(self.literal):
            return True

        for co_r, var_r in self.elements:
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
                for co_a, var_a in self.elements:
                    if var_a == var_r:
                        continue
                    vs_a = state.var_state(var_a)

                    # calculate reason literal
                    ret, slack_r, lit_a = self._calculate_reason(state, cc, slack_r, vs_a, co_a, config)
                    if not ret:
                        return False

                    # append the reason literal
                    if not ass.is_fixed(lit_a):
                        reason.append(lit_a)

                # append the consequence
                guess = reason or self.tagged
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
                if not cc.add_clause(reason, tag=self.tagged):
                    return False

                # minimize constraints cannot be propagated on decision level 0
                assert ass.is_true(lit_r) or self.tagged and ass.decision_level == 0

        return True

    def check_full(self, state):
        """
        This function checks if a constraint is satisfied w.r.t. the final
        values of its integer variables.

        This function should only be called total assignments.
        """
        rhs = self.rhs(state)

        if rhs is None:
            return True

        lhs = 0
        for co, var in self.elements:
            vs = state.var_state(var)
            assert vs.is_assigned
            lhs += co*vs.lower_bound

        if self.marked_inactive:
            assert lhs <= self.upper_bound
        else:
            assert lhs == self.lower_bound
            assert lhs == self.upper_bound

        return lhs <= rhs


class SumConstraintState(AbstractSumConstraintState):
    """
    A translateable constraint state.

    Class Variables
    ======
    tagged           -- True if constraint applies only during current solving step.
    tagged_removable -- True if the constraint can be temporarily removed.
    """

    tagged = False
    tagged_removable = True

    def __init__(self, constraint):
        AbstractSumConstraintState.__init__(self)
        self.constraint = constraint

    def copy(self):
        """
        Return a copy of the constraint state to be used with another state.
        """
        cs = SumConstraintState(self.constraint)
        cs.inactive_level = self.inactive_level
        cs.lower_bound = self.lower_bound
        cs.upper_bound = self.upper_bound
        return cs

    @property
    def literal(self):
        """
        Return the literal of the associated constraint.
        """
        return self.constraint.literal

    @property
    def elements(self):
        """
        Return the elements of the constraint.
        """
        return self.constraint.elements

    def rhs(self, state):
        """
        Return the bound of the constraint
        """
        return self.constraint.rhs

    def _weight_estimate(self, state):
        """
        Estimate the size of the translation in terms of the number of literals
        necessary for the weight constraint.
        """
        estimate = 0
        slack = self.rhs(state)-self.lower_bound
        for co, var in self.elements:
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
        for co, var in self.elements:
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

        return ret, True

    def _clause_estimate(self, state, elements, lower, upper, maximum):
        todo = [(0, 1, lower, upper)]
        estimate = 0

        while todo:
            i, n, lower, upper = todo[-1]

            if n <= 0:
                todo.pop()
                continue

            if i > 0:
                co, var = elements[i-1]

                if co > 0:
                    todo[-1] = (i, n-1, lower+co, upper+co)
                else:
                    todo[-1] = (i, n-1, lower-co, upper-co)

                estimate -= 1
            else:
                todo.pop()

            if lower < 0:
                estimate += 1
                if estimate >= maximum:
                    return False
                continue

            assert upper < 0 and i < len(elements)

            co, var = elements[i]
            vs = state.var_state(var)

            if co > 0:
                diff_lower = upper+co*vs.upper_bound
                diff_upper = lower+co*vs.lower_bound
                value_lower = max(vs.lower_bound, diff_lower//co+1)
                value_upper = min(vs.upper_bound, diff_upper//co+1)
                lower = lower-co*(value_upper-vs.lower_bound)
                upper = upper+co*(vs.upper_bound-value_upper)
            else:
                diff_upper = upper+co*vs.lower_bound
                diff_lower = lower+co*vs.upper_bound
                value_lower = max(vs.lower_bound, -(diff_lower//-co)-1)
                value_upper = min(vs.upper_bound, -(diff_upper//-co)-1)
                lower = lower-co*(value_lower-vs.upper_bound)
                upper = upper+co*(vs.lower_bound-value_lower)

            n = value_upper-value_lower+1
            estimate += n
            if estimate >= maximum:
                return False
            todo.append((i+1, n, lower, upper))

        return True

    def _clause_translate(self, cc, state, elements, lower, upper, config):
        todo = [(0, 0, 0, 0, lower, upper)]
        clause = [0] * (len(elements) + 1)

        while todo:
            i, j, value_lower, value_upper, lower, upper = todo[-1]

            if value_lower > value_upper:
                todo.pop()
                continue

            if i > 0:
                co, var = elements[i-1]
                vs = state.var_state(var)
                if co > 0:
                    todo[-1] = (i, j, value_lower, value_upper-1, lower+co, upper+co)
                    lit = state.get_literal(vs, value_upper, cc)
                else:
                    todo[-1] = (i, j, value_lower+1, value_upper, lower-co, upper-co)
                    lit = -state.get_literal(vs, value_lower, cc)
            else:
                todo.pop()
                lit = -TRUE_LIT if cc.assignment.is_false(-self.literal) else -self.literal

            if lit != -TRUE_LIT:
                clause[j] = lit
                j += 1

            if lower < 0:
                if not config.literals_only and not cc.add_clause(clause[:j]):
                    return False
                continue

            assert upper < 0 and i < len(elements)

            co, var = elements[i]
            vs = state.var_state(var)

            if co > 0:
                diff_lower = upper+co*vs.upper_bound
                diff_upper = lower+co*vs.lower_bound
                value_lower = max(vs.lower_bound-1, diff_lower//co)
                value_upper = min(vs.upper_bound-1, diff_upper//co)
                lower = lower-co*(value_upper-vs.lower_bound+1)
                upper = upper+co*(vs.upper_bound-value_upper-1)
            else:
                diff_lower = lower+co*vs.upper_bound
                diff_upper = upper+co*vs.lower_bound
                value_lower = max(vs.lower_bound, -(diff_lower//-co)-1)
                value_upper = min(vs.upper_bound, -(diff_upper//-co)-1)
                upper = upper+co*(vs.lower_bound-value_lower)
                lower = lower-co*(value_lower-vs.upper_bound)

            assert vs.lower_bound <= value_upper+1
            assert value_lower <= vs.upper_bound
            todo.append((i+1, j, value_lower, value_upper, lower, upper))

        return True

    def translate(self, cc, state, config, added):
        """
        Translate a constraint to clauses or weight constraints.
        """
        ass = cc.assignment

        if self.tagged:
            return True, False

        rhs = self.rhs(state)
        if ass.is_false(self.literal) or self.upper_bound <= rhs:
            return True, True

        lower = rhs-self.lower_bound
        upper = rhs-self.upper_bound

        # Note: otherwise propagation is broken
        assert lower >= 0

        # translation to clauses
        if not config.sort_constraints:
            elements = sorted(self.elements, key=lambda x: -abs(x[0]))
        else:
            elements = self.elements

        if self._clause_estimate(state, elements, lower, upper, config.clause_limit):
            ret = self._clause_translate(cc, state, elements, lower, upper, config)
            return ret, not config.literals_only

        # translation to weight constraints
        if self._weight_estimate(state) < config.weight_constraint_limit:
            return self._weight_translate(cc, state, lower)

        return True, False


class MinimizeConstraintState(AbstractSumConstraintState):
    """
    A translateable minimize constraint state.

    Class Variables
    ======
    tagged           -- True if constraint applies only during current solving step.
    tagged_removable -- True if the constraint can be temporarily removed.
    """

    tagged = True
    tagged_removable = False

    def __init__(self, constraint):
        AbstractSumConstraintState.__init__(self)
        self.constraint = constraint

    def copy(self):
        """
        Return a copy of the constraint state to be used with another state.
        """
        cs = MinimizeConstraintState(self.constraint)
        cs.inactive_level = self.inactive_level
        cs.lower_bound = self.lower_bound
        cs.upper_bound = self.upper_bound
        return cs

    @property
    def literal(self):
        """
        Return the literal of the associated constraint.
        """
        return self.constraint.literal

    @property
    def elements(self):
        """
        Return the elements of the constraint.
        """
        return self.constraint.elements

    def rhs(self, state):
        """
        Return the bound of the constraint
        """
        return state.minimize_bound

    def translate(self, cc, state, config, added):
        """
        Translate the minimize constraint into clasp's minimize constraint.
        """
        if not config.translate_minimize:
            return True, False

        cc.add_minimize(TRUE_LIT, -self.constraint.adjust, 0)
        for co, var in self.constraint.elements:
            vs = state.var_state(var)
            cc.add_minimize(TRUE_LIT, co*vs.min_bound, 0)
            for v in range(vs.min_bound, vs.max_bound):
                cc.add_minimize(-state.get_literal(vs, v, cc), co, 0)
        return True, True


class DistinctState(AbstractConstraintState):
    """
    Capture the state of a distinct constraint.

    Class Variables
    ======
    tagged_removable -- True if the constraint can be temporarily removed.
    """

    tagged_removable = True

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
        ds.dirty = self.dirty.copy()
        return ds

    @property
    def literal(self):
        """
        Return the literal of the associated constraint.
        """
        return self.constraint.literal

    def _init(self, state, i):
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

    def attach(self, state):
        """
        Attach the distinct constraint to the state.
        """
        for i, (_, elements) in enumerate(self.constraint.elements):
            self._init(state, i)
            for co, var in elements:
                state.add_var_watch(var, i+1 if co > 0 else -i-1, self)

    def detach(self, state):
        """
        Detach the constraint frow the given state.
        """
        for i, (_, elements) in enumerate(self.constraint.elements):
            for co, var in elements:
                state.remove_var_watch(var, i+1 if co > 0 else -i-1, self)

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
            self._init(state, i)
        self.dirty.clear()

    def _estimate(self):
        """
        Estimate the translation cost of the constraint in terms of the
        required number of weight constraints.
        """
        cost = 0

        intervals = IntervalSet()
        for lower, upper in self.assigned.values():
            intervals.add(lower, upper+1)
        for lower, upper in intervals:
            cost += upper - lower

        return cost

    def _domain(self, state, lower, elements):
        """
        Calculate the domain of a term.
        """
        values = IntervalSet()

        values.add(lower, lower+1)
        for co, var in elements:
            current = values.copy()
            cs = state.var_state(var)
            add = abs(co)
            for _ in range(cs.lower_bound, cs.upper_bound):
                for l, u in current:
                    values.add(l+add, u+add)
                add += abs(co)

        return values

    def _var(self, state, lower, upper, elements, added):
        """
        Introduce a variable and make it equal to the term.
        """
        assert elements

        if len(elements) == 1:
            return elements[0][1]

        var = state.add_variable(lower, upper)
        elems = [(-1, var)] + elements
        added.append(SumConstraint(TRUE_LIT, elems, 0))
        added.append(SumConstraint(TRUE_LIT, [(-c, v) for c, v in elems], 0))
        return var

    def translate(self, cc, state, config, added):
        """
        Translate small enough distinct constraints to weight constraints.
        """
        if self._estimate() >= config.distinct_limit:
            return True, False

        # compute domain of terms and identify values involved in at least two terms
        union = set()
        counts = {}
        elem_values = []
        for i, (_, elements) in enumerate(self.constraint.elements):
            elem_values.append(self._domain(state, self.assigned[i][0], elements))
            for value in elem_values[-1].enum():
                if value not in counts:
                    counts[value] = 0
                counts[value] += 1
                if counts[value] > 1:
                    union.add(value)

        # calculate variables for terms avoiding unnecessary variables
        elem_vars = []
        for (fixed, elements), (lower, upper), values in zip(self.constraint.elements, self.assigned.values(), elem_values):
            var = None
            for value in values.enum():
                if value not in union:
                    continue
                var = self._var(state, lower, upper, elements, added)
                break
            elem_vars.append((fixed, var))

        # add weight constraints
        for value in sorted(union):
            # variables that have to be different
            wlits = []
            for (fixed, var), values in zip(elem_vars, elem_values):
                if value not in values:
                    continue
                lit = TRUE_LIT
                if var is not None:
                    # lit == var<=value && var>=value
                    #     == var<=value && not var<=value-1
                    vs = state.var_state(var)
                    a = state.get_literal(vs, value-fixed, cc)
                    b = -state.get_literal(vs, value-fixed-1, cc)
                    if a == TRUE_LIT:
                        lit = b
                    elif b == -TRUE_LIT:
                        lit = a
                    else:
                        lit = cc.add_literal()
                        cc.add_clause([-a, -b, lit])
                        cc.add_clause([a, -lit])
                        cc.add_clause([b, -lit])
                wlits.append((lit, 1))

            assert len(wlits) > 1
            cc.add_weight_constraint(self.literal, wlits, 1, 1)

        return True, True

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

    def propagate(self, state, cc, config, check_state):
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


class ConstraintBuilder(object):
    """
    CSP builder to use with the parse_theory function.
    """
    def __init__(self, cc, propagator, minimize):
        self.cc = cc
        self._propagator = propagator
        self._minimize = minimize

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
        Adds a simplistic translation for a distinct constraint.

        For each x_i, x_j with i < j in elems add
          `literal => &sum { x_i } != x_j`
        where each x_i is of form (rhs_i, term_i) and term_i is a list of
        coefficients and variables; x_i corresponds to the linear term
          `term - rhs`.
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
