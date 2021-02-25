#script (python) 

from clingo import Number, String, Function

import threading
import time

class IterData:

    def __init__(self, timer, iter_best_solution):
        self.timer = timer
        self.iter_best_solution = iter_best_solution

def handle_stop(iter_data):
    def on_model(ret):
        print("ok")
        if len(ret.cost) > 0:
            if iter_data.timer.is_alive():
                iter_data.timer.cancel()
                iter_data.timer.join()
            iter_data.timer = threading.Timer(60.0, prg.interrupt)
            if sum(ret.cost) < sum(iter_data.iter_best_solution):
                iter_data.iter_best_solution = ret.cost
            if not iter_data.timer.is_alive():
                iter_data.timer.start()


def get(val, default):
    return val if val != None else default

def main(prg):
    imin   = get(prg.get_const("imin"), Number(0))
    imax   = prg.get_const("imax")
    iloop   = get(prg.get_const("iloop"), Number(3))
    iconst   = get(prg.get_const("isecond"), Number(60))

    step, ret = 0, None
    iteration_without_improvement, all_time_best_solution = 0, [float('inf')]
    while ((imax is None or step < imax.number) and
           (step == 0 or step < imin.number or not ret.satisfiable)) or iteration_without_improvement < iloop.number:
        iter_data = IterData(threading.Timer(iconst.number, prg.interrupt), [float('inf')])
        parts = []
        if step > 0:
            parts.append(("step", [Number(step)]))
        else:
            parts.append(("base", []))
        prg.ground(parts)
        iter_data.timer.start()
        ret, step = prg.solve(on_model=handle_stop(iter_data)), step+1
        if sum(iter_data.iter_best_solution) < sum(all_time_best_solution):
            all_time_best_solution = iter_data.iter_best_solution
            print("opt," + ",".join(all_time_best_solution))
            prg.configuration.solve.opt_mode = "opt," + ",".join(all_time_best_solution)
            iteration_without_improvement = 0
        else:
            iteration_without_improvement += 1
        if iter_data.timer.is_alive():
            iter_data.timer.cancel()
            iter_data.timer.join()
#end.

#program step(t).
