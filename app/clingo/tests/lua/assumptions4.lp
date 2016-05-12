#script (lua)

Fun = gringo.Fun

function on_model(m)
    return true
end

function on_finish(ret, canceled)
end

function main(prg)
    local assumptions = {{Fun("a"), true}}
    prg:ground({{"base", {}}})
    prg:solve(assumptions, on_model)

    local f = prg:solve_async(assumptions, on_model, on_finish)
    f:wait()

    local it = prg:solve_iter(assumptions)
    local ret = pcall(function()
        for m in it:iter() do
        end
    end)
    it:close()
end

#end.

{ a }.
b :- a.
