#script (python)

import sys
from clingo import Function

def main(prg):
    prg.ground([("base", [])])
    prg.solve_refactored(assumptions=[(Function("a"), True), (Function("b"), False), (Function("d"), True)])
    prg.solve_refactored(assumptions=[(Function("a"), False), (Function("c"), True)])

#end.  

#external d.
{ a; b; c }.
