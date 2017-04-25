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
        self.__states = {}

    def check(self, ctl):
        assign = ctl.assignment
        state = self.__states.setdefault(ctl.thread_id, State())
        state.messages = []
        state.invalid = False
        for atom in self.__atoms:
            if assign.is_true(atom.solver_literal):
                state.invalid = True
                for elem in atom.elements:
                    if assign.is_true(elem.solver_literal):
                        state.messages.append(elem.message)

    def invalid(self, thread_id):
        return self.__states[thread_id].invalid

    def messages(self, thread_id):
        return self.__states[thread_id].messages

    def init(self, init):
        for atom in init.theory_atoms:
            if atom.term.name == "cannot" and len(atom.term.arguments) == 0:
                a = Thing()
                a.solver_literal = init.solver_literal(atom.literal)
                a.elements = []
                for elem in atom.elements:
                    e = Thing()
                    t = elem.terms
                    e.message = t[0].name[1:-1]
                    for term in t[1:]:
                        e.message = e.message.replace("%", str(term), 1)
                    e.solver_literal = init.solver_literal(elem.condition_id)
                    a.elements.append(e)
                self.__atoms.append(a)

def main(prg):
    prg.ground([("base", [])])
    # minimize the number of messages
    backend = prg.backend
    lits = []
    for atom in prg.theory_atoms:
        if atom.term.name == "cannot" and len(atom.term.arguments) == 0:
            for elem in atom.elements:
                condition = elem.condition[:]
                condition.append(atom.literal)
                if len(condition) > 1:
                    atom = backend.add_atom()
                    backend.add_rule([atom], condition)
                    condition[0] = atom
                lits.append((condition[0], 1))
    backend.add_minimize(0, lits)
    # solve where the propagator takes care of collecting the messages
    p = Cannot()
    prg.register_propagator(p)
    with prg.solve(yield_=True) as it:
        for m in it:
            if p.invalid(m.thread_id):
                print("ERROR: invalid answer set")
                for msg in p.messages(m.thread_id):
                    print ("- cannot {}".format(msg))

#end.

#theory cannot {
    t { };
    &cannot/0: t, head
}.
