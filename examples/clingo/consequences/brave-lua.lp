#script(lua)

Model, SolveResult = gringo.Model, gringo.SolveResult

brave = {}

function on_model(m)
    for _, x in ipairs(m:atoms(Model.ATOMS)) do
        if x:name() == "holds" then
            brave[tostring(x:args()[1])] = true
        end
    end
end

function is_brave(x)
    if brave[tostring(x)] then
        return 1
    else
        return 0
    end
end

function main(prg)
    local n = 0
    prg:ground({{"base", {}}})
    while prg:solve(nil, on_model) == SolveResult.SAT do
        if n > 10 then break end
        n = n + 1
        prg:ground({{"brave", {}}})
    end
    local ans = ""
    for x, _ in pairs(brave) do
        if ans ~= "" then
            ans = ans .. ", "
        end
        ans = ans .. tostring(x)
    end
    print(string.format("brave consequences: %s", ans))
end

#end.

#program brave.

:- not holds(X) : atom(X), @is_brave(X) == 0.

