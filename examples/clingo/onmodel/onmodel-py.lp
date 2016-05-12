#script (python)

import clingo

def main(prg):
    def on_model(m):
        print "shown"
        print "  positive:", ", ".join(map(str, m.atoms(shown=True)))
        print "  negative:", ", ".join(map(str, m.atoms(shown=True, comp=True)))
        print "csp"
        print "  positive:", ", ".join(map(str, m.atoms(csp=True)))
        print "  negative:", ", ".join(map(str, m.atoms(csp=True, comp=True)))
        print "atoms"
        print "  positive:", ", ".join(map(str, m.atoms(atoms=True)))
        print "  negative:", ", ".join(map(str, m.atoms(atoms=True, comp=True)))
        print "terms"
        print "  positive:", ", ".join(map(str, m.atoms(terms=True)))
        print "  negative:", ", ".join(map(str, m.atoms(terms=True, comp=True)))

    prg.ground([("base", [])])
    prg.solve(on_model = on_model)

#end.

{a}.
b :- a.
$x $= 1.
$y $= 2.
#show c : a.
#show b/0.
#show $x/0.
