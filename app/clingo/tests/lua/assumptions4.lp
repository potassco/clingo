#script (lua)

function on_model(m)
    return true
end

function on_finish(ret)
end

function main(prg)
    local assumptions = {{clingo.Function("a"), true}}
    prg:ground({{"base", {}}})
    prg:solve_refactored{on_model=on_model, assumptions=assumptions}
    prg:solve_refactored{on_model=on_model, assumptions=assumptions}
    local it = prg:solve_refactored{assumptions=assumptions, yield=true}
    local ret = pcall(function()
        for m in it:iter() do
        end
    end)
    it:close()
end

#end.

{ a }.
b :- a.
