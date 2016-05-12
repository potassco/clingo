#script(python)

def main(prg):
    n = 0
    parts = []
    for lower, upper in prg.get_const("calls").args():
        while n < upper:
            n += 1
            parts.append(("board", [n]))
            if n >= lower:
                prg.ground(parts)
                parts = []
                print 'SIZE {0}'.format(n)
                prg.solve()

#end.
