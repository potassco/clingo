#script(python)
import clingo
def main(prg):
    prg.ground([("base", [])])
    prg.assign_external(clingo.Function("p", [3]), True)
    prg.solve()
    prg.assign_external(clingo.Function("p", [3]), False)
    prg.solve()
    prg.ground([("succ", [1]), ("succ", [2])])
    prg.solve()
    prg.ground([("succ", [3])])
    prg.solve()
#end.

