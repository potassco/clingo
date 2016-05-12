#script (lua)

Fun = gringo.Fun

function main(prg)
    prg:ground({{"base", {}}})
    prg:assign_external(Fun("a"), true)
    prg:solve()
    prg:assign_external(Fun("a"), false)
    prg:solve()
    prg:assign_external(Fun("a"), nil)
    prg:solve()
    prg:release_external(Fun("a"))
    prg:solve()
end

#end.

#external a.
