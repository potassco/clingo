#script(python)

def main(prg):
    prg.ground([("base", [])])
    with prg.solve_iter() as it:
        for m in it: print m
        print it.get()

#end.
