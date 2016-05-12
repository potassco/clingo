#script (lua)

Fun = gringo.Fun

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve({{Fun("a"), true}, {Fun("b"), false}, {Fun("d"), true}})
    prg:solve({{Fun("a"), false}, {Fun("c"), true}})
end

#end.

#external d.
{ a; b; c }.
