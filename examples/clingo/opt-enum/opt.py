"""
Simple example showing how to resume optimization to enumerate all optimal
values in lexicographical order.

# TODO
- add an option to control whether to restore the last solution candidate
- maybe print the models in a nicer way
- maybe add the number of optimal models to the user statistics
"""
import sys
from typing import cast, Dict, List, Optional, Sequence, Tuple

from clingo.application import clingo_main, Application
from clingo.backend import Backend, Observer
from clingo.configuration import Configuration
from clingo.control import Control
from clingo.propagator import Propagator, Assignment, PropagateControl
from clingo.solving import SolveResult, Model


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
    _aux_level: Dict[Tuple[int, int], int]

    def __init__(self):
        # TODO: this should be controlled via an option
        self._restore = True
        self._aux_level = {}

    def _add_upper_bound(self, backend: Backend, wlits: Sequence[Tuple[int, int]], bound: int, level: Optional[int]):
        '''
        Adds the constraint `a <> { wlits } < bound` and returns litera `a`.

        If level is None, then an integrity constraint is added and no
        auxiliary literal is introduced.

        This function reuses literals introduced in earlier iterations.
        '''
        hd = []
        if level:
            if (level, bound) in self._aux_level:
                return self._aux_level[(level, bound)]
            hd.append(backend.add_atom())
            self._aux_level[(level, bound)] = hd[0]

        lower = -bound
        wlits_lower = []
        for l, w in wlits:
            if w > 0:
                lower += w
                l = -l
            else:
                w = -w
            wlits_lower.append((l, w))

        backend.add_weight_rule(hd, lower, wlits_lower)
        return level and hd[0]

    def _set_upper_bound(self, backend: Backend, minimize: Sequence[Sequence[Tuple[int, int]]], bound: Sequence[int]):
        '''
        Adds constraints discarding solutions lexicographically smaller or
        equal than the bound.

        The weighted literals in the minimize variable directly correspond to
        how the solver represents minimize constraints.
        '''
        assert minimize and len(minimize) == len(bound)
        if len(minimize) == 1:
            self._add_upper_bound(backend, minimize[0], bound[0], None)
        else:
            # Note: we could also introduce a chain. But then there are
            # typically few priorities and this should resolve nicely.
            # :- l0 <= b0-1
            # :- l0 <= b0 && l1 <= b1-1
            # :- l0 <= b0 && l1 <= b1 && l2 <= b2-1
            # ...
            # :- l0 <= b0 && l1 <= b1 && l2 <= b2 && ... && ln <= bn
            prefix = []
            for i, (wlits, value) in enumerate(zip(minimize, bound)):
                if i == len(minimize) - 1:
                    prefix.append(self._add_upper_bound(backend, wlits, value - 1, i))
                    backend.add_rule([], prefix)
                    prefix[-1] = self._add_upper_bound(backend, wlits, value, i)
                else:
                    prefix.append(self._add_upper_bound(backend, wlits, value, i))
                    backend.add_rule([], prefix)

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
