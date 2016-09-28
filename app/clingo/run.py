#!/usr/bin/python

import os
import os.path
import sys
import subprocess as sp
import difflib as dl

wd=os.path.normpath(os.path.dirname(__file__))
clingo=os.path.normpath("{}/../../x64/ReleaseScript/clingo.exe".format(wd))

def reorder(out):
    res = []
    current = []
    for line in out.splitlines():
        if line.startswith("Step: ") or line.startswith("SAT") or line.startswith("UNSAT") or line.startswith("UNKNOWN") or line.startswith("OPTIMUM FOUND"):
            res.extend(sorted(current))
            res.append(line)
            current = []
        else:
            current.append(" ".join(sorted(line.split(" "))))
    return "\n".join(res)

def normalize(out):
    state=0
    current=[]
    step=0
    result="ERROR"
    norm=[]
    for line in out.split('\n'):
        if state == 1:
            if step > 0:
                norm.append("Step: {}".format(step))
                models = []
                for model in current:
                    models.append(" ".join(sorted(model)))
                for model in sorted(models):
                    norm.append(model)
            step += 1
            state = 0
            current = []
        if state == 2:
            # TODO: the current script ignores empty models -> bad
            model = line.strip()
            if model != "":
                current.append(model.split(" "))
            state=0
        elif line.startswith("Solving..."):
            state = 1
        elif line.startswith("Answer: "):
            state = 2
        elif line.startswith("SATISFIABLE"):
            result="SAT"
            state=1
        elif line.startswith("UNSATISFIABLE"):
            result="UNSAT"
            state=1
        elif line.startswith("UNKNOWN"):
            result="UNKNOWN"
            state=1
        elif line.startswith("OPTIMUM FOUND"):
            result="OPTIMUM FOUND"
            state=1
    norm.append(result)
    return "\n".join(norm)

success = True

for root, dirs, files in os.walk(wd):
    for f in files:
        if f.endswith(".lp"):
            b = os.path.join(root, f[:-3])
            print "-" * 79
            print b + ".lp"
            print "." * 79
            sys.stdout.flush()
            args = [clingo, "0", b + ".lp", "-Wnone"]
            if os.path.exists(b + ".cmd"):
                for x in open(b + ".cmd"):
                    args.extend(x.strip().split())
            args.extend(sys.argv[1:])
            out, err = sp.Popen(args, stdout=sp.PIPE, universal_newlines=True).communicate()
            norm = normalize(out)
            sol  = reorder(open(b + ".sol").read())
            if norm != sol:
                print "failed"
                print "!" * 79
                print " ".join(args)
                print "!" * 79
                d = dl.Differ()
                for line in list(d.compare(sol.splitlines(), norm.splitlines())):
                    if not line.startswith(" "):
                        print line
                success = False
            else:
                print "ok"
            sys.stdout.flush()

print "-" * 79
print "Result"
print "." * 79
if not success:
    print "FAILED"
    exit(1)
else:
    print "OK"

