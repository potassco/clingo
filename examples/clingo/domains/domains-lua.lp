#script (lua)

gringo = require("gringo")

function main(prg)
    prg:ground({{"base", {}}})
    print ("universe:" .. tostring(#prg.domains))
    for x in prg.domains:iter() do
        print (x.atom, x.is_fact, x.is_external)
    end
    print ("p(2) is in domain: " .. tostring(prg.domains:lookup(gringo.Fun("p", {3})) ~= nil))
    print ("p(4) is in domain: " .. tostring(prg.domains:lookup(gringo.Fun("p", {4})) ~= nil))
    print ("domain of p/1:")
    for x in prg.domains:by_signature("p", 1) do
        print (x.atom, x.is_fact, x.is_external)
    end
    print ("signatures:")
    for i,x in ipairs(prg.domains:signatures()) do
        print (x[1] .. "/" .. tostring(x[2]))
    end
end

#end. 
