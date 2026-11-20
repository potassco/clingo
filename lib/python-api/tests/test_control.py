"""
Unit tests for clingo.control module.
"""

from collections.abc import Mapping
from textwrap import dedent

import pytest
from clingo import ast
from clingo.control import Control
from clingo.core import Library
from clingo.ground import GroundResult
from clingo.symbol import Function, Number
from util import MCB, needs_threads


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

    def test_const_map(self):
        """
        Test the mapping protocol of the constant map.
        """
        ctl = Control(self.lib)
        ctl.parse_string("#const n=1. #const m=f(x).")
        ctl.ground()
        consts = ctl.const_map
        num = Number(self.lib, 1)
        fun = Function(self.lib, "f", [Function(self.lib, "x")])
        assert isinstance(consts, Mapping)
        assert consts.keys() >= {"n", "m"}
        assert ("n", num) in consts.items()
        assert ("n", fun) not in consts.items()
        assert fun in consts.values()
        assert 1 not in consts
        assert consts == dict(consts)
        with pytest.raises(TypeError):
            hash(consts)
        match consts:
            case {"n": n, "m": m}:
                assert n == num and m == fun
            case _:
                pytest.fail("mapping pattern did not match")

    @needs_threads
    def test_start_ground(self):
        """
        Test start_ground method with event handler and interruption.
        """
        ctl = Control(self.lib, ["--convert=text"])
        ctl.parse_string("#show.")
        ctl.parse_string("p(@fun(1)).")
        fres = None

        def f(res):
            nonlocal fres
            fres = res

        with ctl.start_ground(context=Context(self.lib), on_finish=f) as hnd:
            res = hnd.get()
        assert res == fres
        assert res == GroundResult.Ok
        assert ctl.buffer == dedent("""\
            p(1).
            p(2).
            #show.
            """)

    @needs_threads
    def test_start_ground_interrupt(self):
        """
        Test start_ground method with interruption.
        """
        ctl = Control(self.lib, ["--convert=text"])
        ctl.parse_string("#show.")
        ctl.parse_string("p(0).")
        ctl.parse_string("p(X+1) :- p(X).")
        with ctl.start_ground() as hnd:
            assert not hnd.wait(0.1)
            hnd.cancel()
            res = hnd.get()
        assert res == GroundResult.Interrupted

    def test_ground(self):
        """
        Test grounding.
        """
        ctl = Control(self.lib, ["--convert=text"])

        ctl.parse_string("a.")
        ctl.ground([("base", [])])
        ctl.parse_string("#program acid(k). b(k).")
        ctl.ground([("acid", [Number(self.lib, i)]) for i in range(5)])

        prg = ast.Program(self.lib)
        prg.add(ast.parse_statement(self.lib, "#program parse."))
        prg.add(ast.parse_statement(self.lib, "c :- a."))
        ctl.join(prg)
        ctl.ground([("parse", [])])

        assert ctl.buffer == dedent("""\
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
            """)

    def test_ground_context(self):
        """
        Test the grounding context.
        """

        ctl = Control(self.lib, ["--convert=text"])

        ctl.parse_string("p(@fun(1)).")
        ctl.parse_string("q(@gun(2)).")
        ctl.parse_string("#show.")
        ctl.ground([("base", [])], context=Context(self.lib))

        assert ctl.buffer == dedent("""\
            p(1).
            p(2).
            q(3).
            #show.
            """)

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
        assert ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [["a", "b"]]

    def test_incmode(self):
        """
        Test running the incremental mode from python.
        """
        ctl = Control(self.lib, ["0"])
        ctl.parse_string(dedent("""\
                #include <incmode>.

                #program base.

                {a;b;c}.

                #program step(k).

                { c(k) }.
                q(k) :- c(k).

                #program check(k).

                :- not c(3), query(k).
                """))
        # NOTE: we cannot intercept models here; the incmode is more
        # interesting for clingo-based apps.
        ctl.main()
        mcb = MCB()
        ctl.solve(on_model=mcb)
        assert all("c(3)" in mdl for mdl in mcb.symbols)
        assert len(mcb.symbols) == 32

    def test_assume(self):
        """
        Test passing assumptions to the solver.
        """
        ctl = Control(self.lib)
        ctl.parse_string("a.")
        ctl.main()
        res = ctl.solve(assumptions=[(Function(self.lib, "a"), True)])
        assert res.satisfiable
        res = ctl.solve(assumptions=[(Function(self.lib, "a"), False)])
        assert res.unsatisfiable
        res = ctl.solve(assumptions=[(Function(self.lib, "b"), True)])
        assert res.unsatisfiable
        res = ctl.solve(assumptions=[(Function(self.lib, "b"), False)])
        assert res.satisfiable
