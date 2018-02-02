#script (python)

import clingo

def main(ctl):
    with ctl.backend() as backend:
        a = backend.add_atom(clingo.Function("a", [2]))
        b = backend.add_atom(clingo.Function("b", [1]))
        backend.add_rule([a], [], True)
        backend.add_rule([b], [], True)

    ctl.ground([("base", [])])
    with ctl.backend() as backend:
        e = backend.add_atom(clingo.Function("e", [3]))
        backend.add_rule([e], [], False)
    ctl.solve()
    ctl.ground([("multi", [])])
    ctl.solve()

#end.

#program base.
#show a/1.
#show c/1.
#show d/1.
a(1).
c(X) :- a(X).
d(X) :- b(X).

#program multi.
a(X) :- e(X).
