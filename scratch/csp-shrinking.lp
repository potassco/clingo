%!clingo scratch/csp-shrinking.lp 0 -q -V0

#script (lua)

json = (function () 
    local ret, json = pcall(function () return require("cjson") end)
    if ret then return json else return require("json") end
end)()

function main(prg)
    sep = false
    for i=10,-4,-1 do
        if sep then print("======================")
        else        sep = true end 
        print("k", ":", i)
        prg:ground("p", {i})
        prg:solve()
        stats = prg:getStats()
        print("M", ":", stats.numEnum)
    end
end

#end.

#program p(k).

1 $<= $x $<= k.
