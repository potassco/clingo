#script (lua)

function print_conf(conf, ident)
    for _, x in ipairs(conf:keys()) do
        key = string.gsub(x, "[.]", "")
        if key == x then
            print (ident .. key .. "[=" .. tostring(conf[key]) .. "] - " .. conf["__desc_" .. key])
        else
            subconf = conf[key]
            label   = key
            if #subconf >= 0 then
                label = label .. "[0.." .. tostring(#subconf) .. "]"
            end
            print (ident .. label .. " - " .. conf["__desc_" .. key])
            print_conf(subconf, "  " .. ident .. label .. ".")
        end
    end
end

function main(prg)
    prg:ground({{"base", {}}})
    
    print_conf(prg.conf, "")

    print ("The heuristics of the solvers in the 'many' portfolio:")
    prg.conf.configuration = "many"
    prg.conf.solve.parallel_mode = 3 -- just use the first 3 solvers
    for x in prg.conf.solver:iter() do
        print ("  " .. x.heuristic)
    end

    prg.conf.solve.models = 0
    print ("==================== All Models ===================")
    prg:solve()

    prg.conf.solve.models = 1
    print ("===================== One Model ===================")
    prg:solve()

    prg.conf.solve.models = 0
    prg.conf.solve.enum_mode = "cautious"
    print ("=============== Cautious Consequences =============")
    prg:solve()

    prg.conf.solve.models = 0
    prg.conf.solve.enum_mode = "brave"
    print ("================ Brave Consequences ===============")
    prg:solve()
end

#end.

{ a; b; c }.
