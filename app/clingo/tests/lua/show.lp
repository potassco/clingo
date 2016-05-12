#script (lua)

Fun   = gringo.Fun
Model = gringo.Model

models = {}

function getModels()
    return models
end

function normalize(terms)
    table.sort(terms, function (a, b) return gringo.cmp(a, b) < 0 end)
    return terms
end

function split(model)
    terms = {}
    for i in string.gmatch(tostring(model), "%S+") do terms[#terms+1] = i end
    table.sort(terms)
    return terms
end

function fun(name, model)
    return Fun(name, model)
end

function on_model(model)
    models[#models+1] = fun("model", split(model))
    models[#models+1] = fun("shown", normalize(model:atoms(Model.SHOWN)))
    models[#models+1] = fun("atoms", normalize(model:atoms(Model.ATOMS)))
    models[#models+1] = fun("terms", normalize(model:atoms(Model.TERMS)))
    models[#models+1] = fun("csp",   normalize(model:atoms(Model.CSP)))
end

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve(nil, on_model)
    prg:ground({{"result", {}}})
    prg:solve()
end

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

