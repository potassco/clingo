#script (lua)

function tostringTable(t) 
    ret   = ""
    comma = False
    for _, elem in pairs(t) do
        if comma then ret = ret .. ", "
        else          comma = true 
        end
        ret = ret .. tostring(elem)
    end
    return ret
end

function main(prg)
    on_model = function (m)
        print("theory")
        print("  positive: " .. tostringTable(m:atoms{theory=true}))
        print("  negative: " .. tostringTable(m:atoms{theory=true, comp=true}))
    end
    prg:ground({{"base", {}}})
    prg:solve(on_model)
end

#end.
#include "csp.lp".


{a}.
b :- a.

&dom{1..10} = x.

&show {x}.
