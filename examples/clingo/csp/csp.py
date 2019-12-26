import sys
import clingo

MAX_INT = 20
MIN_INT = -20
TRUE_LIT = 1

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
        self.literals = (MAX_INT-MIN_INT)*[None]

    def push_lower(self):
        self._lower_bound.append(self.lower_bound)

    def push_upper(self):
        self._upper_bound.append(self.upper_bound)

    def pop_lower(self):
        assert len(self._lower_bound) > 1
        self._lower_bound.pop()

    def pop_upper(self):
        assert len(self._upper_bound) > 1
        self._upper_bound.pop()

    @property
    def lower_bound(self):
        assert self._lower_bound
        return self._lower_bound[-1]

    @lower_bound.setter
    def lower_bound(self, value):
        assert self._lower_bound
        self._lower_bound[-1] = value

    @property
    def upper_bound(self):
        assert self._upper_bound
        return self._upper_bound[-1]

    @upper_bound.setter
    def upper_bound(self, value):
        assert self._upper_bound
        self._upper_bound[-1] = value

    @property
    def is_assigned(self):
        return self.upper_bound == self.lower_bound

    def has_literal(self, value):
        return (value < MIN_INT or
                value >= MAX_INT or
                self.literals[value - MIN_INT] is not None)

class TodoList(object):
    def __init__(self):
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
        if x not in self:
            self._seen.add(x)
            self._list.append(x)
            return True
        return False

    def clear(self):
        self._seen.clear()
        self._list.clear()

class Level(object):
    def __init__(self, level):
        self.level = level
        # a trail-like data structure would also be possible but then
        # assignments would have to be undone
        self.undo_upper = TodoList()
        self.done_upper = 0
        self.undo_lower = TodoList()
        self.done_lower = 0

class State(object):
    def __init__(self):
        self._vars = []
        self._var_state = {}
        self._litmap = {}
        self._levels = [Level(0)]

    def get_assignment(self, variables):
        vvs = zip(variables, self._vars)
        return [(v, vs.lower_bound) for v, vs in vvs]

    def _push_level(self, level):
        if self._levels[-1].level < level:
            self._levels.append(Level(level))

    def _pop_level(self):
        self._levels.pop()

    def _state(self, var):
        return self._var_state[var]

    @property
    def _level(self):
        return self._levels[-1]

    def _get_literal(self, vs, value, control):
        if value < MIN_INT:
            return -TRUE_LIT
        if value >= MAX_INT:
            return TRUE_LIT
        if not vs.has_literal(value):
            lit = control.add_literal()
            vs.literals[value - MIN_INT] = lit
            self._litmap.setdefault(lit, []).append((vs, value))
            control.add_watch(lit)
            control.add_watch(-lit)
        return vs.literals[value - MIN_INT]

    def _update_literal(self, vs, value, control, truth):
        if value < MIN_INT or value >= MAX_INT or truth is None:
            return None, self._get_literal(vs, value, control)
        lit = TRUE_LIT if truth else -TRUE_LIT
        if not vs.has_literal(value):
            vs.literals[value - MIN_INT] = lit
            self._litmap.setdefault(lit, []).append((vs, value))
            return None, lit
        old = vs.literals[value - MIN_INT]
        if old == lit:
            return None, lit
        vs.literals[value - MIN_INT] = lit
        vec = self._litmap[old]
        assert (vs, value) in vec
        vec.remove((vs, value))
        if not vec:
            assert -old not in self._litmap
            del self._litmap[old]
            control.remove_watch(old)
            control.remove_watch(-old)
        self._litmap.setdefault(lit, []).append((vs, value))
        return old, lit

    # initialization
    def init_domain(self, variables):
        for v in variables:
            vs = VarState()
            self._var_state[v] = vs
            self._vars.append(vs)

    # propagation
    def propagate(self, control, changes):
        ass = control.assignment

        # open a new decision level if necessary
        self._push_level(ass.decision_level)

        # propagate order literals that became true/false
        for lit in changes:
            if not self._update_domain(control, lit):
                return False

        return True

    def _propagate_variable(self, control, vs, value, lit, sign):
        assert vs.has_literal(value)
        ass = control.assignment
        assert ass.is_true(lit)

        # get the literal to propagate
        l = sign * self._get_literal(vs, value, control)

        # on-the-fly simplify
        if ass.level(lit) == 0 and ass.level(l) > 0:
            o, l = self._update_literal(vs, value, control, sign > 0)
            o, l = sign * o, sign * l
            if not control.add_clause([o], lock=True):
                return False

        # propagate the literal
        if not ass.is_true(l) and not control.add_clause([-lit, l]):
            return False

        return True

    def _update_domain(self, control, lit):
        """
        The function updates lower and upper bounds of variables and propagates
        order literals.

        If a variable `v` has lower bound `l`, then the preceding order
        literals `v<=l'` with `l'<l` is made false. Similarly, if a variable
        `v` has upper bound `u`, then the succeeding order literal `v<=u'` with
        `u'<u` is made true.

        Problems
        ========
        - This should be chained.
        - Clauses could be locked.
        - This can as well be done in propagate.
        """
        assert control.assignment.is_true(lit)

        lvl = self._level

        # update and propagate upper bound
        if lit in self._litmap:
            for vs, value in self._litmap[lit]:
                # update upper bound
                if vs.upper_bound > value:
                    if lvl.undo_upper.add(vs):
                        vs.push_upper()
                    vs.upper_bound = value

                # make succeeding literal true
                for succ in range(value+1, MAX_INT):
                    if vs.has_literal(succ):
                        if not self._propagate_variable(control, vs, succ, lit, 1):
                            return False

        # update and propagate lower bound
        if -lit in self._litmap:
            for vs, value in self._litmap[-lit]:
                # update lower bound
                if vs.lower_bound < value+1:
                    if lvl.undo_lower.add(vs):
                        vs.push_lower()
                    vs.lower_bound = value+1

                # make preceeding literal false
                for prev in range(value-1, MIN_INT-1, -1):
                    if vs.has_literal(prev):
                        if not self._propagate_variable(control, vs, prev, lit, -1):
                            return False

        return True

    def propagate_constraint(self, l, c, control):
        """
        This function propagates a constraint that became active because an its
        associated literal became true.

        The function calculates the slack of the constraint w.r.t. to the
        lower/upper bounds of its values. The order values are then propagated
        in such a way that the slack is positive. The trick here is that we can
        use the ordering of variables to restrict the number of propagations.
        For example, for positive coefficients, we just have to enforce the
        smallest order variable that would make the slack non-negative.
        """
        assert not control.assignment.is_false(l)

        # NOTE: recalculation could be avoided by maintaing per clause state
        #       (or at least clauses should be propagated only once)
        slack = c.rhs  # sum of lower bounds (also saw this called slack)
        lbs = []       # lower bound literals
        num_guess = 0  # number of assignments above level 0
        for i, (co, var) in enumerate(c.vars):
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

            assert control.assignment.is_false(lit)
            if control.assignment.level(lit) > 0:
                num_guess += 1
            lbs.append(lit)
        if control.assignment.level(l) > 0:
            num_guess += 1

        lbs.append(-l)

        # this is necessary to correctly handle empty constraints (and do
        # propagation of false constraints)
        if slack < 0:
            yield lbs

        if not control.assignment.is_true(l):
            return True

        for i, (co, var) in enumerate(c.vars):
            vs = self._state(var)

            # adjust the number of guesses if the current literal is a guess
            adjust = 1 if control.assignment.level(lbs[i]) > 0 else 0

            # if all literals are assigned on level 0 then we can associate the
            # value with a true/false literal, taken care of by the extra
            # argument truth to _update_literal
            if co > 0:
                truth = num_guess == adjust or None
                diff = slack+co*vs.lower_bound
                value = diff//co
                old, lit = self._update_literal(vs, value, control, truth)
                if old is not None and not control.add_clause([old], lock=True):
                    return False
            else:
                truth = num_guess > adjust and None
                diff = slack+co*vs.upper_bound
                value = -(diff//-co)
                old, lit = self._update_literal(vs, value-1, control, truth)
                lit = -lit
                if old is not None and not control.add_clause([-old], lock=True):
                    return False

            # the value is chosen in a way that the slack is greater or equal
            # than 0 but also so that it does not exceed the coefficient (in
            # which case the propagation would not be strong enough)
            assert diff - co*value >= 0 and diff - co*value < abs(co)

            if not control.assignment.is_true(lit):
                lbs[i], lit = lit, lbs[i]
                yield lbs
                lbs[i] = lit

    def undo(self):
        lvl = self._level
        for vs in lvl.undo_lower:
            vs.pop_lower()
        lvl.undo_lower.clear()
        for vs in lvl.undo_upper:
            vs.pop_upper()
        self._pop_level()

    # checking
    def check(self, l2c, control):
        lm = self._litmap
        num_fact_next = len(lm.get(1, [])) + len(lm.get(-1, []))

        # Note: We have to loop here because watches for the true/false
        # literals do not fire again.
        while True:
            num_fact = num_fact_next

            # TODO: There is some unnecessary work done here. For each level
            if not self._update_domain(control, 1):
                return False

            # FIXME: do not iterate over hashtable when order matters! Also,
            # only constraints with update variables should be propagated!
            for l, constraints in l2c.items():
                if not control.assignment.is_false(l):
                    for c in constraints:
                        for clause in self.propagate_constraint(l, c, control):
                            if not control.add_clause(clause) or not control.propagate():
                                return False

            num_fact_next = len(lm.get(1, [])) + len(lm.get(-1, []))
            if num_fact == num_fact_next:
                break

        return True

    def check_full(self, control):
        for vs in self._vars:
            if not vs.is_assigned:
                value = lerp(vs.lower_bound, vs.upper_bound)
                self._get_literal(vs, value, control)
                return


class Propagator(object):
    def __init__(self):
        self._l2c = {}     # {literal: [Constraint]}
        self._c2l = {}     # {Constraint: [literal]}
        self._states = []  # [threadId : State]
        self._vars = []    # [str]

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
            state = self._state(i)
            state.init_domain(self._vars)

        ass = init.assignment
        intrail = set()
        trail = []
        trail_offset = 0

        # Note: The initial propagation below, will not introduce any order
        # literals other than true or false.
        while True:
            if not init.propagate():
                return

            # TODO: Trail handling should happen in clingo.
            for var in range(1, ass.max_size):
                if var in intrail:
                    continue
                t = ass.value(var)
                if t is True:
                    trail.append(var)
                    intrail.add(var)
                if t is False:
                    trail.append(-var)
                    intrail.add(var)

            # Note: Initially, the trail is guaranteed to have at least size 1.
            # This ensures that order literals will be propagated.
            if trail_offset == len(trail):
                break
            trail_offset = len(trail)

            for state in self._states:
                if not state.check(self._l2c, init):
                    return

    def propagate(self, control, changes):
        state = self._state(control.thread_id)
        state.propagate(control, changes)

    def check(self, control):
        size = control.assignment.size
        state = self._state(control.thread_id)

        if not state.check(self._l2c, control):
            return

        # TODO: should assignment.is_total be true even after new literals have
        # been added???
        # Make sure that all variables are assigned in the end.
        if size == control.assignment.size and control.assignment.is_total:
            state.check_full(control)

    def undo(self, thread_id, assign, changes):
        self._state(thread_id).undo()

    def get_assignment(self, thread_id):
        return self._state(thread_id).get_assignment(self._vars)


class Application(object):
    def __init__(self):
        self.program_name = "csp"
        self.version = "1.0"
        self.propagator = Propagator()

    def print_assignment(self, model):
        ass = self.propagator.get_assignment(model.thread_id)
        print("Valid assignment for constraints found:")
        print(" ".join("{}={}".format(n, v) for n, v in ass))

    def _parse_min(self, value):
        global MIN_INT
        MIN_INT = int(value)
        return True

    def _parse_max(self, value):
        global MAX_INT
        MAX_INT = int(value)
        return True

    def register_options(self, options):
        group = "CSP Options"
        options.add(group, "min-int", "Minimum integer [-20]", self._parse_min, argument="<i>")
        options.add(group, "max-int", "Maximum integer [20]", self._parse_max, argument="<i>")

    def validate_options(self):
        if MIN_INT > MAX_INT:
            raise RuntimeError("min-int must not be larger than max-int")

    def main(self, prg, files):
        for f in files:
            prg.load(f)
        prg.register_propagator(self.propagator)
        prg.add("base", [], """\
#theory cp {
    constant  { - : 1, unary };
    sum_term {
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left
    };
    &sum/0 : sum_term, {<=}, constant, head
}.
""")

        prg.ground([("base", [])])
        prg.solve(on_model=self.print_assignment)


if __name__ == "__main__":
    sys.exit(int(clingo.clingo_main(Application(), sys.argv[1:])))

