#script(lua)

function make_on_finish(stop)
    return function (ret, interrupted)
        stop[1] = true
    end
end

function main(prg)
    stop = {false}
    prg:ground({{"base", {}}})
    future, n, m = prg:solve_async(nil, nil, make_on_finish(stop)), 0, 0
    while not stop[1] do
        x, y, m = math.random(), math.random(), m+1
        if x * x + y * y < 1 then
            n = n + 1
        end
    end
    future:wait()
    print(string.format("approximation of pi: %f", 4.*n/m))
end

#end.
