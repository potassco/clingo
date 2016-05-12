#script(python)

def main(prg):
    n = 0
    parts = []
    for arg in prg.get_const("calls").args:
        lower = arg.args[0].number
        upper = arg.args[1].number
        while n < upper:
            n += 1
            parts.append(("board", [n]))
            if n >= lower:
                prg.ground(parts)
                parts = []
                print 'SIZE {0}'.format(n)
                prg.solve()

#end.
