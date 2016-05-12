#!/usr/bin/env python

import gringo
import json
import sys

control = gringo.Control()
scratch = False

outputs = True
statist = False
verbose = False

sources = []
options = []

objects = 0
horizon = 0

end = 0

def info():
    print "Usage:   main.py [--scratch] [--quiet] [--stats] [--verbose] [--maxobj=n] [--option=\"<option key> <option value>\"]* [logic program files]+"
    print "Example: main.py --scratch --quiet --stats --verbose --maxobj=42 --option=\"solve.models 0\" encoding.lp instance.lp"

def show(model):
    global outputs

    if outputs: print "Model:  ", model.atoms()

def ground(kind):
    global control, scratch, verbose, sources, options, objects, horizon

    count = objects+horizon+1
    parts = [("expand", [count])]

    if scratch and count > 1:
        control = gringo.Control()
        for source in sources: control.load(source)
        for i in range(0,objects): parts.append(("object", [i+1, count]))
        for i in range(0,horizon): parts.append(("horizon", [i+1, count]))

    if scratch or count == 1:
        for option in options: setattr(control.conf, option[0], option[1])
        parts.append(("base", []))

    if kind:
        objects += 1
        parts.append(("object", [objects, count]))
    else:
        horizon += 1
        parts.append(("horizon", [horizon, count]))

    if verbose:
         print
         print "Objects:", objects
         print "Horizon:", horizon

    control.ground(parts)

    if verbose:
         print "Solving:", count

def main(argv):
    global control, scratch, outputs, statist, verbose, sources, options, objects, horizon, end

    args = len(argv)
    for i in range(0, args):
        arg = argv[args-(i+1)]
        if arg == "--scratch":
            scratch = True
            argv.remove(arg)
        else:
            if arg == "--quiet":
                outputs = False
                argv.remove(arg)
            else:
                if arg == "--stats":
                    statist = True
                    argv.remove(arg)
                else:
                    if arg == "--verbose":
                        verbose = True
                        argv.remove(arg)
                    else:
                        if arg.startswith("--maxobj="):
                            end = int(arg[9:])
                            argv.remove(arg)
                        else:
                            if arg.startswith("--option="):
                                options.append(arg[9:].split())
                                argv.remove(arg)
    sources = argv

    if "-h" in sources or "--help" in sources or sources == []: info()
    else:
        for source in sources: control.load(source)
        if end == 0: end = control.get_const("n")
        while objects < end:
            ground(True)
            while True:
                ret = control.solve(on_model=show)
                if statist:
                    print "\"step\":", json.dumps(control.stats["step"], sort_keys=True, indent=0, separators=(',', ': '))
                    print "\"enumerated\":", json.dumps(control.stats["enumerated"], sort_keys=True, indent=0, separators=(',', ': '))
                    print "\"time_cpu\":", json.dumps(control.stats["time_cpu"], sort_keys=True, indent=0, separators=(',', ': '))
                    print "\"time_solve\":", json.dumps(control.stats["time_solve"], sort_keys=True, indent=0, separators=(',', ': '))
                    print "\"time_sat\":", json.dumps(control.stats["time_sat"], sort_keys=True, indent=0, separators=(',', ': '))
                    print "\"time_unsat\":", json.dumps(control.stats["time_unsat"], sort_keys=True, indent=0, separators=(',', ': '))
                    print "\"time_total\":", json.dumps(control.stats["time_total"], sort_keys=True, indent=0, separators=(',', ': '))
                    print json.dumps(control.stats["lp"], sort_keys=True, indent=0, separators=(',', ': '))
                    print json.dumps(control.stats["ctx"], sort_keys=True, indent=0, separators=(',', ': '))
                    print json.dumps(control.stats["solvers"], sort_keys=True, indent=0, separators=(',', ': '))
                if ret == gringo.SolveResult.SAT: break
                ground(False)

main(sys.argv[1:])
