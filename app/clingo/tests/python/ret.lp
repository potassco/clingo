#script (python)

import clingo

ctl = clingo.Control()
def a():
    return (1,2,3)

def b():
    return [1,2,3]

def c():
    return iter([1,2,3])

ctl.ground([("base", [])])
ctl.solve()

#end.

a(@a()).
b(@b()).
c(@c()).
