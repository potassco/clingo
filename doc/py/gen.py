import os
import pdoc
import clingo
import clingo.ast
import re
ctx = pdoc.Context()

cmod = pdoc.Module(clingo, context=ctx)
amod = pdoc.Module(clingo.ast, supermodule=cmod, context=ctx)

cmod.doc["ast"] = amod
pdoc.link_inheritance(ctx)

def replace(s):
    s = s.replace('href="clingo.html', 'href="clingo/')
    s = s.replace('href="../clingo.html', 'href="../')
    s = s.replace('href="clingo/ast.html', 'href="ast/')
    s = re.sub(r"['\"]https://cdnjs\.cloudflare\.com/.*/([^/'\"]+\.(css|js))['\"]", r"'\2/\1'", s)
    return s

os.makedirs("clingo/ast", exist_ok=True)
open("clingo/index.html", "w").write(replace(cmod.html(external_links=True)))
open("clingo/ast/index.html", "w").write(replace(amod.html(external_links=True)))
