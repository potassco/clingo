#!/usr/bin/env python

import gringo

print "to begin with"
a = gringo.Control(["0"])
a.add("base", [],
"""
#script(python)
def test2():
    return 42
#end.
1{a;b;p(@test2())}.
""")
a.ground("base", [])
a.solve()
with a.iterSolve() as it:
    for m in it: print m

def test3():
    return 23

def test():
    print("solve something")
    c = gringo.Control(["0"])
    c.add("base", [],"1{a;b;p(@test2())}.")
    c.ground("base", [])
    with c.iterSolve() as it:
        for m in it: print m

    print("solve something different")
    d = gringo.Control(["0"])
    d.add("base", [], "1{b;c;p(@test3())}.")
    d.ground("base", [])
    with d.iterSolve() as it:
        for m in it: print m

print "do it once"
test()
print "do it twice"
test()
print "do it thrice"
test()
print "time to stop this"

a = gringo.Inf()
b = gringo.Sup()
c = gringo.Fun("a", [a, b])
d = gringo.Inf()
e = gringo.Inf()
