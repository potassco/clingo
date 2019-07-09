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

os.makedirs("../clingo/ast", exist_ok=True)
open("../clingo/index.html", "w").write(cmod.html(external_links=True))
open("../clingo/ast/index.html", "w").write(amod.html(external_links=True))
