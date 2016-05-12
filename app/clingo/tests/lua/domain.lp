p(1).
{ p(3) }. 
#external p(1..3).

q(X) :- p(X).

#script (lua)

gringo = require("gringo")

function main(prg)
    prg:ground({{"base", {}}})
    print "Solving..."
    print "Answer: 1"
    out = "universe(" .. tostring(#prg.domains) .. ")"
    for x in prg.domains:iter() do
        out = out .. " domain(" .. tostring(x.atom) .. "," .. tostring(x.is_fact) .. "," .. tostring(x.is_external) .. ")"
    end
    out = out .. " in_domain(p(2)," .. tostring(prg.domains:lookup(gringo.Fun("p", {3})) ~= nil) .. ")"
    out = out .. " in_domain(p(4)," .. tostring(prg.domains:lookup(gringo.Fun("p", {4})) ~= nil) .. ")"
    for x in prg.domains:by_signature("p", 1) do
        out = out .. " domain_of_p(" .. tostring(x.atom) .. "," .. tostring(x.is_fact) .. "," .. tostring(x.is_external) .. ")"
    end
    for i,x in ipairs(prg.domains:signatures()) do
        out = out .. " sig(" .. x[1] .. "," .. tostring(x[2]) .. ")"
    end
    print (out)
end

#end. 
