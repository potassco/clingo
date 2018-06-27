#script (lua)

function main(prg)
    prg.configuration.solve.models = 0
    prg:ground({{"base", {}}})
    prg.use_enumeration_assumption = true
    print("===================== Solve ========================")
    prg:solve()
    prg.use_enumeration_assumption = false
    print("===================== Solve ========================")
    prg:solve()
    print("===================== Solve ========================")
    prg:solve()
end

#end.

{ a; b; c }.
