#program step(k).

#external vol(k).
p(k).
:- vol(k), not 5 { p(K) : K <= k }.

#script (python)

from gringo import *

def main(prg):
    step = 0
    while True:
        if step > 0: prg.release_external(Fun("vol", [step-1]))
        prg.ground([("step", [step])])
        prg.assign_external(Fun("vol", [step]), True)
        ret = prg.solve()
        if ret == SolveResult.SAT:
            break
        step += 1

#end.
