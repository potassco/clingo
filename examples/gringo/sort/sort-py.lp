#script (python)

import clingo

values = []

def gather(a):
    values.append(a)
    return 1

def sort():
    return list(enumerate(sorted(values)))

#end.
