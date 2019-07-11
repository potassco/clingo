#!/usr/bin/env python3

import os
import re
import pdoc
import clingo
import importlib.machinery

def _is_public(ident_name):
    """
    Returns `True` if `ident_name` matches the export criteria for an
    identifier name.
    """
    return True

#pdoc._is_public = _is_public

clingo.ast.__spec__ = importlib.machinery.ModuleSpec("clingo.ast", None)
clingo.__pdoc__ = {}

pdoc.tpl_lookup.directories.insert(0, './templates')
ctx = pdoc.Context()

cmod = pdoc.Module(clingo, context=ctx)
amod = pdoc.Module(clingo.ast, supermodule=cmod, context=ctx)

cmod.doc["ast"] = amod
cmod.doc["__version__"] = pdoc.Variable("__version__", cmod, "__version__: str\n\nVersion of the clingo module (`'{}'`).".format(clingo.__version__))
cmod.doc["Infimum"] = pdoc.Variable("Infimum", cmod, '''Infimum: Symbol\n\nRepresents a symbol of type `clingo.SymbolType.Infimum`.''')
cmod.doc["Supremum"] = pdoc.Variable("Supremum", cmod, '''Supremum: Symbol\n\nRepresents a symbol of type `clingo.SymbolType.Supremum`.''')
pdoc.link_inheritance(ctx)

prefix = "../clingo/python-api/{}".format(".".join(clingo.__version__.split(".")[:2]))
cprefix = "../clingo/python-api/current"

os.makedirs("{}/ast".format(prefix), exist_ok=True)
os.makedirs("{}/ast".format(cprefix), exist_ok=True)

cmod_html = cmod.html(external_links=True)
amod_html = amod.html(external_links=True)

open("{}/index.html".format(prefix), "w").write(cmod_html)
open("{}/ast/index.html".format(prefix), "w").write(amod_html)

open("{}/index.html".format(cprefix), "w").write(cmod_html.replace("clingo/python-api/5.4", "clingo/python-api/current"))
open("{}/ast/index.html".format(cprefix), "w").write(amod_html.replace("clingo/python-api/5.4", "clingo/python-api/current"))
