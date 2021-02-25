#script (python) 

from clingo import Number, String, Function

import threading
import time

class State:
    def __init__(self, imin, imax, iloop, isecond):
        self.step = 0
        self.first_solution_food = False
        self.ret = None
        self.timer = None
        self.iteration_without_improvement = 0
        self.all_time_best_solution = [float('inf')]
        self.iter_best_solution = [float('inf')]
        self.min = imin.number
        if imax is None:
            self.max = None
        else:
            self.max = imax.number
        self.loop = iloop.number
        self.second = isecond.number

    def __on_model(self, m):
        if len(m.cost) > 0:
            if self.timer.is_alive():
                self.timer.cancel()
                self.timer.join()
            self.timer = threading.Timer(self.second, self.prg.interrupt)
            if sum(m.cost) < sum(self.iter_best_solution):
                self.iter_best_solution = m.cost
            if not self.timer.is_alive():
                self.timer.start()

    def run(self, prg):
        self.prg = prg
        while ((self.max is None or self.step < self.max) and
               (self.step == 0 or self.step < self.min or not self.first_solution_food)) or self.iteration_without_improvement < self.loop:
            self.iter_best_solution = [float('inf')]
            parts = []
            if self.step > 0:
                parts.append(("step", [Number(self.step)]))
            else:
                parts.append(("base", []))
            self.prg.ground(parts)
            self.timer = threading.Timer(self.second, self.prg.interrupt)
            self.timer.start()
            self.ret, self.step = self.prg.solve(on_model=self.__on_model), self.step+1
            if sum(self.iter_best_solution) < sum(self.all_time_best_solution):
                self.all_time_best_solution = self.iter_best_solution
                self.prg.configuration.solve.opt_mode = "opt," + ",".join([str(x) for x in self.all_time_best_solution])
                self.iteration_without_improvement = 0
                self.first_solution_food = True
            else:
                self.iteration_without_improvement += 1
            if self.timer.is_alive():
                self.timer.cancel()
                self.timer.join()

def get(val, default):
    return val if val != None else default

def main(prg):
    imin   = get(prg.get_const("imin"), Number(0))
    imax   = prg.get_const("imax")
    iloop   = get(prg.get_const("iloop"), Number(3))
    isecond   = get(prg.get_const("isecond"), Number(60))

    x = State(imin, imax, iloop, isecond)
    x.run(prg)
#end.

#program step(t).
