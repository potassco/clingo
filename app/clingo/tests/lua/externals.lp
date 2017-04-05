#script (lua)

Function = clingo.Function

function main(prg)
    prg:ground({{"base", {}}})
    prg:assign_external(Function("a"), true)
    prg:solve_refactored()
    prg:assign_external(Function("a"), false)
    prg:solve_refactored()
    prg:assign_external(Function("a"), nil)
    prg:solve_refactored()
    prg:release_external(Function("a"))
    prg:solve_refactored()
end

#end.

#external a.
