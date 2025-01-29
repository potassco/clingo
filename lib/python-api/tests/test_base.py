"""
Unit tests for clingo.base module.
"""

from textwrap import dedent
from unittest import TestCase

from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Function, Number


class TestScript(TestCase):
    """
    Tests for the control module.
    """

    def setUp(self):
        self._lib = Library()
        self._ctl = Control(self._lib)

    def tearDown(self):
        self._ctl = None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    @property
    def ctl(self) -> Control:
        """
        Get the control object.
        """
        assert self._ctl is not None
        return self._ctl

    def test_atom_base(self):
        """
        Test atom bases.
        """
        self.ctl.parse_string(
            dedent(
                """\
            p(1).
            { p(3) }.
            #external p(1..3).
            q(X) :- p(X).
            """
            )
        )
        self.ctl.ground()

        base = self.ctl.base
        self.assertEqual(len(base), 2)

        fun_p = Function(self.lib, "p", [Number(self.lib, 2)])
        sig_p = fun_p.signature()
        assert sig_p is not None

        self.assertIn(sig_p, base)
        self.assertIn((sig_p[0], sig_p[1]), base)

        base_p = base[sig_p]
        base_ps = base[(sig_p[0], sig_p[1])]
        self.assertEqual(len(base_p), len(base_ps))

        self.assertIn(fun_p, base_p)
        self.assertNotIn(Function(self.lib, "p", [Number(self.lib, 4)]), base_p)

        self.assertEqual(sorted(base), [("p", 1, False), ("q", 1, False)])
        self.assertEqual(
            sorted([(str(x.symbol), x.fact, x.external) for x in base_p.values()]),
            [("p(1)", True, False), ("p(2)", False, True), ("p(3)", False, False)],
        )

        self.assertGreater(base_p[fun_p].literal, 0)

    def test_term_base(self):
        """
        Test term bases.
        """
        self.ctl.parse_string(
            dedent(
                """\
            p(1).
            { x; p(3) }.
            #show q(3) : x.
            #show q(X) : p(X).
            """
            )
        )
        self.ctl.ground()

        base = self.ctl.base.terms
        self.assertEqual(len(base), 2)

        fun_q = Function(self.lib, "q", [Number(self.lib, 1)])
        self.assertIn(fun_q, base)
        self.assertNotIn(Function(self.lib, "q", [Number(self.lib, 2)]), base)

        term_q = base[fun_q]
        term_r = base[Function(self.lib, "q", [Number(self.lib, 3)])]
        self.assertEqual(term_q.symbol, fun_q)
        self.assertEqual(len(term_q.condition or []), 1)
        self.assertEqual(len(term_r.condition or []), 2)
