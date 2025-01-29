"""
Unit tests for clingo.control module.
"""

from textwrap import dedent
from unittest import TestCase

from clingo.ast import Program, parse_statement
from clingo.control import Control
from clingo.core import Library
from clingo.solving import Model
from clingo.symbol import Number


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


class TestScript(TestCase):
    """
    Tests for the control module.
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

    def test_ground(self):
        """
        Test grounding.
        """
        ctl = Control(self.lib, ["--text-buffer", "--mode=ground"])

        ctl.parse_string("a.")
        ctl.ground([("base", [])])
        ctl.parse_string("#program acid(k). b(k).")
        ctl.ground([("acid", [Number(self.lib, i)]) for i in range(5)])

        prg = Program(self.lib)
        prg.add(parse_statement(self.lib, "#program parse."))
        prg.add(parse_statement(self.lib, "c :- a."))
        ctl.join(prg)
        ctl.ground([("parse", [])])

        self.assertEqual(
            ctl.buffer,
            dedent(
                """\
                a.
                #show a/0.
                #show.
                b(0).
                b(1).
                b(2).
                b(3).
                b(4).
                #show b/1.
                c.
                #show c/0.
                """
            ),
        )

    def test_ground_context(self):
        """
        Test the grounding context.
        """

        class Context:
            """
            Simple test context.
            """

            def __init__(self, lib):
                """
                Initialize the context.
                """
                self._lib = lib

            def fun(self, arg):
                """
                Test function f.
                """
                return [arg, Number(self._lib, arg.number + 1)]

            def gun(self, arg):
                """
                Test function g.
                """
                return Number(self._lib, arg.number + 1)

        ctl = Control(self.lib, ["--text-buffer", "--mode=ground"])

        ctl.parse_string("p(@fun(1)).")
        ctl.parse_string("q(@gun(2)).")
        ctl.parse_string("#show.")
        ctl.ground([("base", [])], context=Context(self.lib))

        self.assertEqual(
            ctl.buffer,
            dedent(
                """\
                p(1).
                p(2).
                q(3).
                #show.
                """
            ),
        )

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
