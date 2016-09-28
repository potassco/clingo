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
    for line in out.split('\n'):
        if line.startswith("Step: "):
            res.extend(sorted(current))
            res.append(line)
        elif line.startswith("SAT"):
            res.extend(sorted(current))
            res.append(line)
        elif line.startswith("UNSAT"):
            res.extend(sorted(current))
            res.append(line)
        elif line.startswith("UNKNOWN"):
            res.extend(sorted(current))
            res.append(line)
        elif line.startswith("OPTIMUM FOUND"):
            res.extend(sorted(current))
            res.append(line)
        else:
            current.append(" ".join(sorted(line.split(" "))))
            pass
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
            current.append(line.strip().split(" "))
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

for root, dirs, files in os.walk(wd):
    for f in files:
        if f.endswith(".lp"):
            b = os.path.join(root, f[:-3])
            args = [clingo, "0", b + ".lp", "-Wnone"]
            if os.path.exists(b + ".cmd"):
                for x in open(b + ".cmd"):
                    args.extend(x.strip().split())
            out, err = sp.Popen(args, stdout=sp.PIPE, universal_newlines=True).communicate()
            norm = normalize(out)
            sol  = reorder(open(b + ".sol").read())
            if norm != sol:
                print "*" * (len(b) + 3)
                print b + ".lp"
                print "*" * (len(b) + 3)
                d = dl.Differ()
                for line in list(d.compare(norm.splitlines(1), sol.splitlines(1))):
                    if not line.startswith(" "):
                        sys.stdout.write(line)
                sys.stdout.write("\n")
                sys.stdout.flush()

