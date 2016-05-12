#script (python)

import sys
from gringo import Fun

def main(prg):
    prg.ground([("base", [])])
    prg.solve(assumptions=[(Fun("a"), True)])

    f = prg.solve_async(assumptions=[(Fun("a"), True)])
    f.wait()

    with prg.solve_iter(assumptions=[(Fun("a"), True)]) as it:
        for model in it: pass

#end.

{ a }.
b :- a.
