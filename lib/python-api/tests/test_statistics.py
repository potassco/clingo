"""
Unit tests for the clingo.statistics module.
"""

from clingo.control import Control
from clingo.core import Library
from util import MCB


class TestStatistics:
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

    def test_solve(self):
        """
        Test the solver statistics.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib)
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()
        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        # TODO: add tests

    def test_user(self):
        """
        Test the user statistics.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib)
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()
        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        # TODO: add tests
