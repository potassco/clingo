'''
Helpers to simplify testing.
'''

from unittest import TestCase

from .. import SolveResult, parse_term

def _p(*models):
    return [[parse_term(symbol) for symbol in model] for model in models]

class _MCB:
    # pylint: disable=missing-function-docstring
    def __init__(self):
        self._models = []
        self._core = None
        self.last = None

    def on_core(self, c):
        self._core = c

    def on_model(self, m):
        self.last = (m.type, sorted(m.symbols(shown=True)))
        self._models.append(self.last[1])

    @property
    def core(self):
        return sorted(self._core)

    @property
    def models(self):
        return sorted(self._models)

def _check_sat(case: TestCase, ret: SolveResult) -> None:
    case.assertTrue(ret.satisfiable is True)
    case.assertTrue(ret.unsatisfiable is False)
    case.assertTrue(ret.unknown is False)
    case.assertTrue(ret.exhausted is True)
