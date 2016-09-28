#!/usr/bin/python

import os
import os.path
import sys
import subprocess as sp
import difflib as dl
import argparse

parser = argparse.ArgumentParser(description="""
Can be used to run and normalize tests.
Additional options can be passed to clingo by adding them at the end of the command line preceded by '--'.
""")
parser.add_argument('-c', '--clingo', default=None, help="path to clingo executable")

subparsers = parser.add_subparsers(dest="action", help="sub-command --help")
subparsers.required = True
parser_run = subparsers.add_parser('run', help='run all tests')
parser_normalize = subparsers.add_parser('normalize', help='normalize the output of clingo')
parser_normalize.add_argument("file")

argv = sys.argv[1:]
extra_argv = []
if "--" in argv:
    extra_argv = argv[argv.index("--")+1:]
    argv       = argv[:argv.index("--")]

parse_ret = parser.parse_args(argv)

if parse_ret.action is None:
    print (parser.usage)
    exit(0)

clingo = parse_ret.clingo

def find_clingo():
    clingos = [
        "build/debug/clingo",
        "build/release/clingo",
        "x64/ReleaseScript/clingo.exe",
        "ReleaseScript/clingo.exe",
        "x64/Release/clingo.exe",
        "Release/clingo.exe",
        "x64/DebugScript/clingo.exe",
        "DebugScript/clingo.exe",
        "x64/Debug/clingo.exe",
        "Debug/clingo.exe",
        ]
    for x in clingos:
        x = os.path.normpath("{}/../../../{}".format(wd, x))
        if os.path.exists(x):
            return x
    return None

wd = os.path.normpath(os.path.dirname(__file__))
if clingo is None:
    clingo = find_clingo()

if clingo is None:
    print ("no usable clingo version found")
    exit(1)

def reorder(out):
    return out
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
    norm.append("")
    return "\n".join(norm)

if parse_ret.action == "normalize":
    args = [clingo, "0", parse_ret.file, "-Wnone"]
    b = os.path.splitext(parse_ret.file)[0]
    if os.path.exists(b + ".cmd"):
        for x in open(b + ".cmd"):
            args.extend(x.strip().split())
    args.extend(extra_argv)
    out, err = sp.Popen(args, stderr=sp.PIPE, stdout=sp.PIPE, universal_newlines=True).communicate()
    sys.stdout.write(normalize(out))
    exit(0)
if parse_ret.action == "run":
    total  = 0
    failed = 0

    for root, dirs, files in os.walk(wd):
        for f in sorted(files):
            if f.endswith(".lp"):
                total+= 1
                b = os.path.join(root, f[:-3])
                print ("-" * 79)
                print (b + ".lp")
                print ("." * 79)
                sys.stdout.flush()
                args = [clingo, "0", b + ".lp", "-Wnone"]
                if os.path.exists(b + ".cmd"):
                    for x in open(b + ".cmd"):
                        args.extend(x.strip().split())
                args.extend(extra_argv)
                out, err = sp.Popen(args, stdout=sp.PIPE, universal_newlines=True).communicate()
                norm = normalize(out)
                sol  = reorder(open(b + ".sol").read())
                if norm != sol:
                    failed+= 1
                    print ("failed")
                    print ("!" * 79)
                    print (" ".join(args))
                    print ("!" * 79)
                    d = dl.Differ()
                    for line in list(d.compare(sol.splitlines(), norm.splitlines())):
                        if not line.startswith(" "):
                            print (line)
                else:
                    print ("ok")
                sys.stdout.flush()

    print ("-" * 79)
    print ("Result")
    print ("." * 79)
    if failed > 0:
        print ("FAILED ({} of {})".format(failed, total))
        exit(1)
    else:
        print ("OK ({})".format(total))
        exit(0)

