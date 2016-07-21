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
        print("  positive: " .. tostringTable(m:symbols{shown=true}))
        print("  negative: " .. tostringTable(m:symbols{shown=true, comp=true}))
        print("csp")
        print("  positive: " .. tostringTable(m:symbols{csp=true}))
        print("  negative: " .. tostringTable(m:symbols{csp=true, comp=true}))
        print("atoms")
        print("  positive: " .. tostringTable(m:symbols{atoms=true}))
        print("  negative: " .. tostringTable(m:symbols{atoms=true, comp=true}))
        print("terms")
        print("  positive: " .. tostringTable(m:symbols{terms=true}))
        print("  negative: " .. tostringTable(m:symbols{terms=true, comp=true}))
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
