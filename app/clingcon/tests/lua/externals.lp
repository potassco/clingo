#script (lua)

fun = clingo.fun

function main(prg)
    prg:ground({{"base", {}}})
    prg:assign_external(fun("a"), true)
    prg:solve()
    prg:assign_external(fun("a"), false)
    prg:solve()
    prg:assign_external(fun("a"), nil)
    prg:solve()
    prg:release_external(fun("a"))
    prg:solve()
end

#end.

#external a.
