#script (python)

import gringo

def print_conf(conf, ident):
    for x in conf.keys():
        key = x.split(".")
        if len(key) > 1:
            subconf = getattr(conf, key[0])
            label   = key[0]
            if (len(subconf) >= 0):
                label += "[0.." + str(len(subconf)) + "]"
            print ("{0}{1} - {2}".format(ident, label, getattr(conf, "__desc_" + key[0])))
            print_conf(subconf, "  " + ident + label + ".")
        else:
            print ("{0}{1}[={2}] - {3}".format(ident, key[0], getattr(conf, key[0]), getattr(conf, "__desc_" + key[0])))

def main(prg):
    prg.ground([("base", [])])
    blub = prg.conf.solver
 
    print_conf(prg.conf, "")

    print ("The heuristics of the solvers in the 'many' portfolio:")
    prg.conf.configuration = "many"
    prg.conf.solve.parallel_mode = 3 # just use the first 3 solvers
    for x in prg.conf.solver:
        print ("  " + x.heuristic)

    prg.conf.solve.models = 0
    print "==================== All Models ==================="
    prg.solve()

    prg.conf.solve.models = 1
    print "===================== One Model ==================="
    prg.solve()

    prg.conf.solve.models = 0
    prg.conf.solve.enum_mode = "cautious"
    print "=============== Cautious Consequences ============="
    prg.solve()

    prg.conf.solve.models = 0
    prg.conf.solve.enum_mode = "brave"
    print "================ Brave Consequences ==============="
    prg.solve()

#end.

{ a; b; c }.
