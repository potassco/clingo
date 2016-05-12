#script (lua)

function on_model(m)
    return true
end

function on_finish(ret)
end

function main(prg)
    local assumptions = {{clingo.fun("a"), true}}
    prg:ground({{"base", {}}})
    prg:solve(assumptions, on_model)
    prg:solve(assumptions, on_model)
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
