import math
import clingo

MAX_INT = 20
MIN_INT = -20
OFFSET = 0-MIN_INT
TRUE_LIT = 1

def clamp(x, l=MIN_INT, u=MAX_INT):
    return min(max(x, l), u)

def lerp(x, y):
    # NOTE: integer division with floor
    return x + (y - x) // 2

def match(term, name, arity):
    return (term.type in (clingo.TheoryTermType.Function, clingo.TheoryTermType.Symbol) and
            term.name == name and
            len(term.arguments) == arity)

class Constraint(object):
    def __init__(self, atom):
        self.rhs = self._parse_num(atom.guard[1])
        self.vars = list(self._parse_elems(atom.elements))

    def _parse_elems(self, elems):
        for elem in elems:
            if len(elem.terms) == 1 and len(elem.condition) == 0:
                # python 3 has yield from
                for x in self._parse_elem(elem.terms[0]):
                    yield x
            else:
                raise RuntimeError("Invalid Syntax")

    def _parse_elem(self, term):
        if match(term, "+", 2):
            for x in self._parse_elem(term.arguments[0]):
                yield x
            for x in self._parse_elem(term.arguments[1]):
                yield x
        elif match(term, "*", 2):
            num = self._parse_num(term.arguments[0])
            var = self._parse_var(term.arguments[1])
            yield num, var
        else:
            raise RuntimeError("Invalid Syntax")

    def _parse_num(self, term):
        if term.type == clingo.TheoryTermType.Number:
            return term.number
        elif (term.type == clingo.TheoryTermType.Symbol and
                term.symbol.type == clingo.SymbolType.Number):
            return term.symbol.number
        elif match(term, "-", 1):
            return -self._parse_num(term.arguments[0])
        elif match(term, "+", 1):
            return +self._parse_num(term.arguments[0])
        else:
            raise RuntimeError("Invalid Syntax")

    def _parse_var(self, term):
        return str(term)

    def __str__(self):
        return "{} <= {}".format(self.vars, self.rhs)


class VarState(object):
    def __init__(self):
        self._upper_bound = [MAX_INT]
        self._lower_bound = [MIN_INT]
        # FIXME: needs better container
        self.literals = (MAX_INT-MIN_INT+1)*[None]

    def push(self):
        self._lower_bound.append(self.lower_bound)
        self._upper_bound.append(self.upper_bound)

    def pop(self):
        self._lower_bound.pop()
        self._upper_bound.pop()

    @property
    def lower_bound(self):
        return self._lower_bound[-1]

    @lower_bound.setter
    def lower_bound(self, value):
        self._lower_bound[-1] = value

    @property
    def upper_bound(self):
        return self._upper_bound[-1]

    @upper_bound.setter
    def upper_bound(self, value):
        self._upper_bound[-1] = value

    def has_literal(self, value):
        return self.literals[value - MIN_INT] is not None

class State(object):
    def __init__(self):
        self._vars = []
        self._var_state = {}
        self._litmap = {}
        self._levels = [0]

    def get_assignment(self, variables):
        vvs = zip(variables, self._vars)
        return [(v, vs.lower_bound) for v, vs in vvs if vs.lower_bound == vs.upper_bound]

    def _state(self, var):
        return self._var_state[var]

    def _get_literal(self, vs, value, control, watch=True):
        if not vs.has_literal(value):
            lit = TRUE_LIT if control is None else control.add_literal()
            vs.literals[value - MIN_INT] = lit
            self._litmap.setdefault(lit, []).append((vs, value))
            if watch:
                control.add_watch(lit)
                control.add_watch(-lit)
        return vs.literals[value - MIN_INT]

    # initialization
    def init_domain(self, variables):
        for v in variables:
            vs = VarState()
            self._var_state[v] = vs
            self._get_literal(vs, MAX_INT, None, False)
            self._vars.append(vs)

    # propagation
    def propagate(self, dl, changes):
        assert self._levels[-1] <= dl
        if self._levels[-1] < dl:
            # FIXME: not much effort to make lazy
            for vs in self._vars:
                vs.push()
            self._levels.append(dl)
        for i in changes:
            self._update_domain(i)

    #literal is always true, may need negation to be found
    def _update_domain(self, order_lit):
        if order_lit in self._litmap:
            for vs, value in self._litmap[order_lit]:
                if vs.upper_bound > value:
                    vs.upper_bound = value
        else:
            assert -order_lit in self._litmap
            for vs, value in self._litmap[-order_lit]:
                if vs.lower_bound < value+1:
                    vs.lower_bound = value+1

    def propagate_true(self, l, c, control):
        """
        TODO: this guy needs documentation
        """
        assert control.assignment.is_true(l)

        # NOTE: recalculation could be avoided by maintaing per clause state
        #       (or at least clauses should be propagated only once)
        slb = c.rhs  # sum of lower bounds
        lbs = []     # lower bound literals
        tri = -1     # index of true literal (if any)
        for i, (co, var) in enumerate(c.vars):
            vs = self._state(var)
            lit = 0
            if co > 0:
                slb -= co*vs.lower_bound
                if vs.lower_bound > MIN_INT:
                    assert vs.has_literal(vs.lower_bound-1)
                    lit = self._get_literal(vs, vs.lower_bound-1, control)
            else:
                slb -= co*vs.upper_bound
                lit = self._get_literal(vs, vs.upper_bound, control)

            if lit != 0:
                if control.assignment.is_true(lit):
                    if tri >= 0:
                        # TODO: not sure this can happen without
                        #       understanding the algorithm better
                        return
                    tri = i
                lbs.append(lit)
        lbs.append(-l)

        it = enumerate(c.vars) if tri < 0 else ((tri, (c.vars[tri])),)
        for i, (co, var) in it:
            vs = self._state(var)
            if co > 0:
                bound = slb+co*vs.lower_bound
                value = clamp(bound//co)
                lit = self._get_literal(vs, value, control)
            else:
                bound = slb+co*vs.upper_bound
                value = clamp(-(bound//-co))
                lit = -self._get_literal(vs, value-1, control)

            if not control.assignment.is_true(lit):
                lbs[i], lit = lit, lbs[i]
                yield lbs
                lbs[i] = lit

    def propagate_orderlits(self, control):
        for vs in self._vars:
            lb_lit = self._get_literal(vs, vs.lower_bound-1, control)
            for value in range(MIN_INT+1, vs.lower_bound):
                if (vs.has_literal(value-1) and
                        not control.assignment.is_true(-self._get_literal(vs, value-1, control)) and
                        not control.add_clause([lb_lit, -self._get_literal(vs, value-1, control)])):
                    return False
            assert vs.has_literal(vs.upper_bound)
            ub_lit = self._get_literal(vs, vs.upper_bound, control)
            for value in range(vs.upper_bound+1, MAX_INT):
                if (vs.has_literal(value) and
                        not control.assignment.is_true(self._get_literal(vs, value, control)) and
                        not control.add_clause([-ub_lit, self._get_literal(vs, value, control)])):
                    return False
        return True

    def undo(self):
        # FIXME: not much effort to make lazy
        for vs in self._vars:
            vs.pop()
        self._levels.pop()

    # checking
    def check_full(self, control):
        for vs in self._vars:
            if vs.lower_bound != vs.upper_bound:
                value = lerp(vs.lower_bound, vs.upper_bound)
                self._get_literal(vs, value, control)
                return


class Propagator(object):
    def __init__(self):
        self._l2c = {}    # {literal: [Constraint]}
        self._c2l = {}    # {Constraint: [literal]}
        self._states = [] # [threadId : State]
        self._vars = []   # [str]

    def _state(self, thread_id):
        while len(self._states) <= thread_id:
            self._states.append(State())
        return self._states[thread_id]

    def init(self, init):
        init.check_mode = clingo.PropagatorCheckMode.Fixpoint

        variables = set()
        for atom in init.theory_atoms:
            if match(atom.term, "sum", 0):
                c = Constraint(atom)
                lit = init.solver_literal(atom.literal)
                self._l2c.setdefault(lit, []).append(c)
                self._c2l.setdefault(c, []).append(lit)
                for _, v in c.vars:
                    variables.add(v)
        self._vars = list(sorted(variables))

        for i in range(len(self._states), init.number_of_threads):
            self._state(i).init_domain(self._vars)

    def propagate(self, control, changes):
        state = self._state(control.thread_id)
        dl = control.assignment.decision_level
        state.propagate(dl, changes)

    def check(self, control):
        size = control.assignment.size
        state = self._state(control.thread_id)
        if not state.propagate_orderlits(control):
            return
        # FIXME: do not iterate over hashtable when order matters!
        for l, constraints in self._l2c.items():
            for c in constraints:
                if control.assignment.is_true(l):
                    for clause in state.propagate_true(l, c, control):
                        if not control.add_clause(clause):
                            return
            if not control.propagate():
                return
            if not state.propagate_orderlits(control):
                return
        # first condition ensures that wait for propagate first to update our domains
        if size == control.assignment.size and control.assignment.is_total:
            state.check_full(control)

    def undo(self, thread_id, assign, changes):
        self._state(thread_id).undo()

    def get_assignment(self, thread_id):
        return self._state(thread_id).get_assignment(self._vars)
