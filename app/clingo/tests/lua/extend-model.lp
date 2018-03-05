#script (lua)

clingo = require("clingo")

Propagator = { }
Propagator.__index = Propagator

function Propagator.new()
    local self = setmetatable({}, Propagator)
    self.models = 0
    return self
end

function Propagator:init(init)
    self.models = 0
end

function Propagator:extend_model(thread_id, complement)
    if complement then
        return {clingo.Number(self.models+1)}
    else
        return {clingo.Number(17)}
    end
end

function Propagator:on_model(m)
    io.write("Answer: 0\n")
    space = false
    for _, symbol in ipairs(m:symbols{extra=true, complement=true}) do
        if space then
            io.write(" ")
        else
            space = true
        end
        io.write(tostring(symbol))
    end
    io.write("\n")
    io.flush()
    self.models = self.models + 1
end

function main(prg)
    local p = Propagator.new()
    prg:register_propagator(p)
    prg:ground({{"base", {}}})
    prg:solve{on_model=function (m) p:on_model(m) end}
end

#end.

1{a;b;c}1.
