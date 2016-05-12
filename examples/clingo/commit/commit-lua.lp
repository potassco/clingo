#script (lua)

function echo(x)
    print (x)
    return x
end

function main(prg)
    prg:ground({{"test", {1}}})
    print ("---------- commited ----------")
    prg:ground({{"test", {2}}, {"test", {3}}})
    print ("---------- commited ----------")
    prg:ground({{"test", {4}}})
    prg:solve()
end

#end.
