#script (python)

import clingo

class Values:
    def __init__(self, values):
        self.__values = values
    def get(self):
        return tuple(self.__values)

def ground(prg, solve=False):
    ctl = clingo.Control()
    ctl.add("base", [], prg)
    ctl.ground([("base", [])])
    if solve: ctl.solve()
    return ctl.is_conflicting

def main(prg):
    values = []
    values.append(str(ground("a.")))
    values.append(str(ground(":-.")))
    values.append(str(ground("2 { a; b }. :- a, b.")))
    values.append(str(ground("2 { a; b }. :- a, b.", True)))

    prg.ground([("base", [])], Values(values))
    prg.solve()

#end.

p(X) :- X = @get().
