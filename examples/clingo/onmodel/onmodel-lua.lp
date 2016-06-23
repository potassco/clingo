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
        print("shown")
        print("  positive: " .. tostringTable(m:atoms{shown=true}))
        print("  negative: " .. tostringTable(m:atoms{shown=true, comp=true}))
        print("csp")
        print("  positive: " .. tostringTable(m:atoms{csp=true}))
        print("  negative: " .. tostringTable(m:atoms{csp=true, comp=true}))
        print("atoms")
        print("  positive: " .. tostringTable(m:atoms{atoms=true}))
        print("  negative: " .. tostringTable(m:atoms{atoms=true, comp=true}))
        print("terms")
        print("  positive: " .. tostringTable(m:atoms{terms=true}))
        print("  negative: " .. tostringTable(m:atoms{terms=true, comp=true}))
    end
    prg:ground({{"base", {}}})
    prg:solve(on_model)
end

#end.

{a}.
b :- a.
$x $= 1.
$y $= 2.
#show c : a.
#show b/0.
#show $x/0.
