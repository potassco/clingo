import clingo

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
            self.__rhs = -atom.guard[1].arguments[0].number
        else:
            self.__rhs = weight = atom.guard[1].number
        self.__vars = []
        for i in atom.elements:
            self.__vars += self.parse(i.terms[0])

    def __str__(self):
        return "{} <= {}".format(self.__vars,self.__rhs)
    def vars(self):
        return self.__vars
       
class State:
       def __init__(self):
           self.__dl = -1
           self.__ub = []
           self.__lb = []
       # {decisionLevel -> [lowerBounds,upperBounds]}
       def __str__(self):
           return "{}".format([self.__dl,self.__ub,self.__lb])
       def set_dl(self, dl):
           for i in range(len(self.__ub),dl+1):
               self.__ub.append({})
               self.__lb.append({})
           self.__dl = dl
       def set_ub(self, var, value):
           self.__ub[self.__dl][var] = value
       def set_lb(self, var, value):
           self.__lb[self.__dl][var] = value
       def backtrack(self, dl):
           print("backtrack dl", dl)
           self.__ub = self.__ub[:dl]
           self.__lb = self.__lb[:dl]
           self.__dl = -1
       def propagate_true(self, c):
           return []
       def propagate_false(self, c):
           return []
       def propagate_free(self, c):
           return []
       def lb(self, var):
           if var in self.__lb[self.__dl]:
               return self.__lb[self.__dl][var]
           else:
               return None
       def ub(self, var):
           if var in self.__ub[self.__dl]:
               return self.__ub[self.__dl][var]
           else:
               return None
          

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
                for (a,v) in c.vars():
                    self.__vars.add(v)

#                init.add_watch(lit)

    def check(self, control):
        change = True
        state = self.__state(control.thread_id)
        state.set_dl(control.assignment.decision_level)
        while (change):
            change = False
            for l,c in self.__l2c.items(): # bad?
                if control.assignment.is_false(l):
                    cl = state.propagate_false(c)
                if control.assignment.is_true(l):
                    cl = state.propagate_true(c)
                if not control.assignment.is_fixed(l):
                    cl = state.propagate_free(c)
                for clause in cl:
                    change = True
                    control.add_clause(clause)
            if not control.propagate():
                return
        return

    def undo(self, thread_id, assign, changes):
        self.__state(thread_id).backtrack(assign.decision_level)

    def get_assignment(self, thread_id):
        s = self.__state(thread_id)
        return [(str(x),s.lb(x)) for x in self.__vars if s.lb(x) == s.ub(x) and s.lb(x)!=None] 
