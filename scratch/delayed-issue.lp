{ p(0) }.
a :- a, p(_).
{ c } :- a.

#script (python)

import clingo

def main(ctl):
    print "base"
    ctl.ground([("base", [])])
    print "nothing"
    ctl.ground([("nothing", [])])

#end.
