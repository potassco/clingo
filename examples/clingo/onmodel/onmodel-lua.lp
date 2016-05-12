#script (lua)

Model = gringo.Model

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
        print("  positive: " .. tostringTable(m:atoms(Model.SHOWN)))
        print("  negative: " .. tostringTable(m:atoms(Model.SHOWN + Model.COMP)))
        print("csp")
        print("  positive: " .. tostringTable(m:atoms(Model.CSP)))
        print("  negative: " .. tostringTable(m:atoms(Model.CSP + Model.COMP)))
        print("atoms")
        print("  positive: " .. tostringTable(m:atoms(Model.ATOMS)))
        print("  negative: " .. tostringTable(m:atoms(Model.ATOMS + Model.COMP)))
        print("terms")
        print("  positive: " .. tostringTable(m:atoms(Model.TERMS)))
        print("  negative: " .. tostringTable(m:atoms(Model.TERMS + Model.COMP)))
    end
    prg:ground({{"base", {}}})
    prg:solve(nil, on_model)
end

#end.

{a}.
b :- a.
$x $= 1.
$y $= 2.
#show c : a.
#show b/0.
#show $x/0.
