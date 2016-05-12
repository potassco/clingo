#script (python) 

import gringo

def get(val, default):
    return val if val != None else default

def main(prg):
    imin   = get(prg.get_const("imin"), 1)
    imax   = prg.get_const("imax")
    istop  = get(prg.get_const("istop"), "SAT")

    step, ret = 0, gringo.SolveResult.UNKNOWN
    while ((imax is None or step < imax) and
           (step == 0   or step < imin or (
              (istop == "SAT"     and ret != gringo.SolveResult.SAT) or
              (istop == "UNSAT"   and ret != gringo.SolveResult.UNSAT) or 
              (istop == "UNKNOWN" and ret != gringo.SolveResult.UNKNOWN)))):
        parts = []
        parts.append(("check", [step]))
        if step > 0:
            prg.release_external(gringo.Fun("query", [step-1]))
            parts.append(("step", [step]))
            prg.cleanup_domains()
        else:
            parts.append(("base", []))
        prg.ground(parts)
        prg.assign_external(gringo.Fun("query", [step]), True)
        ret, step = prg.solve(), step+1
#end.

#program check(t).
#external query(t).
