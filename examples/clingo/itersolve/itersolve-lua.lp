#script(lua)

function main(prg)
    prg:ground({{"base", {}}})
    local it = prg:solve_iter()
    local ret, err = pcall(function()
        for m in it:iter() do
            print(m)
        end
        local res = it:get()
        print (res)
    end)
    it:close()
    if not ret then
        error(err)
    end
end

#end.
