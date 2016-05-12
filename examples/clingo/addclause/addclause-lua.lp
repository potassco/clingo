#script (lua)

Fun = gringo.Fun

function main(prg)
    on_model = function (m)
        m.context:add_clause({{Fun("a"), false}, {Fun("b"), false}})
        m.context:add_nogood({{Fun("c"), true}, {Fun("d"), true}})
    end

    prg.conf.solve.models = 0
    prg:ground({{"base", {}}})
    prg:solve(nil, on_model)
    print "and again"
    prg:solve()
end

#end.

2 { a; b; c; d }.
