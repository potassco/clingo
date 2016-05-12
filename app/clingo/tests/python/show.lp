#script (python)

import gringo
from gringo import Fun, Model
import sys

models = []

def getModels():
    return models

def normalize(terms):
    major = sys.version_info[0]
    if major >= 3:
        import functools
        return sorted(terms, key=functools.cmp_to_key(gringo.cmp))
    else:
        return sorted(terms, gringo.cmp)

def split(model):
    return sorted(str(model).split(" "))

def fun(name, model):
    return Fun(name, model) if len(model) > 0 else Fun(name)

def on_model(model):
    models.append(fun("model", split(model)))
    models.append(fun("shown", normalize(model.atoms(Model.SHOWN))))
    models.append(fun("atoms", normalize(model.atoms(Model.ATOMS))))
    models.append(fun("terms", normalize(model.atoms(Model.TERMS))))
    models.append(fun("csp",   normalize(model.atoms(Model.CSP))))

def main(prg):
    prg.ground([("base", [])])
    prg.solve(None, on_model)
    prg.ground([("result", [])])
    prg.solve()

#end.

$1 $= 11.
{ b }.
c.
$x $= 42.
$y(1) $= 13.
$y(2) $= 23.
$z $= 7.
#show $1.
#show c/0.
#show a : b.
#show $x/0.
#show $y(1) : b.
#show $y(2) : not b.
#show $y(3) : b.

#program result.
res(X) :- X = @getModels(). 
#show res/1.

