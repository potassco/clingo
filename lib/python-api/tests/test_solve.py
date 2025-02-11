"""
Unit tests for clingo.solve module.
"""

from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Function, Symbol
from util import MCB


class TestSolve:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the solve module.
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
        Test solving.
        """
        res = [["a"], ["b"]]
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("1 { a; b } 1.")
        ctl.ground()

        # default
        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == res

        # yield
        mcb = MCB()
        mcc = MCB()
        with ctl.solve(on_model=mcb, yield_=True) as hnd:
            for mdl in hnd:
                mcc(mdl)
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        assert mcc.symbols == res

        # async
        mcb = MCB()
        with ctl.solve(on_model=mcb, async_=True) as hnd:
            assert hnd.get().satisfiable
        assert mcc.symbols == res

        # yield+async
        mcb = MCB()
        mcc = MCB()
        with ctl.solve(on_model=mcb, yield_=True, async_=True) as hnd:
            while mdl := hnd.model():
                mcc(mdl)
                hnd.resume()
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        assert mcc.symbols == res

    def test_core(self):
        """
        Test solving.
        """
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("{a; b; c}. :- a, b.")
        ctl.ground()

        def lit(name: str) -> tuple[Symbol, bool] | int:
            fun = Function(self.lib, name, [])
            return ctl.base[(name, 0)][fun].literal

        assumptions = [lit("a"), lit("b"), lit("c")]

        with ctl.solve(assumptions=assumptions) as hnd:
            assert hnd.get().unsatisfiable
            assert sorted(hnd.core()) == sorted(assumptions[:2])
