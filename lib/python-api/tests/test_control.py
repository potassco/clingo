"""
Unit tests for clingo.control module.
"""

from textwrap import dedent

from clingo import ast
from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Number
from util import MCB


class TestControl:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the control module.
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

    def test_ground(self):
        """
        Test grounding.
        """
        ctl = Control(self.lib, ["--text-buffer", "--mode=ground"])

        ctl.parse_string("a.")
        ctl.ground([("base", [])])
        ctl.parse_string("#program acid(k). b(k).")
        ctl.ground([("acid", [Number(self.lib, i)]) for i in range(5)])

        prg = ast.Program(self.lib)
        prg.add(ast.parse_statement(self.lib, "#program parse."))
        prg.add(ast.parse_statement(self.lib, "c :- a."))
        ctl.join(prg)
        ctl.ground([("parse", [])])

        assert ctl.buffer == dedent(
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

        assert ctl.buffer == dedent(
            """\
            p(1).
            p(2).
            q(3).
            #show.
            """
        )

    def test_join(self):
        """
        Test adding a single parsed statement to a program.
        """
        ctl = Control(self.lib, [])
        ctl.parse_string("a.")

        prg = ast.Program(self.lib)
        prg.add(ast.parse_statement(self.lib, "b :- a."))
        ctl.join(prg)

        ctl.ground([("base", [])])

        mcb = MCB()
        with ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == [["a", "b"]]
