#script (python)

import sys
from clingo import function

def main(prg):
    prg.ground([("base", [])])
    prg.solve(assumptions=[(function("b"), True)])

#end.

{ a }.
b :- a.
