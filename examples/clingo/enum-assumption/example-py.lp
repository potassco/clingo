#script (python)

def main(prg):
    prg.configuration.solve.models = 0
    print prg.use_enumeration_assumption
    prg.ground([("base", [])])
    prg.use_enumeration_assumption = True
    print "===================== Solve ========================"
    prg.solve()
    prg.use_enumeration_assumption = False
    print "===================== Solve ========================"
    prg.solve()
    print "===================== Solve ========================"
    prg.solve()
    print prg.use_enumeration_assumption

#end.

{ a; b; c }.
