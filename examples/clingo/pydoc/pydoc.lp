#!/usr/bin/env clingo-4
#script (python)
import gringo, pydoc, subprocess

pydoc.writedoc(gringo)
subprocess.call(["sed", "-i", 
    "-e", r"s/\<ffc8d8\>/88ff99/g", 
    "-e", r"s/\<ee77aa\>/22bb33/g",
    "-e", r's/<a href=".">index<\/a>.*<\/font>/<a href=".">\&laquo;Potassco<\/a><\/font>/',
    "-e", r's/<a href="__builtin__.html#object">[^<]*object<\/a>/object/g',
    "gringo.html"])

#end.
