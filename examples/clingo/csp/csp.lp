#theory cp {
    constant  { - : 1, unary };
    sum_term {
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left
    };
    &sum/0 : sum_term, {<=}, constant, head
}.

#script (python)

import clingo, cp_logic as cp

def print_assignment(p, m):
    a = p.get_assignment(m.thread_id)
    print ("Valid assignment for constraints found:")
    print (" ".join(["{}={}".format(n, v) for n, v in a]))

def main(prg):
    p = cp.Propagator()
    prg.register_propagator(p)
    prg.ground([("base", [])])
    prg.solve(on_model=lambda m: print_assignment(p, m))

#end.
