#script (lua)

models = {}

function getModels()
    return models
end

function normalize(terms)
    table.sort(terms)
    return terms
end

function split(model)
    terms = {}
    for i in string.gmatch(tostring(model), "%S+") do terms[#terms+1] = i end
    table.sort(terms)
    return terms
end

function on_model(model)
    models[#models+1] = clingo.Function("model", split(model))
    models[#models+1] = clingo.Function("shown", normalize(model:symbols{shown=true}))
    models[#models+1] = clingo.Function("atoms", normalize(model:symbols{atoms=true}))
    models[#models+1] = clingo.Function("terms", normalize(model:symbols{terms=true}))
    models[#models+1] = clingo.Function("csp",   normalize(model:symbols{csp=true}))
end

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve(on_model)
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

