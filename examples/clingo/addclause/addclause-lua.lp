#script (lua)

function main(prg)
    on_model = function (m)
        m.context:add_clause({{clingo.fun("a"), false}, {clingo.fun("b"), false}})
        m.context:add_nogood({{clingo.fun("c"), true}, {clingo.fun("d"), true}})
    end

    prg.conf.solve.models = 0
    prg:ground({{"base", {}}})
    prg:solve(nil, on_model)
    print "and again"
    prg:solve()
end

#end.

2 { a; b; c; d }.
