"""
Simple example showing how to resume optimization to enumerate all optimal
values in lexicographical order.

# TODO
- add an option to control whether to restore the last solution candidate
- add constraints to establish upper bounds with more than one priority
- maybe print the models in a nicer way
- maybe add the number of optimal models to the user statistics
"""
import sys
from typing import cast, Dict, List, Optional, Sequence, Tuple

from clingo.configuration import Configuration
from clingo.backend import Backend, Observer
from clingo.control import Control
from clingo.application import Application
from clingo.solving import SolveResult, Model
from clingo.propagator import Propagator, Assignment, PropagateControl
from clingo import clingo_main


class MinObs(Observer):
    literals: Dict[int, List[Tuple[int, int]]]

    def __init__(self):
        self.literals = {}

    def minimize(self, priority: int, literals: Sequence[Tuple[int,int]]):
        self.literals.setdefault(priority, []).extend(literals)

class RestoreHeu(Propagator):
    costs: Dict[Tuple[int], Sequence[int]]
    models: Dict[int, Sequence[int]]
    restore: Optional[Sequence[int]]

    def __init__(self):
        self.costs = {}
        self.models = {}
        self.restore = None

    def set_restore(self, bound: Tuple[int]):
        min_costs = bound
        for costs, model in self.costs.items():
            if bound < costs < min_costs:
                min_costs = costs
                self.restore = model

    def on_model(self, model: Model):
        idx = cast(Tuple[int], tuple(model.cost))
        self.costs[idx] = self.models[model.thread_id]

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
    _restore: bool

    def __init__(self):
        # TODO: this should be controlled via an option
        self._restore = True

    def _set_upper_bound(self, backend: Backend, minimize: Sequence[Sequence[Tuple[int, int]]], bound: Sequence[int]):
        assert len(minimize) == len(bound)
        if len(minimize) != 1:
            raise RuntimeError('only one priority level accepted for now')

        # TODO: multiple priority level should be supported
        lower = -bound[0]
        wlits = []
        for l, w in minimize[0]:
            if w > 0:
                lower += w
                l = -l
            else:
                w = -w
            wlits.append((l, w))
        backend.add_weight_rule([], lower, wlits)

    def _optimize(self, control: Control):
        obs = MinObs()
        control.register_observer(obs)

        heu = None
        on_model = None
        if self._restore:
            heu = RestoreHeu()
            on_model = heu.on_model
            control.register_propagator(heu)

        control.ground([('base', [])])
        res = cast(SolveResult, control.solve(on_model=on_model))

        solve_config = cast(Configuration, control.configuration.solve)

        num_models = int(cast(str, solve_config.models))

        minimize = [ x[1] for x in sorted(obs.literals.items(), key=lambda x: x[0]) ]

        while (res.satisfiable and not res.interrupted and
               minimize and 'costs' in control.statistics['summary']):
            summary = control.statistics['summary']
            if num_models > 0:
                num_models -= int(summary['models']['optimal'])
                if num_models <= 0:
                    break
                solve_config.models = num_models

            costs = cast(Tuple[int], tuple(int(x) for x in control.statistics['summary']['costs']))
            with control.backend() as backend:
                self._set_upper_bound(backend, minimize, costs)

            if heu is not None:
                heu.set_restore(costs)

            res = cast(SolveResult, control.solve(on_model=on_model))

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
