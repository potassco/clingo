#script (python)

import signal

class Status:
    def __init__(self, f):
        self.solving = True
        self.f       = f


def interrupt(s, a, b):
    s.f.interrupt()
    s.solving = False

def main(prg):
    prg.ground("pigeon", [])
    f = prg.asolve()
    s = Status(f)
    signal.signal(signal.SIGINT, lambda a, b: interrupt(s, a, b))
    while s.solving:
        pass
    f.wait()
#end.

#const n = 190.

#program pigeon.

1 { p(X); q(X) } 1 :- X = 1..n.
:- not n+1 { p(1..n); q(1..n) }.
