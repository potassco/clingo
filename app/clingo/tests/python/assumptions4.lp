#script (python)

import sys
from clingo import Function

def main(prg):
    prg.ground([("base", [])])
    prg.solve_refactored(assumptions=[(Function("a"), True)])

    with prg.solve_refactored(assumptions=[(Function("a"), True)], async=True) as handle:
        handle.get()

    with prg.solve_refactored(assumptions=[(Function("a"), True)], yield_=True) as handle:
        for model in handle:
            pass

#end.

{ a }.
b :- a.
