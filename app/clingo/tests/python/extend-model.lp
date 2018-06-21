#script (python)

import clingo
import sys

def on_model(m):
    m.extend([clingo.Number(17)])

def main(prg):
    prg.ground([("base", [])])
    prg.solve(on_model=on_model)

#end.

1{a;b;c}1.
