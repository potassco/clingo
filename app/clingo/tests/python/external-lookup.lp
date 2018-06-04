#script (python)

def foo(x):
    return bar(x) + 1

def main(prg):
    global bar
    def bar(x):
        return 42 + x.number
    prg.ground([("base", [])])
    prg.solve()
    global foobar
    def foobar(x):
        return x.number
    prg.ground([("test", [])])
    prg.solve()

#end.

p(@bar(10)).
p(@foo(10)).
p(@foo(X)) :- X=1.
p(@bar(X)) :- X=1.

#program test.

q(@foobar(5)).
