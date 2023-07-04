#script (lua)

require("clingo")

local Pigeonator = { }
Pigeonator.__index = Pigeonator

function Pigeonator.new()
    local self = setmetatable({ }, Pigeonator)
    self.place = {} -- shared state
    return self
end

function Pigeonator:init(init)
    for atom in init.symbolic_atoms:by_signature("place", 2) do
        local lit = init:solver_literal(atom.literal)
        -- store hole H of each place(P,H) atom
        self.place[lit] = atom.symbol.arguments[2].number
        init:add_watch(lit)
    end
    for i = 1, init.number_of_threads do
        init:set_state(i, {})
    end
end

function Pigeonator:propagate(control, changes, holes)
    for _, lit in ipairs(changes) do
        local hole = self.place[lit]
        if holes[hole] == nil then
            holes[hole] = lit
        end
        local prev = holes[hole]
        if prev ~= lit and not control:add_nogood{{lit, prev}} then
            return
        end
    end
end

function Pigeonator:undo(thread_id, assignment, changes, holes)
    for _, lit in ipairs(changes) do
        local hole = self.place[lit]
        if holes[hole] == lit then
            holes[hole] = nil
        end
    end
end

function main(prg)
    prg:register_propagator(Pigeonator.new())
    prg:ground({{"base", {}}})
    prg:solve()
end

#end.

#const h = 5.
#const p = h+1.

1 { place(P,H) : H = 1..h } 1 :- P = 1..p.
% { place(P,H) : P = 1..p } 1 :- H = 1..h.
