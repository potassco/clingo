#script(lua)
Fun = gringo.Fun
function main(prg)
    prg:ground({{"base", {}}})
    prg:assign_external(Fun("p", {3}), true)
    prg:solve()
    prg:assign_external(Fun("p", {3}), false)
    prg:solve()
    prg:ground({{"succ", {1}}, {"succ", {2}}})
    prg:solve()
    prg:ground({{"succ", {3}}})
    prg:solve()
end
#end.

