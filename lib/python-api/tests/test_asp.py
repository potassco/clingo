"""
Acceptence tests for logic programs.

This are mainly tests that were used in clingo 5 to detect bugs.
"""

import json
from pathlib import Path

import pytest
from clingo.control import Control
from clingo.core import Library
from util import MCB

FILES = [path.name for path in Path(__file__).parent.joinpath("resources").iterdir()]


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
        for path in Path(__file__).parent.joinpath("resources").iterdir():
            if path.name == file:
                self.run_file(path.read_text())
                return
        assert False
