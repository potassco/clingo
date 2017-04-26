#theory dl {
    constant  { - : 1, unary };
    diff_term { - : 1, binary, left };
    &diff/0 : diff_term, {<=}, constant, head
}.

#script (python)

import clingo, difference_logic as dl

def print_assignment(p, m):
    a = p.get_assignment(m.thread_id)
    print "Valid assignment for constraints found:"
    print " ".join(["{}={}".format(n, v) for n, v in a])

def main(prg):
    p = dl.Propagator()
    prg.register_propagator(p)
    prg.ground([("base", [])])
    prg.solve(on_model=lambda m: print_assignment(p, m))

#end.
