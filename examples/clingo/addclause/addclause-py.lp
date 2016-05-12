#script (python)

import gringo
from gringo import Fun

def main(prg):
    def on_model(m):
        m.context.add_clause([(Fun("a"), False), (Fun("b"), False)])
        m.context.add_nogood([(Fun("c"), True), (Fun("d"), True)])

    prg.conf.solve.models = 0
    prg.ground([("base", [])])
    prg.solve(on_model = on_model)
    print "and again"
    prg.solve()

#end.

2 { a; b; c; d }.
