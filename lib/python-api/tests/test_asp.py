"""
Acceptence tests for logic programs.

This are mainly tests that were used in clingo 5 to detect bugs.
"""

import json
from importlib.resources import files
from unittest import TestCase

from clingo.control import Control
from clingo.core import Library
from clingo.solving import Model


class MCB:
    """
    Helper to intercept symbols while solving.
    """

    def __init__(self) -> None:
        self._syms = []

    def __call__(self, mdl: Model):
        self._syms.append(sorted(mdl.symbols(shown=True)))

    @property
    def symbols(self) -> list[list[str]]:
        """
        Get the collected symbols.
        """
        return [[str(sym) for sym in syms] for syms in sorted(self._syms)]


def make_test(path):
    prg = path.read_text()

    def method(self):
        opt = json.loads(
            "\n".join(line[2:] for line in prg.splitlines() if line.startswith("%%"))
        )

        lib = Library()
        ctl = Control(lib, opt["options"])

        mcb = MCB()
        ctl.parse_string(prg)
        ctl.ground()
        ctl.solve(on_model=mcb)
        self.assertEqual(mcb.symbols, opt["solutions"])
        mcb, ctl, lib = None, None, None

    return method


def make_tests(cls):
    for path in files(__name__).joinpath("resources").iterdir():
        n = path.name.replace(".lp", "").replace("-", "_")
        method = make_test(path)
        method.__name__ = f"test_{n}"
        setattr(cls, method.__name__, method)
    return cls


@make_tests
class TestASP(TestCase):
    """
    Tests for the solving module.
    """
