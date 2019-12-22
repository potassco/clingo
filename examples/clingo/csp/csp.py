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

    def push(self):
        self._lower_bound.append(self.lower_bound)
        self._upper_bound.append(self.upper_bound)

    def pop(self):
        assert len(self._lower_bound) > 1 and len(self._upper_bound) > 1
        self._lower_bound.pop()
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

class State(object):
    def __init__(self):
        self._vars = []
        self._var_state = {}
        self._litmap = {}
        self._levels = [0]

    def get_assignment(self, variables):
        vvs = zip(variables, self._vars)
        return [(v, vs.lower_bound) for v, vs in vvs]

    def _state(self, var):
        return self._var_state[var]

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
    def propagate(self, dl, changes):
        assert self._levels[-1] <= dl
        if self._levels[-1] < dl:
            # FIXME: not much effort to make lazy
            for vs in self._vars:
                vs.push()
            self._levels.append(dl)
        for i in changes:
            self._update_domain(i)

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
        assert control.assignment.is_true(l)

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

        # this is necessary to correctly handle empty constraints
        if slack < 0:
            yield lbs

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

    def _propagate_variable(self, control, vs, rng, lit, sign):
        ass = control.assignment
        lit = sign * lit
        for value in rng:
            if not vs.has_literal(value):
                continue
            l = sign * self._get_literal(vs, value, control)
            if ass.is_true(lit) and ass.level(lit) == 0 and ass.level(l) > 0:
                o, l = self._update_literal(vs, value, control, sign > 0)
                o, l = sign * o, sign * l
                if not control.add_clause([o], lock=True):
                    return False
            if ass.is_true(l):
                continue
            if not control.add_clause([-lit, l]):
                return False
        return True

    def propagate_variables(self, control):
        """
        The function propagates order literals depending on the lower and upper
        bound of variables.

        If a variable `v` has lower bound `l`, then all order literals `v<=l'`
        with `l'<l` are made false. Similarly, if a variable `v` has upper
        bound `u`, then all order literals `v<=u'` with `u'<u` are made true.

        Problems
        ========
        - This should be chained.
        - This has to be done over and over because there is no way to
          statically learn the associated clauses.
        - This can as well be done in propagate.
        """
        for vs in self._vars:
            lit = self._get_literal(vs, vs.lower_bound, control)
            rng = range(MIN_INT, vs.lower_bound)
            if not self._propagate_variable(control, vs, rng, lit, -1):
                return False
            assert vs.has_literal(vs.upper_bound)
            lit = self._get_literal(vs, vs.upper_bound, control)
            rng = range(vs.upper_bound+1, MAX_INT)
            if not self._propagate_variable(control, vs, rng, lit, 1):
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
            self._state(i).init_domain(self._vars)

    def propagate(self, control, changes):
        state = self._state(control.thread_id)
        dl = control.assignment.decision_level
        state.propagate(dl, changes)

    def check(self, control):
        size = control.assignment.size
        state = self._state(control.thread_id)

        # TODO: This one is tricky, as this literal is never part of the change
        # list. For now the update simply happens here making sure it is
        # reapplied for every decision level.
        if 1 in state._litmap:
            state._update_domain(1)
        # TODO: can this ever do anything at this point?
        if not state.propagate_variables(control):
            return
        # FIXME: do not iterate over hashtable when order matters!
        for l, constraints in self._l2c.items():
            if control.assignment.is_true(l):
                for c in constraints:
                    for clause in state.propagate_constraint(l, c, control):
                        if not control.add_clause(clause) or not control.propagate():
                            return
            # TODO: only the variables propagated by the clause should be
            # propagated
            if not state.propagate_variables(control):
                return
        # first condition ensures that wait for propagate first to update our
        # domains
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

