#script (lua)

function main(prg)
    prg.conf.solve.models = 0
    print(prg.use_enum_assumption)
    prg:ground({{"base", {}}})
    prg.use_enum_assumption = true
    print("===================== Solve ========================")
    prg:solve()
    prg.use_enum_assumption = false
    print("===================== Solve ========================")
    prg:solve()
    print("===================== Solve ========================")
    prg:solve()
    print(prg.use_enum_assumption)
end

#end.

{ a; b; c }.
