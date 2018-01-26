#script (python)

import sys
import time
import clingo

### This propagator counts the models and adds a respective symbol to the model for inspection
class Propagator:
    def init(self, init):
	self.model = 0
        init.check_mode = clingo.PropagatorCheckMode.Total

    def check(self, ctl):
	self.model = self.model+1
        return True

    def extend_model(self, threadId, complement, ret):
        ret.append(clingo.Number(self.model))

def on_model(m):
    print("This is the model with number: " + " ".join(map(str, m.symbols(extra=True))))

def main(prg):
    prg.register_propagator(Propagator())
    prg.ground([("base", [])])
    prg.solve(on_model=on_model)
    sys.stdout.write("\n")

#end.


1{a;b;c}1.
