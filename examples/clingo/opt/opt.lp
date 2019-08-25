#program bound(b).

:- #sum { V,I: _minimize(V,I) } >= b.

#script (python)

import clingo

def main(prg):
    prg.ground([("base", [])])
    while True:
        bound = 0
        with prg.solve(yield_=True) as h:
            for m in h:
                for atom in m.symbols(atoms=True):
                    if (atom.name == "_minimize"
                    and len(atom.arguments) > 0
                    and atom.arguments[0].type
                    is  clingo.SymbolType.Number):
                        bound += atom.arguments[0].number
                break
            else:
                print ("Optimum found")
                break
        print ("Found new bound: {}".format(bound))
        prg.ground([("bound", [bound])])

#end.
