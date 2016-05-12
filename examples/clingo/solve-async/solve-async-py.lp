#script(python)

from random import random

def make_on_finish(stop):
    def on_finish(ret, interrupted):
        stop[0] = True
    return on_finish

def main(prg):
    stop = [False]
    prg.ground([("base", [])])
    future, n, m = prg.solve_async(None, None, make_on_finish(stop)), 0, 0
    while not stop[0]:
        x, y, m = random(), random(), m+1
        if x * x + y * y < 1: n+= 1
    future.wait()
    print "approximation of pi: {0}".format(4.*n/m)

#end.
