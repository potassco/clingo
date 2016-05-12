#script(python)

from gringo import Model, SolveResult

brave = set()

def on_model(m):
    for x in m.atoms(Model.ATOMS):
        if x.name() == "holds": brave.add(x.args()[0])

def is_brave(x): return x in brave

def main(prg):
    prg.ground([("base", [])])
    while prg.solve(None, on_model) == SolveResult.SAT:
        prg.ground([("brave", [])])
    print "brave consequences: {0}".format(", ".join(str(x) for x in sorted(brave)))

#end.

#program brave.

:- not holds(X) : atom(X), @is_brave(X) == 0.

