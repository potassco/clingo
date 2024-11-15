"""
Unit tests for clingo.control module.
"""

from textwrap import dedent
from unittest import TestCase

from clingo.ast import Program, parse_statement
from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Number


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

    def test_control(self):
        """
        Test the control class.
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
                #show a: a.
                #show.
                b(0).
                b(1).
                b(2).
                b(3).
                b(4).
                #show b(0): b(0).
                #show b(1): b(1).
                #show b(2): b(2).
                #show b(3): b(3).
                #show b(4): b(4).
                c.
                #show c: c.
                """
            ),
        )

    def test_context(self):
        """
        Test the control class.
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
