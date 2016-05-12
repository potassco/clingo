#script (lua)

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve({{clingo.fun("a"), true}})
end

#end.

{ a }.
b :- a.
