#script(python)
import clingo
from clingo import *

def on_model(m):
        print "shown"
        print "  positive:", ", ".join(map(str, m.atoms(shown=True)))
        print "  negative:", ", ".join(map(str, m.atoms(shown=True, comp=True)))
        print "csp"
        print "  positive:", ", ".join(map(str, m.atoms(csp=True)))
        print "  negative:", ", ".join(map(str, m.atoms(csp=True, comp=True)))
        print "atoms"
        print "  positive:", ", ".join(map(str, m.atoms(atoms=True)))
        print "  negative:", ", ".join(map(str, m.atoms(atoms=True, comp=True)))
        print "terms"
        print "  positive:", ", ".join(map(str, m.atoms(terms=True)))
        print "  negative:", ", ".join(map(str, m.atoms(terms=True, comp=True)))


def main(prg):
#  help(clingo)
  prg.ground([("base",[])])
  prg.solve(on_model=on_model)
  prg.ground([("test",[10])])
  prg.assign_external(Function("active",[10]),True)
  prg.solve(on_model=on_model)
  prg.release_external(Function("active",[10]))

  prg.ground([("test",[20])])
  prg.assign_external(Function("active",[20]),True)
  prg.solve(on_model=on_model)
#  prg.release_external(Function("active",[20]))

#end.





#program base.
#include "csp.lp".
&sum{x}>=0.
&show {x}.

&sum{y}>=0.
&sum{y}<=10.

#program test(n).
#external active(n).
&show{y}.
:- active(n), not &sum{x} <= n.


