#script (lua)

Fun = gringo.Fun

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve({{Fun("b"), true}})
end

#end.

{ a }.
b :- a.
