#script (lua)

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve({{clingo.fun("a"), true}, {clingo.fun("b"), false}, {clingo.fun("d"), true}})
    prg:solve({{clingo.fun("a"), false}, {clingo.fun("c"), true}})
end

#end.

#external d.
{ a; b; c }.
