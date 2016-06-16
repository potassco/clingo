#script(python)
class Tag:
    def __init__(self):
        pass

    def init(self, init):
        for atom in init.symbolic_atoms:
            init.add_watch(init.solver_literal(atom.literal))
            init.add_watch(-init.solver_literal(atom.literal))

    def propagate(self, control, changes):
        global add
        if add: control.add_clause([],tag=True)
        return True

    def undo(self, thread_id, assign, undo):
        pass

    def check(self, control):
        return True

def main(prg):
    global add
    add=True
    prg.ground([("base", [])])
    prg.register_propagator(Tag())
    ret=prg.solve()
    add=not add
    ret=prg.solve()

#end.

{a}.
