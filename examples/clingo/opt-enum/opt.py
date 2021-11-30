import sys
from typing import cast, Sequence, Tuple

from clingo.configuration import Configuration
from clingo.backend import Observer
from clingo.control import Control
from clingo.application import Application
from clingo.solving import SolveResult, Model
from clingo.propagator import Propagator, Assignment, PropagateControl
from clingo import clingo_main


class MinObs(Observer):
    def __init__(self):
        self.literals = None

    def minimize(self, priority: int, literals: Sequence[Tuple[int,int]]):
        if priority != 0:
            raise RuntimeError('only priority level 0 accepted for now')

        if self.literals is None:
            self.literals = []

        self.literals.extend(literals)


class RestoreHeu(Propagator):
    def __init__(self):
        self.costs = {}
        self.models = {}
        self.restore = None

    def set_restore(self, bound: int):
        min_cost = None
        for costs, model in self.costs.items():
            if costs[0] > bound and (min_cost is None or min_cost > costs[0]):
                min_cost = costs[0]
                self.restore = model

    def on_model(self, model: Model):
        self.costs[tuple(model.cost)] = self.models[model.thread_id]

    def check(self, control: PropagateControl):
        self.models[control.thread_id] = list(control.assignment.trail)
        self.restore = None

    def decide(self, thread_id: int, assignment: Assignment, fallback: int) -> int:
        if self.restore:
            for lit in self.restore:
                if assignment.is_free(lit):
                    return lit
        return fallback


class OptApt(Application):
    def __init__(self):
        self._bound = None

    def _optimize(self, control: Control):
        obs = MinObs()
        control.register_observer(obs)

        heu = RestoreHeu()
        control.register_propagator(heu)

        control.ground([('base', [])])
        res = cast(SolveResult, control.solve(on_model=heu.on_model))

        solve_config = cast(Configuration, control.configuration.solve)

        num_models = int(cast(str, solve_config.models))

        while (res.satisfiable and not res.interrupted and
               obs.literals is not None and 'costs' in control.statistics['summary']):
            summary = control.statistics['summary']
            if num_models > 0:
                num_models -= int(summary['models']['optimal'])
                if num_models <= 0:
                    break
                solve_config.models = num_models

            bounds = control.statistics['summary']['costs']
            with control.backend() as backend:
                assert len(bounds) == 1
                bound = -int(bounds[0])
                for _, w in obs.literals:
                    bound += w
                backend.add_weight_rule([], bound, [(-l, w) for (l, w) in obs.literals])

            heu.set_restore(int(bounds[0]))

            res = cast(SolveResult, control.solve(on_model=heu.on_model))

    def main(self, control: Control, files: Sequence[str]):
        for file_ in files:
            control.load(file_)
        if not files:
            control.load('-')

        solve_config = cast(Configuration, control.configuration.solve)

        if solve_config.opt_mode == 'optN':
            self._optimize(control)
        else:
            control.ground([('base', [])])
            control.solve()


sys.exit(clingo_main(OptApt()))
