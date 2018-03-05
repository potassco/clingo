#script (lua)

clingo = require("clingo")

Propagator = { }
Propagator.__index = Propagator

function Propagator.new()
    local self = setmetatable({}, Propagator)
    self.check_calls = 0
    return self
end

function Propagator:init(init)
    init.check_mode = clingo.PropagatorCheckMode.Total
    self.check_calls = 0
end

function Propagator:check(ctl)
    self.check_calls = self.check_calls + 1
    return true
end

function Propagator:extend_model(thread_id, complement)
    return {clingo.Number(self.check_calls)}
end

function Propagator:on_model(m)
    io.write("Extra symbols:")
    for _, symbol in ipairs(m:symbols{extra=true}) do
        io.write(" ")
        io.write(tostring(symbol))
    end
    io.write("\n")
    io.flush()
end

function main(prg)
    local p = Propagator.new()
    prg:register_propagator(p)
    prg:ground({{"base", {}}})
    prg:solve{on_model=function (m) p:on_model(m) end}
end

#end.

1{a;b;c}1.
