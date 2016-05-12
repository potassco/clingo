#script (python)

def main(prg):
    prg.conf.solve.models = 0
    print prg.use_enum_assumption
    prg.ground([("base", [])])
    prg.use_enum_assumption = True
    print "===================== Solve ========================"
    prg.solve()
    prg.use_enum_assumption = False
    print "===================== Solve ========================"
    prg.solve()
    print "===================== Solve ========================"
    prg.solve()
    print prg.use_enum_assumption

#end.

{ a; b; c }.
