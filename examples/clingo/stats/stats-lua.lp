#script (lua)

json = require("json")

function main(prg)

    prg:ground({{"base", {}}})
    prg:solve()
    print(json.encode(prg.stats)) -- no pretty printing support

    print("lp.bodies", prg.stats["lp"]["bodies"])
    -- lua tables also support dot access
    print("lp.bodies", prg.stats.lp.bodies)
    print("optimization", prg.stats.costs[1])

end

#end.
