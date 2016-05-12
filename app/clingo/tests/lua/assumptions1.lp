#script (lua)

Fun = gringo.Fun

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve({{Fun("a"), true}})
end

#end.

{ a }.
b :- a.
