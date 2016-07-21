#script (python)

import clingo

def main(prg):
    def on_model(m):
        print "shown"
        print "  positive:", ", ".join(map(str, m.symbols(shown=True)))
        print "  negative:", ", ".join(map(str, m.symbols(shown=True, comp=True)))
        print "csp"
        print "  positive:", ", ".join(map(str, m.symbols(csp=True)))
        print "  negative:", ", ".join(map(str, m.symbols(csp=True, comp=True)))
        print "atoms"
        print "  positive:", ", ".join(map(str, m.symbols(atoms=True)))
        print "  negative:", ", ".join(map(str, m.symbols(atoms=True, comp=True)))
        print "terms"
        print "  positive:", ", ".join(map(str, m.symbols(terms=True)))
        print "  negative:", ", ".join(map(str, m.symbols(terms=True, comp=True)))

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
