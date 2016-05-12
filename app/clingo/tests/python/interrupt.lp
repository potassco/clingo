#script (python) 

import clingo

def main(prg):
    prg.ground([("pigeon", [])])
    for i in range(0,2):
        prg.assign_external(clingo.function("p"), True)
        prg.solve()
        prg.assign_external(clingo.function("p"), False)
        f = prg.solve_async()
        f.wait(0.01)
        f.cancel()

#end.

#const n = 190.

#program pigeon.
#external p.

1 { p(X); q(X) } 1 :- X = 1..n, not p.
:- not n+1 { p(1..n); q(1..n) }, not p.
