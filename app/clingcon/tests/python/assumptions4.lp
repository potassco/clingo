#script (python)

import sys
from clingo import function

def main(prg):
    prg.ground([("base", [])])
    prg.solve(assumptions=[(function("a"), True)])

    f = prg.solve_async(assumptions=[(function("a"), True)])
    f.wait()

    with prg.solve_iter(assumptions=[(function("a"), True)]) as it:
        for model in it: pass

#end.

{ a }.
b :- a.
