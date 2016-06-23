#script (lua)

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve(nil, {{clingo.Function("a"), true}, {clingo.Function("b"), false}, {clingo.Function("d"), true}})
    prg:solve(nil, {{clingo.Function("a"), false}, {clingo.Function("c"), true}})
end

#end.

#external d.
{ a; b; c }.
