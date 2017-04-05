#script (python)

import sys
from clingo import Function

def main(prg):
    prg.ground([("base", [])])
    prg.assign_external(Function("a"), True)
    prg.solve_refactored()
    prg.assign_external(Function("a"), False)
    prg.solve_refactored()
    prg.assign_external(Function("a"), None)
    prg.solve_refactored()
    prg.release_external(Function("a"))
    prg.solve_refactored()

#end.

#external a.
