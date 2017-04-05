#script (lua)

function make_on_model()
    print ("Solving...")
    counter = 1
    return function (m)
        print ("Answer: "  .. tostring(counter))
        print (m)
        counter = counter + 1
    end
end

function main(prg)
    prg:ground({{"base", {}}, {"check", {0}}})
    prg:solve_refactored{on_model=make_on_model()}
    prg:ground({{"check", {1}}})
    prg:solve_refactored{on_model=make_on_model()}
    prg:ground({{"check", {2}}})
    prg:solve_refactored{on_model=make_on_model()}
    prg:ground({{"check", {3}}})
    prg:solve_refactored{on_model=make_on_model()}
    print ("SATISFIABLE")
    print ()
end

#end.

#program base.

1 { p(0..3) } 1.

$x $= 1.

#program check(k).

#show p/1.
#show $x : p(k).
