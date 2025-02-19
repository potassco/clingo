"""
Unit tests for clingo.base module.
"""

from textwrap import dedent

from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Function, Number


class TestBase:
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
        self._ctl = Control(self._lib)

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
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
        assert len(base) == 2

        fun_p = Function(self.lib, "p", [Number(self.lib, 2)])
        sig_p = fun_p.signature()
        assert sig_p is not None

        assert sig_p in base
        assert (sig_p[0], sig_p[1]) in base

        base_p = base[sig_p]
        base_ps = base[(sig_p[0], sig_p[1])]
        assert len(base_p) == len(base_ps)

        assert fun_p in base_p
        assert Function(self.lib, "p", [Number(self.lib, 4)]) not in base_p

        assert sorted(base) == [("p", 1, False), ("q", 1, False)]
        assert sorted(
            [
                (str(x.symbol), base.is_fact(x.literal), base.is_external(x.literal))
                for x in base_p.values()
            ]
        ) == [("p(1)", True, False), ("p(2)", False, True), ("p(3)", False, False)]

        assert base_p[fun_p].literal > 0

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
        assert len(base) == 2

        fun_q = Function(self.lib, "q", [Number(self.lib, 1)])
        assert fun_q in base
        assert Function(self.lib, "q", [Number(self.lib, 2)]) not in base

        term_q = base[fun_q]
        term_r = base[Function(self.lib, "q", [Number(self.lib, 3)])]
        assert term_q.symbol == fun_q
        assert len(term_q.condition or []) == 1
        assert len(term_r.condition or []) == 2

    def test_theory_base(self):
        """
        Test term bases.
        """
        self.ctl.parse_string(
            dedent(
                """\
                #theory x {
                    a {
                        - : 1, unary;
                        + : 2, binary, left;
                        - : 3, binary, right;
                        + : 4, unary
                    };
                    b {
                        * : 1, binary, left;
                        / : 2, binary, right
                    };
                    &p/0: a, {<,>}, b, any

                }.
                &p { +f(1,"x",[1,2],(2,3),{4,5})-y }.
                """
            )
        )
        self.ctl.ground()
        assert len(self.ctl.base.theory) == 1
        atom = self.ctl.base.theory[0]
        assert str(atom) == '&p { (+(f(1,"x",[1,2],(2,3),{4,5}))-y) }'
        # TODO: more
        # - test atom
        # - test element
        # - test term
