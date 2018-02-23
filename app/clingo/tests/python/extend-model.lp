#script (python)

import clingo
import sys


class Propagator:
    def init(self, init):
        self.models = 0

    def extend_model(self, threadId, complement):
        if complement:
            return [clingo.Number(self.models+1)]
        else:
            return [clingo.Number(17)]

    def on_model(self, m):
        sys.stdout.write("Answer: 0\n")
        sys.stdout.write("{}\n".format(" ".join(map(str, m.symbols(extra=True, complement=True)))))
        sys.stdout.flush()
        self.models += 1

def main(prg):
    p = Propagator()
    prg.register_propagator(p)
    prg.ground([("base", [])])
    prg.solve(on_model=p.on_model)

#end.

1{a;b;c}1.
