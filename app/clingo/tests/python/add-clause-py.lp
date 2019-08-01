#script (python)

import clingo

class Propagator:
    def __init__(self):
        self.step = 0

    def init(self, init):
        a = init.solver_literal(init.symbolic_atoms[clingo.Function("a")].literal)
        b = init.solver_literal(init.symbolic_atoms[clingo.Function("b")].literal)
        if self.step == 0:
            assert(init.add_clause([-a, -b]))
            assert(init.add_clause([-a,  b]))
            assert(init.add_clause([-a, -a, -b]))
            assert(init.add_clause([-a, a]))
        if self.step == 1:
            assert(init.add_clause([b]))
            assert(init.add_clause([b]))
        if self.step == 2:
            assert(not init.add_clause([]))
            assert(not init.add_clause([a]))
            assert(not init.add_clause([b]))
            assert(not init.add_clause([-a]))
            assert(not init.add_clause([-b]))
        self.step += 1

def main(prg):
    prg.register_propagator(Propagator())
    prg.ground([("base", [])])
    prg.solve()
    prg.solve()
    prg.solve()

#end.

{ a; b }.
