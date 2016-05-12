#script (lua) 

function get(val, default)
    if val ~= nil then 
        return val 
    else 
        return default 
    end
end

function main(prg)
    local imin   = get(prg:get_const("imin"), 0)
    local imax   = prg:get_const("imax")
    local istop  = get(prg:get_const("istop"), "SAT")

    local step, ret = 0, gringo.SolveResult.UNKNOWN
    while (imax == nil or step < imax) and
          (step == 0   or step < imin or (
              (istop == "SAT"     and ret ~= gringo.SolveResult.SAT) or
              (istop == "UNSAT"   and ret ~= gringo.SolveResult.UNSAT) or 
              (istop == "UNKNOWN" and ret ~= gringo.SolveResult.UNKNOWN))) do
        local parts = {}
        table.insert(parts, {"check", {step}})
        if step > 0 then
            prg:release_external(gringo.Fun("query", {step-1}))
            table.insert(parts, {"step", {step}})
            prg:cleanup_domains()
        else
            table.insert(parts, {"base", {}})
        end
        prg:ground(parts)
        prg:assign_external(gringo.Fun("query", {step}), true)
        ret, step = prg:solve(), step+1
    end
end

#end.

#program check(t).
#external query(t).
