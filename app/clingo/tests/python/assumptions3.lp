#script (python)

import sys
from gringo import Fun

def main(prg):
    prg.ground([("base", [])])
    prg.solve(assumptions=[(Fun("a"), True), (Fun("b"), False), (Fun("d"), True)])
    prg.solve(assumptions=[(Fun("a"), False), (Fun("c"), True)])

#end.  

#external d.
{ a; b; c }.
