"""
Unit tests for the clingo.config module.
"""

import re

from clingo.control import Control
from clingo.core import Library
from util import MCB


class TestSolving:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the solving module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    def test_config(self):
        """
        Test inspection of config objects.
        """
        ctl = Control(self.lib)
        cfg = ctl.config
        assert "solve" in cfg.attributes
        assert re.search("^solve:", str(ctl.config), re.MULTILINE) is not None
        assert "models" in cfg.solve.attributes
        assert cfg.solve.models.is_value
        assert not cfg.solve.models.is_sequence
        assert cfg.solve.models.description.startswith("Compute")
        cfg.solve.models.value = -1
        assert str(cfg.solve.models) == '"-1"'
        assert cfg.solve.models.value == "-1"
        assert "solver" in cfg.attributes
        assert cfg.solver.is_sequence
        assert not cfg.solver.is_value
        assert len(cfg.solver) >= 1
        assert "heuristic" in cfg.solver[0].attributes

    def test_solve(self):
        """
        Test if config updates apply to solving.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib)
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()

        ctl.config.solve.models = 0
        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == res

        ctl.config.solve.models = 2
        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert len(mcb.symbols) == 2

        ctl.config.solve.models = 0
        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == res
