:-.

#script (lua)

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve_async():wait()
    prg:solve_async():wait()
end

#end.
