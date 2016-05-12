#script (python)

import clingo

def main(prg):
    prg.ground([("step1", [])])
    prg.assign_external(clingo.function("f"), True)
    prg.conf.solve.models = 1
    prg.solve()
    prg.ground([("step2", [])])
    prg.assign_external(clingo.function("f"), False)
    prg.conf.solve.models = 0
    prg.solve()

#end.

#program step1.

#external f.
{ a; b; c }.

#show f/0.

#program step2.

#show a : a.
#show b : b.
#show c : c.

