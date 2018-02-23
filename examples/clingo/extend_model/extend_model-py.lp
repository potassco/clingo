#script (python)

import clingo

class Propagator:
    def init(self, init):
        self.check_calls = 0
        init.check_mode = clingo.PropagatorCheckMode.Total

    def check(self, ctl):
        self.check_calls += 1
        return True

    def extend_model(self, thread_id, complement):
        return [clingo.Number(self.check_calls)]

def on_model(m):
    print ("Extra symbols: {}".format(" ".join(map(str, m.symbols(extra=True)))))

def main(prg):
    prg.register_propagator(Propagator())
    prg.ground([("base", [])])
    prg.solve(on_model=on_model)

#end.

1{a;b;c}1.
