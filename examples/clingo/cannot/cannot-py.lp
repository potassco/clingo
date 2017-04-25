#script (python)

import clingo

class State:
    def __init__(self):
        self.messages = []
        self.invalid = False

class Thing:
    def __init__(self):
        pass

class Cannot:
    def __init__(self):
        self.__atoms = []
        self.states = {}

    def add_minimize(self, backend):
        lits = []
        for atom in self.__atoms:
            for elem in atom.elements:
                condition = elem.condition[:]
                condition.append(atom.literal)
                if len(condition) > 1:
                    atom = backend.add_atom()
                    backend.add_rule([atom], condition)
                    condition[0] = atom
                lits.append((condition[0], 1))
        backend.add_minimize(0, lits)

    def check(self, ctl):
        assign = ctl.assignment
        state = self.states.setdefault(ctl.thread_id, State())
        state.messages = []
        state.invalid = False
        for atom in self.__atoms:
            if assign.is_true(atom.solver_literal):
                state.invalid = True
                for elem in atom.elements:
                    if assign.is_true(elem.solver_literal):
                        state.messages.append(elem.message)

    def init(self, init):
        for atom in init.theory_atoms:
            a = Thing()
            a.literal = atom.literal
            a.solver_literal = init.solver_literal(atom.literal)
            a.elements = []
            for elem in atom.elements:
                e = Thing()
                t = elem.terms
                e.message = t[0].name[1:-1]
                for term in t[1:]:
                    e.message = e.message.replace("%", str(term), 1)
                e.condition = elem.condition
                e.solver_literal = init.solver_literal(elem.condition_id)
                a.elements.append(e)
            self.__atoms.append(a)

def main(prg):
    p = Cannot()
    prg.register_propagator(p)
    prg.ground([("base", [])])
    # just to initialize the propagator
    dummy = clingo.Function("a")
    prg.solve(assumptions=[(dummy, True), (dummy, False)])
    # minimize the number of messages
    p.add_minimize(prg.backend)
    # the actual solve
    with prg.solve(yield_=True) as it:
        for m in it:
            state = p.states[m.thread_id]
            if state.invalid:
                print("ERROR: invalid answer set")
                for msg in state.messages:
                    print ("- cannot {}".format(msg))

#end.

#theory cannot {
    t { };
    &cannot/0: t, head
}.
