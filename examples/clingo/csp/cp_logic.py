import clingo, math

max_int = 10
min_int = -10
offset = 0-min_int

class Constraint(object):
    def parse(self, term):
        if term.type == clingo.TheoryTermType.Number:
            return term.number
        if term.type == clingo.TheoryTermType.Function:
            if term.name == "+":
                return self.parse(term.arguments[0]) + self.parse(term.arguments[1])
            if term.name == "*":
                return [(self.parse(term.arguments[0]), str(term.arguments[1]))]
            if term.name == "-":
                return -1*term.arguments[0].number

    def __init__(self, atom):
        if len(atom.guard[1].arguments) > 0:
            self.rhs = -atom.guard[1].arguments[0].number
        else:
            self.rhs = atom.guard[1].number
        self.vars = []
        for i in atom.elements:
            self.vars += self.parse(i.terms[0])

    def __str__(self):
        return "{} <= {}".format(self.vars, self.rhs)


class State(object):
    def __init__(self):
        self.__dl = -1
        self.__ub = {} # var -> [int]
        self.__lb = {} # var -> [int]
        self.__varval2orderlit = {}
        self.__orderlit2varval = {} # {decisionLevel -> [lowerBounds,upperBounds]}

    def __str__(self):
        return "{}".format([self.__dl, self.__ub, self.__lb])

    def init_domain(self, variables, true_lit):
        self.__dl = 0
        for v in variables:
            self.__ub.setdefault(v, []).append(max_int)
            self.__lb.setdefault(v, []).append(min_int)
            self.__varval2orderlit[v] = [None]*(max_int-min_int+1)
            self.set_literal(v, max_int, true_lit)

    def set_dl(self, dl):
        self.backtrack(dl+1)
        if dl > self.__dl:
            for v in self.__ub:
                cp_ub = self.__ub[v][self.__dl]
                cp_lb = self.__lb[v][self.__dl]
                for i in range(len(self.__ub[v]), dl+1):
                    self.__ub[v].append(cp_ub)
                    self.__lb[v].append(cp_lb)
        self.__dl = dl

    def propagate_orderlits(self, control):
        for v in self.__varval2orderlit:
            lb_lit = self.get_literal(v, self.lb(v)-1, control)
            for value in range(min_int+1, self.lb(v)):
                if (self.has_literal(v, value-1) and
                        not control.assignment.is_true(-self.get_literal(v, value-1, control)) and
                        not control.add_clause([lb_lit, -self.get_literal(v, value-1, control)])):
                    return False
            assert self.has_literal(v, self.ub(v))
            ub_lit = self.get_literal(v, self.ub(v), control)
            for value in range(self.ub(v)+1, max_int):
                if (self.has_literal(v, value) and
                        not control.assignment.is_true(self.get_literal(v, value, control)) and
                        not control.add_clause([-ub_lit, self.get_literal(v, value, control)])):
                    return False
        return True

    def backtrack(self, dl):
        if dl > self.__dl:
            return
        for v in self.__ub:
            self.__ub[v] = self.__ub[v][:dl]
            self.__lb[v] = self.__lb[v][:dl]
        self.__dl = dl

    def propagate_true(self, l, c, control):
        clauses = []
        for a, v in c.vars:
            bound = c.rhs
            lbs = []
            for x, var in c.vars:
                if (a, v) != (x, var):
                    if x > 0:
                        bound -= x*self.lb(var)
                        if self.lb(var) > min_int:
                            assert self.has_literal(var, self.lb(var)-1)
                            lbs.append(self.get_literal(var, self.lb(var)-1, control))

                    else:
                        bound -= x*self.ub(var)
                        lbs.append(self.get_literal(var, self.ub(var), control))

            if a > 0:
                ub = max(min(int(math.floor(bound/a)),max_int), min_int)
                lit = self.get_literal(v, ub, control)
            else:
                lb = max(min(int(bound/a), max_int), min_int)
                lit = -self.get_literal(v, lb-1, control)

            if (not any(control.assignment.is_true(x) for x in lbs) and
                    not control.assignment.is_true(lit)):
                clauses.append([-l, lit]+lbs)
        return clauses

    def lb(self, var):
        return self.__lb[var][self.__dl]

    def ub(self, var):
        return self.__ub[var][self.__dl]

    def has_literal(self, var, value):
        return self.__varval2orderlit[var][value+offset] is not None

    def set_literal(self, var, value, lit):
        self.__varval2orderlit[var][value+offset] = lit
        self.__orderlit2varval.setdefault(lit, []).append((var, value))

    def get_literal(self, var, value, control):
        if self.__varval2orderlit[var][value+offset] is None:
            lit = control.add_literal()
            self.set_literal(var, value, lit)
            control.add_watch(lit)
            control.add_watch(-lit)
        return self.__varval2orderlit[var][value+offset]

    def update_domain(self, order_lit): #literal is always true, may need negation to be found
        if order_lit in self.__orderlit2varval:
            for var, value in self.__orderlit2varval[order_lit]:
                if self.ub(var) > value:
                    self.__ub[var][self.__dl] = value
        else:
            assert -order_lit in self.__orderlit2varval
            for var, value in self.__orderlit2varval[-order_lit]:
                if self.lb(var) < value+1:
                    self.__lb[var][self.__dl] = value+1


class Propagator(object):
    def __init__(self):
        self.__l2c = {}    # {literal: [Constraint]}
        self.__c2l = {}    # {Constraint: [literal]}
        self.__states = [] # [threadId : State]
        self.__vars = set()

    def __state(self, thread_id):
        while len(self.__states) <= thread_id:
            self.__states.append(State())
        return self.__states[thread_id]

    def init(self, init):
        init.check_mode = clingo.PropagatorCheckMode.Fixpoint
        for atom in init.theory_atoms:
            term = atom.term
            if term.name == "sum" and len(term.arguments) == 0:
                c = Constraint(atom)
                lit = init.solver_literal(atom.literal)
                self.__l2c.setdefault(lit, []).append(c)
                self.__c2l.setdefault(c, []).append(lit)
                for _, v in c.vars:
                    self.__vars.add(v)
        true_lit = init.add_literal()
        init.add_clause([true_lit])
        self.__state(0).init_domain(self.__vars, true_lit)
        for i in range(1, init.number_of_threads):
            self.__state(i)
            self.__states[i] = self.__state(0)

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
            if s.lb(v) != s.ub(v):
                l = control.add_literal()
                s.set_literal(v, s.lb(v)+int(math.floor((s.ub(v)-s.lb(v))/2)), l)
                control.add_watch(l)
                control.add_watch(-l)
                return

    def undo(self, thread_id, assign, changes):
        self.__state(thread_id).backtrack(assign.decision_level)

    def get_assignment(self, thread_id):
        s = self.__state(thread_id)
        return [(str(x), s.lb(x)) for x in self.__vars if s.lb(x) == s.ub(x)]
