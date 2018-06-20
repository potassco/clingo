#script (python)

import clingo

def add_statistics(statistics):
    statistics.update({
        "test": 10,
        "x": [1, 2, 3],
        "y": {
            "a": 2,
            "b": [1,2,3]
        }
    })
    statistics["test"] = 20
    statistics["x"] = [99, 9]
    statistics["x"].extend([4,5,6])
    for i in range(1, 10):
        statistics["test"] += i
    statistics.update({
        "y": {
            "a": lambda x: x+40,
            "c": 99,
            "b": [0]
        }
    })

def tosymbol(x):
    if isinstance(x, dict):
        ret = []
        for key in sorted(x):
            ret.append(clingo.Tuple([key, tosymbol(x[key])]))
        return clingo.Function("dict", ret)
    elif isinstance(x, list):
        ret = []
        for y in x:
            ret.append(tosymbol(y))
        return clingo.Function("list", ret)
    else:
        return clingo.Number(int(x))

def tomodel():
    return tosymbol(user)

def main(prg):
    prg.ground([("base", [])])
    with prg.solve(async=True) as handle:
        handle.get()
        add_statistics(handle.user_statistics(True))
    global user
    user = prg.statistics["user_accu"]
    prg.ground([("two", [])])
    prg.solve()

#end.

#program two.
p(@tomodel()).
