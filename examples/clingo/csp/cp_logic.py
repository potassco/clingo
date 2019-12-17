import math
import clingo

MAX_INT = 10
MIN_INT = -10
OFFSET = 0-MIN_INT

def match(term, name, arity):
    return (term.type == clingo.TheoryTermType.Function and
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
                for x in self._parse_elem(elem.terms[0]): yield x
            else:
                raise RuntimeError("Invalid Syntax")

    def _parse_elem(self, term):
        if match(term, "+", 2):
            for x in self._parse_elem(term.arguments[0]): yield x
            for x in self._parse_elem(term.arguments[1]): yield x
        elif match(term, "*", 2):
            yield self._parse_num(term.arguments[0]), self._parse_var(term.arguments[1])
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
        self.upper_bound = [MAX_INT]
        self.lower_bound = [MIN_INT]
        # FIXME: needs better container
        self.literals = (MAX_INT-MIN_INT+1)*[None]

    # TODO: meh
    def set_dl(self, old, new):
        if new > old:
            l = self.lower_bound[old]
            u = self.upper_bound[old]
            while len(self.lower_bound) <= new:
                self.lower_bound.append(l)
                self.upper_bound.append(u)
        else:
            del self.lower_bound[new:]
            del self.upper_bound[new:]

    def get_lb(self, dl):
        return self.lower_bound[dl]

    def set_lb(self, dl, value):
        self.lower_bound[dl] = value

    def get_ub(self, dl):
        return self.upper_bound[dl]

    def set_ub(self, dl, value):
        self.upper_bound[dl] = value

    def add_literal(self, value, lit):
        self.literals[value - MIN_INT] = lit

    def get_literal(self, value):
        return self.literals[value - MIN_INT]

    def has_literal(self, value):
        return self.literals[value - MIN_INT] is not None

class State(object):
    def __init__(self):
        self.__dl = -1
        self._vars = []
        self._var_state = {}
        self._litmap = {}

    def __str__(self):
        return "{}".format([self.__dl])

    def init_domain(self, variables, true_lit):
        self.__dl = 0
        self._vars = variables
        for v in self._vars:
            self._var_state[v] = VarState()
            self.add_literal(v, MAX_INT, true_lit)

    def propagate_orderlits(self, control):
        for v in self._vars:
            lb_lit = self.get_literal(v, self.get_lb(v)-1, control)
            for value in range(MIN_INT+1, self.get_lb(v)):
                if (self.has_literal(v, value-1) and
                        not control.assignment.is_true(-self.get_literal(v, value-1, control)) and
                        not control.add_clause([lb_lit, -self.get_literal(v, value-1, control)])):
                    return False
            assert self.has_literal(v, self.get_ub(v))
            ub_lit = self.get_literal(v, self.get_ub(v), control)
            for value in range(self.get_ub(v)+1, MAX_INT):
                if (self.has_literal(v, value) and
                        not control.assignment.is_true(self.get_literal(v, value, control)) and
                        not control.add_clause([-ub_lit, self.get_literal(v, value, control)])):
                    return False
        return True

    # TODO: meh
    def set_dl(self, dl):
        self.backtrack(dl+1) # why? undo is guaranteed
        if dl > self.__dl:
            for v in self._vars:
                self._var_state[v].set_dl(self.__dl, dl)
        self.__dl = dl

    # TODO: meh
    def backtrack(self, dl):
        if dl > self.__dl:
            return
        for v in self._vars:
            self._var_state[v].set_dl(self.__dl, dl)
        self.__dl = dl

    def propagate_true(self, l, c, control):
        clauses = []
        for a, v in c.vars:
            bound = c.rhs
            lbs = []
            for x, var in c.vars:
                if (a, v) != (x, var):
                    if x > 0:
                        bound -= x*self.get_lb(var)
                        if self.get_lb(var) > MIN_INT:
                            assert self.has_literal(var, self.get_lb(var)-1)
                            lbs.append(self.get_literal(var, self.get_lb(var)-1, control))

                    else:
                        bound -= x*self.get_ub(var)
                        lbs.append(self.get_literal(var, self.get_ub(var), control))

            if a > 0:
                ub = max(min(int(math.floor(bound/a)),MAX_INT), MIN_INT)
                lit = self.get_literal(v, ub, control)
            else:
                lb = max(min(int(bound/a), MAX_INT), MIN_INT)
                lit = -self.get_literal(v, lb-1, control)

            if (not any(control.assignment.is_true(x) for x in lbs) and
                    not control.assignment.is_true(lit)):
                clauses.append([-l, lit]+lbs)
        return clauses

    def get_lb(self, var):
        return self._var_state[var].get_lb(self.__dl)

    def set_lb(self, var, value):
        self._var_state[var].set_lb(self.__dl, value)

    def get_ub(self, var):
        return self._var_state[var].get_ub(self.__dl)

    def set_ub(self, var, value):
        self._var_state[var].set_ub(self.__dl, value)

    def has_literal(self, var, value):
        return self._var_state[var].has_literal(value)

    def add_literal(self, var, value, lit):
        self._litmap.setdefault(lit, []).append((var, value))
        self._var_state[var].add_literal(value, lit)

    def get_literal(self, var, value, control):
        if not self._var_state[var].has_literal(value):
            lit = control.add_literal()
            self.add_literal(var, value, lit)
            control.add_watch(lit)
            control.add_watch(-lit)
        return self._var_state[var].get_literal(value)

    def update_domain(self, order_lit): #literal is always true, may need negation to be found
        if order_lit in self._litmap:
            for var, value in self._litmap[order_lit]:
                if self.get_ub(var) > value:
                    self.set_ub(var, value)
        else:
            assert -order_lit in self._litmap
            for var, value in self._litmap[-order_lit]:
                if self.get_lb(var) < value+1:
                    self.set_lb(var, value+1)


class Propagator(object):
    def __init__(self):
        self.__l2c = {}    # {literal: [Constraint]}
        self.__c2l = {}    # {Constraint: [literal]}
        self.__states = [] # [threadId : State]
        self.__vars = []

    def __state(self, thread_id):
        while len(self.__states) <= thread_id:
            self.__states.append(State())
        return self.__states[thread_id]

    def init(self, init):
        init.check_mode = clingo.PropagatorCheckMode.Fixpoint

        variables = set()
        for atom in init.theory_atoms:
            term = atom.term
            if term.name == "sum" and len(term.arguments) == 0:
                c = Constraint(atom)
                lit = init.solver_literal(atom.literal)
                self.__l2c.setdefault(lit, []).append(c)
                self.__c2l.setdefault(c, []).append(lit)
                for _, v in c.vars:
                    variables.add(v)
        self.__vars = list(sorted(variables))

        # Note: literal 1 is guaranteed to be (true or false)
        #       but unlike the literal here it won't be propagated
        true_lit = init.add_literal()
        init.add_clause([true_lit])
        for i in range(len(self.__states), init.number_of_threads):
            self.__state(i).init_domain(self.__vars, true_lit)

    def propagate(self, control, changes):
        s = self.__state(control.thread_id)
        s.set_dl(control.assignment.decision_level)
        for i in changes:
            s.update_domain(i)

    def check(self, control):
        size = control.assignment.size
        state = self.__state(control.thread_id)
        state.set_dl(control.assignment.decision_level)
        if not state.propagate_orderlits(control):
            return
        # FIXME: do not iterate over hashtable when order matters!
        for l, constraints in self.__l2c.items(): # bad?
            for c in constraints:
                if control.assignment.is_true(l):
                    for clause in state.propagate_true(l, c, control):
                        if not control.add_clause(clause):
                            return
            if not control.propagate():
                return
            if not state.propagate_orderlits(control):
                return
        if size == control.assignment.size and control.assignment.is_total: # first condition ensures that wait for propagate first to update our domains
            self.check_full(control)

    def check_full(self, control):
        s = self.__state(control.thread_id)
        for v in self.__vars:
            if s.get_lb(v) != s.get_ub(v):
                l = control.add_literal()
                s.add_literal(v, s.get_lb(v)+int(math.floor((s.get_ub(v)-s.get_lb(v))/2)), l)
                control.add_watch(l)
                control.add_watch(-l)
                return

    def undo(self, thread_id, assign, changes):
        self.__state(thread_id).backtrack(assign.decision_level)

    def get_assignment(self, thread_id):
        s = self.__state(thread_id)
        return [(str(x), s.get_lb(x)) for x in self.__vars if s.get_lb(x) == s.get_ub(x)]
