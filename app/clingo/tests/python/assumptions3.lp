#script (python)

import sys
from clingo import function

def main(prg):
    prg.ground([("base", [])])
    prg.solve(assumptions=[(function("a"), True), (function("b"), False), (function("d"), True)])
    prg.solve(assumptions=[(function("a"), False), (function("c"), True)])

#end.  

#external d.
{ a; b; c }.
