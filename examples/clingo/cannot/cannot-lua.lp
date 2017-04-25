#script (lua)

Cannot = { }
Cannot.__index = Cannot

function table.append(table, value)
    table[#table + 1] = value
    return table
end

function table.shallow_copy(t)
  local ret = {}
  for k, v in pairs(t) do
    ret[k] = v
  end
  return ret
end

function Cannot.new()
    local self = setmetatable({ }, Cannot)
    self.atoms = {}
    self.states = {}
    return self
end

function Cannot:check(ctl)
    local assign = ctl.assignment
    local state = self.states[ctl.thread_id]
    state.messages = {}
    state.invalid = false
    for _, atom in ipairs(self.atoms) do
        if assign:is_true(atom.solver_literal) then
            state.invalid = true
            for _, elem in ipairs(atom.elements) do
                if assign:is_true(elem.solver_literal) then
                    table.append(state.messages, elem.message)
                end
            end
        end
    end
end

function Cannot:init(init)
    for atom in init.theory_atoms do
        if atom.term.name == "cannot" and #atom.term.arguments == 0 then
            local a = {}
            a.literal = atom.literal
            a.solver_literal = init:solver_literal(atom.literal)
            a.elements = {}
            for _, elem in ipairs(atom.elements) do
                local e = {}
                local t = elem.terms
                e.message = string.sub(t[1].name, 2, -2)
                for i = 2, #t do
                    e.message = string.gsub(e.message, "%%", tostring(t[i]), 1)
                end
                e.condition = elem.condition
                e.solver_literal = init:solver_literal(elem.condition_id)
                table.append(a.elements, e)
            end
            table.append(self.atoms, a)
        end
    end
    for i = 1, init.number_of_threads do
        local state = {}
        state.messages = {}
        state.invalid = {}
        self.states[i] = state
    end
end

function main(prg)
    prg:ground({{"base", {}}})
    -- minimize the number of messages
    local backend = prg.backend
    local lits = {}
    for atom in prg.theory_atoms do
        if atom.term.name == "cannot" and #atom.term.arguments == 0 then
            for _, elem in ipairs(atom.elements) do
                local condition = table.shallow_copy(elem.condition)
                table.append(condition, atom.literal)
                if #condition > 1 then
                    atom = backend:add_atom()
                    backend:add_rule{{atom}, condition}
                    condition[1] = atom
                end
                table.append(lits, {condition[1], 1})
            end
        end
    end
    backend:add_minimize{0, lits}
    -- solve where the propagator takes care of collecting the messages
    local p = Cannot.new()
    prg:register_propagator(p)
    local it = prg:solve{yield=true}
    local res, ret = pcall(function()
        for m in it:iter() do
            local state = p.states[m.thread_id]
            if state.invalid then
                print("ERROR: invalid answer set")
                for _, msg in ipairs(state.messages) do
                    print ("- cannot " .. msg)
                end
            end
        end
    end)
    it:close()
    if not res then
        error(ret)
    end
end

#end.

#theory cannot {
    t { };
    &cannot/0: t, head
}.
