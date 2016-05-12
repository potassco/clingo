#!/usr/bin/env clingo-4-banane
#script (python)
import gringo, pydoc, subprocess

pydoc.writedoc(gringo)
subprocess.call(["sed", "-i", "-e", r"s/\<ffc8d8\>/88ff99/g", "-e", r"s/\<ee77aa\>/22bb33/g", "gringo.html"])

#end.
