"""
Unit tests for incremental solving.
"""

from clingo.control import Control
from clingo.core import Library
from clingo.symbol import parse_term
from util import MCB


class TestIncremental:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for incremental solving.
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

    def test_simplify(self):
        """
        Test simplification of incremental programs.
        """
        ctl = Control(self.lib)
        ctl.parse_string("""\
            d(1..3).
            p(X) :- not q(X), d(X), X!=1.
            q(X) :- not p(X), d(X), X!=3.
            """)
        ctl.ground()
        sp2 = parse_term(self.lib, "p(2)")
        lq1 = ctl.base[parse_term(self.lib, "q(1)")].literal
        lq2 = ctl.base[parse_term(self.lib, "q(2)")].literal
        lp2 = ctl.base[sp2].literal
        lp3 = ctl.base[parse_term(self.lib, "p(3)")].literal
        assert not ctl.base.is_fact(lq2)
        assert not ctl.base.is_fact(lp2)
        assert ctl.base.is_fact(lq1) != ctl.base.is_fact(lp3)
        ctl.solve()
        assert ctl.base.is_fact(lq1)
        assert ctl.base.is_fact(lp3)
        assert not ctl.base.is_fact(lq2)
        assert not ctl.base.is_fact(lp2)
        ctl.parse_string("#program x. :- p(2).")
        ctl.ground([("x", [])])
        ctl.solve()
        bp, bq = ctl.base[("p", 1)], ctl.base[("q", 1)]
        assert len(bp) == 1
        assert len(bq) == 2
        assert sp2 not in ctl.base

    def test_simplify_minimize(self):
        """
        Test simplification of incremental minimize statements.
        """

        ctl = Control(self.lib, ["--opt-mode=optN", "0"])
        ctl.parse_string("""\
            {a; b}.
            #minimize { 1: a; 2 : not b }.
            """)
        ctl.ground()
        mcb = MCB()
        ctl.solve(on_model=mcb)
        assert ctl.stats["summary"]["costs"].nestify() == [0.0]
        assert mcb.symbols == [["b"]]
        ctl.parse_string("#program x. :- not a. :- not b.")
        ctl.ground([("x", [])])
        mcb = MCB()
        ctl.solve(on_model=mcb)
        assert ctl.stats["summary"]["costs"].nestify() == [1.0]
        assert mcb.symbols == [["a", "b"]]
        ctl.parse_string("#program y. {c}. #minimize{ 1: b; 2 : not c }.")
        ctl.ground([("y", [])])
        mcb = MCB()
        ctl.solve(on_model=mcb)
        assert ctl.stats["summary"]["costs"].nestify() == [1.0]
        assert mcb.symbols == [["a", "b", "c"]]
