#script(lua)

function main(prg)
    prg:ground({{"base", {}}})
    local it = prg:solve_iter()
    local ret, err = pcall(function()
        for m in it:iter() do
            print(m)
        end
        local res = it:get()
        if res == gringo.SolveResult.SAT then
            print("SAT")
        elseif res == gringo.SolveResult.UNSAT then
            print("UNSAT")
        elseif res == gringo.SolveResult.UNKNOWN then
            print("UNKNOWN")
        end
    end)
    it:close()
    if not ret then
        error(err)
    end
end

#end.
