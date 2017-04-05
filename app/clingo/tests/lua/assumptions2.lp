#script (lua)

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve_refactored{assumptions={{clingo.Function("b"), true}}}
end

#end.

{ a }.
b :- a.
