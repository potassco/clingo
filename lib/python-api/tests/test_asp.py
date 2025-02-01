"""
Acceptence tests for logic programs.

This are mainly tests that were used in clingo 5 to detect bugs.
"""

import json
from importlib.resources import files

import pytest
from clingo.control import Control
from clingo.core import Library
from clingo.solving import Model

FILES = [path.name for path in files(__name__).joinpath("resources").iterdir()]


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


class TestASP:
    """
    Tests for the solving module.
    """

    def run_file(self, prg):
        """
        Run individual test program.
        """
        opt = json.loads(
            "\n".join(line[2:] for line in prg.splitlines() if line.startswith("%%"))
        )
        lib = Library()
        ctl = Control(lib, opt["options"])
        mcb = MCB()
        ctl.parse_string(prg)
        ctl.ground()
        ctl.solve(on_model=mcb)
        assert mcb.symbols == opt["solutions"]
        mcb, ctl, lib = None, None, None

    @pytest.mark.parametrize("file", FILES)
    def test_file(self, file):
        """
        Run all test programs found under resources.
        """
        for path in files(__name__).joinpath("resources").iterdir():
            if path.name == file:
                self.run_file(path.read_text())
                return
        assert False
