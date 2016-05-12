#script (lua)

function main(prg)
    prg:ground("base", {})
    prg:solve()
    prg:ground("base", {})
    prg:solve()
end

#end.

a.
