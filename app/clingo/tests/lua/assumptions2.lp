#script (lua)

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve({{clingo.fun("b"), true}})
end

#end.

{ a }.
b :- a.
