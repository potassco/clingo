"""
Unit tests for the clingo.stats module.
"""

import pytest
from clingo.control import Control
from clingo.core import Library
from util import MCB


class TestStats:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the stats module.
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

    def test_solve(self):
        """
        Test the solver stats.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()
        mcb = MCB()
        with ctl.solve(on_model=mcb, async_=True, yield_=True) as hnd:
            with pytest.raises(ValueError):
                _ = ctl.stats
            for _ in hnd:
                pass
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        stats = ctl.stats
        assert isinstance(stats, dict)
        cpu = stats["summary"]["times"]["cpu"]
        assert isinstance(cpu, float) and cpu > 0.0

    def test_user(self):
        """
        Test the user stats.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()
        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        # TODO: add tests
