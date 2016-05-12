#script (lua)

function map(func, array)
  for i,v in ipairs(array) do
    array[i] = func(v)
  end
  return array
end

function sorted(t)
    table.sort(t)
    return t
end

function convert(ret)
    if     ret == gringo.SolveResult.SAT then return "SAT"
    elseif ret == gringo.SolveResult.UNSAT then return "UNSAT"
    elseif ret == gringo.SolveResult.UNKNOWN then return "UNKNOWN"
    end
end

function on_model(x)
    print("Answer: 42")
    print(
           "hasA(" ..        tostring(x:contains(gringo.Fun("a"))) .. ") "
        .. "hasVolatile(" .. tostring(x:contains(gringo.Fun("volatile", {9}))) .. ") "
        .. "model(" ..       table.concat(sorted(map(tostring, x:atoms())), ",") .. ") ")
end

function on_finish(ret, int) 
    print("Answer: 42")
    print(convert(ret) .. "_" .. tostring(int))
end

function main (prg)
    n = prg:get_const("n")
    Fun = gringo.Fun
    parts = {}
    parts[#parts+1] = {"base", {}}
    prg:ground(parts)
    prg:solve()
    parts = {}
    prg:add("test", {"x"}, "test(x).")
    parts[#parts+1] = {"test", {Fun("f", {1,2})}}
    for i=1,n do
        parts[#parts+1] = {"cumulative", {i}}
        prg:ground(parts)
        parts = {}
        f = prg:solve_async(nil, on_model, on_finish)
        f:wait(0)
        ret = f:get()
        print("Answer: 42")
        print(convert(ret))
    end
end

#end.

#const n = 10.

#program base.

a.

#program cumulative(k).

1 { c(k) } 1.
q(k) :- c(k).
r(k,X) :- test(X).

#external volatile(k).
 :- not c(5), not volatile(k).
volatile(k-1).

