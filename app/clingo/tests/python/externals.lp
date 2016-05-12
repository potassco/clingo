#script (python)

import sys
from clingo import function

def main(prg):
    prg.ground([("base", [])])
    prg.assign_external(function("a"), True)
    prg.solve()
    prg.assign_external(function("a"), False)
    prg.solve()
    prg.assign_external(function("a"), None)
    prg.solve()
    prg.release_external(function("a"))
    prg.solve()

#end.

#external a.
