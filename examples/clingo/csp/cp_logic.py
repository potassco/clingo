import clingo, math

max_int = 10
min_int = -10
offset = 0-min_int

class Constraint:
    def parse(self, term):
        if term.type == clingo.TheoryTermType.Number :
            return term.number
        if term.type == clingo.TheoryTermType.Function :
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
            self.rhs = weight = atom.guard[1].number
        self.vars = []
        for i in atom.elements:
            self.vars += self.parse(i.terms[0])

    def __str__(self):
        return "{} <= {}".format(self.vars,self.rhs)
       
class State:
       def __init__(self):
           self.__dl = -1
           self.__ub = {} # var -> [int]
           self.__lb = {} # var -> [int]
           self.__varval2orderlit = {}
           self.__orderlit2varval = {}
       # {decisionLevel -> [lowerBounds,upperBounds]}
       def __str__(self):
           return "{}".format([self.__dl,self.__ub,self.__lb])
       def init_domain(self, vars, true_lit):
           self.__dl = 0
           for v in vars:
               self.__ub.setdefault(v, []).append(max_int)
               self.__lb.setdefault(v, []).append(min_int)
               self.__varval2orderlit[v] = [None]*(max_int-min_int+1)
               self.set_literal(v,max_int,true_lit)
       def set_dl(self, dl):
           assert dl >= self.__dl
           if dl > self.__dl:
               for v in self.__ub.keys():
                   cp_ub = self.__ub[v][self.__dl]
                   cp_lb = self.__lb[v][self.__dl]
                   for i in range(len(self.__ub[v]),dl+1):
                       self.__ub[v].append(cp_ub)
                       self.__lb[v].append(cp_lb)
           self.__dl = dl
       def propagate_orderlits(self, control):
           for v in self.__varval2orderlit:
               lb_lit = (self.__varval2orderlit[v][self.lb(v)-1+offset])
               for value in range(min_int+1,self.lb(v)):
                   if self.__varval2orderlit[v][value-1+offset] != None:
                       if not control.add_clause([lb_lit,-self.__varval2orderlit[v][value-1+offset]]):
                           print("Clause {}<={}, {}>{} failed".format(v,self.lb(v)-1,v,value))
                           return False
                       print("Clause {}<={}, {}>{} posted".format(v,self.lb(v)-1,v,value))
               assert self.__varval2orderlit[v][self.ub(v)+offset] != None
               ub_lit = self.__varval2orderlit[v][self.ub(v)+offset]
               for value in range(self.ub(v)+1, max_int):
                   if self.__varval2orderlit[v][value+offset] != None:
                       if not control.add_clause([-ub_lit,self.__varval2orderlit[v][value+offset]]):
                           print("Clause {}>{}, {}<={} failed".format(v,self.ub(v),v,value))
                           return False
                       print("Clause {}>{}, {}<={} posted".format(v,self.ub(v),v,value))
           return True

       def backtrack(self, dl):
           print("backtrack dl", dl)
           for v in self.__ub:
               self.__ub[v] = self.__ub[v][:dl]
               self.__lb[v] = self.__lb[v][:dl]
           self.__dl = dl-1
       def propagate_true(self, l, c, control):
           clauses = []
           for (a,v) in c.vars:
               lb = c.rhs
               lbs = []
               for (x,var) in c.vars:
                   if (a,v) != (x,var):
                       lb -= x*self.lb(var)
                       if self.lb(var) > min_int:
                           lbs.append((self.__varval2orderlit[var][x-1]))
               ub = max(min(math.floor(lb/a),max_int),min_int)
               if self.__varval2orderlit[v][ub] == None:
                   self.__varval2orderlit[v][ub] = control.add_literal()
               lit = self.__varval2orderlit[v][ub]
               clauses.append([-l,lit,]+lbs)        
           return clauses
       def propagate_false(self, c):
           return []
       def propagate_free(self, c):
           return []
       def lb(self, var):
           return self.__lb[var][self.__dl]
       def ub(self, var):
           return self.__ub[var][self.__dl]
       def set_literal(self, var, value, lit):
           print("Added Literal {}<={} =:= {}".format(var, value, lit))
           self.__varval2orderlit[var][value+offset] = lit
           self.__orderlit2varval.setdefault(lit, []).append((var,value))
       def update_domain(self, order_lit): #literal is always true, may need negation to be found
           if order_lit in self.__orderlit2varval:
               for (var,value) in self.__orderlit2varval[order_lit]:
                   if self.ub(var) > value:
                       print("New upper bound {}<={}".format(var, value))
                       self.__ub[var][self.__dl] = value
                   else:
                       print("Old upper bound {}<={}".format(var, value))
                       
           else:
               assert (-order_lit in self.__orderlit2varval)
               for (var,value) in self.__orderlit2varval[-order_lit]:
                   if self.lb(var) < value+1:
                       print("New lower bound {}>={}".format(var, value+1))
                       self.__lb[var][self.__dl] = value+1
                   else:
                       print("Old lower bound {}>={}".format(var, value+1))

class Propagator:
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
                for (a,v) in c.vars:
                    self.__vars.add(v)
        true_lit = init.add_literal()
        init.add_clause([true_lit])
        self.__state(0).init_domain(self.__vars, true_lit)
        for i in range(1,init.number_of_threads):
           self.__state(i)
           self.__states[thread_id] = self.__state(0)

#                init.add_watch(lit)
    def propagate(self, control, changes):
        print("Propagate: {} on dl {}".format(changes, control.assignment.decision_level))
        s = self.__state(control.thread_id)
        s.set_dl(control.assignment.decision_level)
        for i in changes:
            s.update_domain(i)


    def check(self, control):
        print("Check:")
        change = True
        state = self.__state(control.thread_id)
        state.set_dl(control.assignment.decision_level)
        if not state.propagate_orderlits(control):
            return
        while (change):
            change = False
            for l,cl in self.__l2c.items(): # bad?
                for c in cl:
                    if control.assignment.is_false(l):
                        cl = state.propagate_false(c)
                    if control.assignment.is_true(l):
                        cl = state.propagate_true(l,c,control)
                    if not control.assignment.is_fixed(l):
                       cl = state.propagate_free(c)
                    for clause in cl:
                        change = True
                        print(clause)
                        if not control.add_clause(clause):
                            return
            if not control.propagate():
                return
            if not state.propagate_orderlits(control):
                return
        if control.assignment.is_total:
            return self.check_full(control)
        return
    def check_full(self, control):
        print("Check Full:")
        s = self.__state(control.thread_id)
        for v in self.__vars:
            if s.lb(v) != s.ub(v):
                l = control.add_literal()
                print("Lower {} and upper {} with new {}".format(s.lb(v), s.ub(v), s.lb(v)+ math.floor((s.ub(v)-s.lb(v))/2)))
                s.set_literal(v,s.lb(v)+ math.floor((s.ub(v)-s.lb(v))/2), l)
                control.add_watch(l)
                control.add_watch(-l)
                break
        

    def undo(self, thread_id, assign, changes):
        self.__state(thread_id).backtrack(assign.decision_level)

    def get_assignment(self, thread_id):
        s = self.__state(thread_id)
        return [(str(x),s.lb(x)) for x in self.__vars if s.lb(x) == s.ub(x)] 
