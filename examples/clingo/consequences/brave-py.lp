#script(python)

import clingo

brave = set()

def on_model(m):
    for x in m.atoms(atoms=True):
        if x.name == "holds": brave.add(x.args[0])

def is_brave(x): return x in brave

def main(prg):
    prg.ground([("base", [])])
    while prg.solve(None, on_model).satisfiable:
        prg.ground([("brave", [])])
    print "brave consequences: {0}".format(", ".join(str(x) for x in sorted(brave)))

#end.

#program brave.

:- not holds(X) : atom(X), @is_brave(X) == 0.

