#script (lua)

function main(prg)
    for i=1,6 do
        print("******************blub!!!!!!!!!!!!!!!!!!!!!")
        prg:ground("step", {i})
        prg:solve()
    end
end

#end.

#program step(t).
1 { step(t,1..5) }.
:- not 3 { step(t-1,X); step(t,X) }.
#minimize { 1,step(t,X):step(t,X) }.

