#script (python)

import gringo

def main(prg):
    def on_model(m):
        print "shown"
        print "  positive:", ", ".join(map(str, m.atoms(gringo.Model.SHOWN)))
        print "  negative:", ", ".join(map(str, m.atoms(gringo.Model.SHOWN | gringo.Model.COMP)))
        print "csp"
        print "  positive:", ", ".join(map(str, m.atoms(gringo.Model.CSP)))
        print "  negative:", ", ".join(map(str, m.atoms(gringo.Model.CSP | gringo.Model.COMP)))
        print "atoms"
        print "  positive:", ", ".join(map(str, m.atoms(gringo.Model.ATOMS)))
        print "  negative:", ", ".join(map(str, m.atoms(gringo.Model.ATOMS | gringo.Model.COMP)))
        print "terms"
        print "  positive:", ", ".join(map(str, m.atoms(gringo.Model.TERMS)))
        print "  negative:", ", ".join(map(str, m.atoms(gringo.Model.TERMS | gringo.Model.COMP)))

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
