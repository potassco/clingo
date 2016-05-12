#script (python)

def echo(x):
    print (x);
    return x

def main(prg):
    prg.ground([("test", [1])])
    print "---------- commited ----------"
    prg.ground([("test", [2]), ("test", [3])])
    print "---------- commited ----------"
    prg.ground([("test", [4])])
    prg.solve()
#end.
