#theory dl {
    constant {- : 1, unary};
    diff_term {- : 1, binary, left};
    &diff/0 : diff_term, {<=}, constant, any
}.

#program bound(b).

&diff { bound-0 } <= b.

#script (python)

import clingo, difference_logic as dl

def main(prg):
    p = dl.Propagator()
    prg.register_propagator(p)
    prg.ground([("base", [])])
    while True:
        bound = 0
        with prg.solve(yield_=True) as h:
            for m in h:
                a = p.get_assignment(m.thread_id)
                for n, v in a:
                    if n == "bound":
                        bound = v
                        break
                print "Valid assignment for constraints found:"
                print " ".join(["{}={}".format(n, v) for n, v in a])
                break
            else:
                print "Optimum found"
                break
        print "Found new bound: {}".format(bound)
        prg.ground([("bound", [bound-1])])
#end.
