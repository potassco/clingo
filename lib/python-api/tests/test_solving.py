"""
Unit tests for clingo.solving module.
"""

from unittest import TestCase

from clingo.control import Control
from clingo.core import Library
from clingo.solving import Model
from clingo.symbol import Function, Symbol


class MCB:
    """
    Helper to intercept symbols while solving.
    """

    def __init__(self) -> None:
        self._syms = []

    def __call__(self, mdl: Model):
        self._syms.append(sorted(mdl.symbols(shown=True)))

    @property
    def symbols(self):
        """
        Get the collected symbols.
        """
        return [[str(sym) for sym in syms] for syms in sorted(self._syms)]


class TestSolving(TestCase):
    """
    Tests for the solving module.
    """

    def setUp(self):
        self._lib = Library()

    def tearDown(self):
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
            self.assertTrue(hnd.get().satisfiable)
        self.assertEqual(mcb.symbols, res)

        # yield
        mcb = MCB()
        mcc = MCB()
        with ctl.solve(on_model=mcb, yield_=True) as hnd:
            for mdl in hnd:
                mcc(mdl)
            self.assertTrue(hnd.get().satisfiable)
        self.assertEqual(mcb.symbols, res)
        self.assertEqual(mcc.symbols, res)

        # async
        mcb = MCB()
        with ctl.solve(on_model=mcb, async_=True) as hnd:
            self.assertTrue(hnd.get().satisfiable)
        self.assertEqual(mcc.symbols, res)

        # yield+async
        mcb = MCB()
        mcc = MCB()
        with ctl.solve(on_model=mcb, yield_=True, async_=True) as hnd:
            while True:
                hnd.resume()
                mdl = hnd.model()
                if mdl is None:
                    break
                mcc(mdl)
            self.assertTrue(hnd.get().satisfiable)
        self.assertEqual(mcb.symbols, res)
        self.assertEqual(mcc.symbols, res)

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
            self.assertTrue(hnd.get().unsatisfiable)
            self.assertEqual(sorted(hnd.core()), sorted(assumptions[:2]))
