#script (lua) 

function run(prg, conn)
    local States = {
        ["SOLVE"] = 1,
        ["IDLE"]  = 2,
    }
    local SolveResults = {
        [gringo.SolveResult.SAT]     = "SAT",
        [gringo.SolveResult.UNSAT]   = "UNSAT",
        [gringo.SolveResult.UNKNOWN] = "UNKNOWN",
    }
    local state = States.IDLE
    local k = 0
    prg:ground({{"pigeon", {}}, {"sleep",  {k}}})
    prg:assign_external(gringo.Fun("sleep", {k}), true)
    while true do
        local f, ret, msg, err
        if state == States.SOLVE then
            f = prg:solve_async(
                nil,
                function (model) conn:send("Answer: "..tostring(model).."\n") end, 
                function (ret, int)
                    local msg = "finish:"..SolveResults[ret]
                    if int then msg = msg..":INTERRUPTED" end
                    conn:send(msg.."\n") 
                end)
        end
        msg, err = conn:receive("*l")
        if msg == nil then error(err) end
        if state == States.SOLVE then
            f:cancel()
            ret = f:get()
        else
            ret = gringo.SolveResult.UNKNOWN
        end
        if msg == "interrupt" then
            state = States.IDLE
        elseif msg == "exit" then
            return
        elseif msg == "less_pigeon_please" then
            prg:assign_external(gringo.Fun("p"), false)
            state = States.IDLE
        elseif msg == "more_pigeon_please" then
            prg:assign_external(gringo.Fun("p"), true)
            state = States.IDLE
        elseif msg == "solve" then
            state = States.SOLVE
        else error("unexpected message: " .. msg) end
        if ret ~= gringo.SolveResult.UNKNOWN then
            k = k + 1
            prg:ground({{"sleep",  {k}}})
            prg:release_external(gringo.Fun("sleep", {k-1}))
            prg:assign_external(gringo.Fun("sleep", {k}), true)
        end
    end
end

function main(prg)
    file = io.open(".controller.PORT", "r")
    if file == nil then error("apparently no server is running") end
    p = tonumber(file:read("*all"))
    file:close()
    os.remove(".controller.PORT")
    socket = require "socket"
    conn = socket.tcp()
    ret, err = conn:connect("127.0.0.1", p)
    if ret == nil then error(err) end
    ret, err = pcall(run, prg, conn)
    conn:close()
    if not ret then error(err) end
end

#end.
