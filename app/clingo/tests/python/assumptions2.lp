#script (python)

import sys
from gringo import Fun

def main(prg):
    prg.ground([("base", [])])
    prg.solve(assumptions=[(Fun("b"), True)])

#end.

{ a }.
b :- a.
