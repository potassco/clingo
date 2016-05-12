#script (python)

import sys
from gringo import Fun

def main(prg):
    prg.ground([("base", [])])
    prg.assign_external(Fun("a"), True)
    prg.solve()
    prg.assign_external(Fun("a"), False)
    prg.solve()
    prg.assign_external(Fun("a"), None)
    prg.solve()
    prg.release_external(Fun("a"))
    prg.solve()

#end.

#external a.
