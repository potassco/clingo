#script(lua)
function main(prg)
    n = 0
    parts = {}
    for _, rng in pairs(prg:get_const("calls"):args()) do
        lower = rng:args()[1]
        upper = rng:args()[2]
        while n < upper do
            n = n + 1
            parts[#parts+1] = {"board", {n}}
            if n >= lower then
                prg:ground(parts)
                parts = {}
                io.write(string.format('SIZE %d\n', n))
                prg:solve()
            end
        end
    end
end
#end.
