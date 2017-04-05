:-.

#script (lua)

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve_refactored()
    prg:solve_refactored()
end

#end.
