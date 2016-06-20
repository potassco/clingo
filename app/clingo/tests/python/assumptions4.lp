#script (python)

import sys
from clingo import Function

def main(prg):
    prg.ground([("base", [])])
    prg.solve(assumptions=[(Function("a"), True)])

    f = prg.solve_async(assumptions=[(Function("a"), True)])
    f.wait()

    with prg.solve_iter(assumptions=[(Function("a"), True)]) as it:
        for model in it: pass

#end.

{ a }.
b :- a.
