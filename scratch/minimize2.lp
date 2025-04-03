#script (lua)

function main(prg)
    prg:ground("base", {})
    for i=1,11 do
        print("******************blub!!!!!!!!!!!!!!!!!!!!!")
        prg:ground("step", {i})
        prg:solve()
    end
end

#end.

#program base.
{ p(1..10) }.
#minimize { 1,X:p(X) }.

#program step(t).
:- not t { p(X) }.
