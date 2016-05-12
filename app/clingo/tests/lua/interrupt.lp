#script (lua) 

function main(prg)
    prg:ground({{"pigeon", {}}})
    for i=1,2 do
        prg:assign_external(gringo.Fun("p"), true)
        prg:solve()
        prg:assign_external(gringo.Fun("p"), false)
        f = prg:solve_async()
        f:wait(0.01)
        f:cancel()
    end
end

#end.

#const n = 190.

#program pigeon.
#external p.

1 { p(X); q(X) } 1 :- X = 1..n, not p.
:- not n+1 { p(1..n); q(1..n) }, not p.
