#script (lua)

clingo = require("clingo")

function on_model(m)
    m:extend{clingo.Number(17)}
end

function main(prg)
    prg:ground({{"base", {}}})
    prg:solve{on_model=on_model}
end

#end.

1{a;b;c}1.
