%!clingo scratch/csp-shrinking.lp 0 -q -V0

#script (lua)

function main(prg)
    for i=10,-2,-3 do
        prg:ground({{"p", {i}}})
        prg:solve()
    end
end

#end.

#program p(k).

-k $<= $x $<= k.
