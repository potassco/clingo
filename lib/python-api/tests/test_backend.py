"""
Unit tests for clingo.backend module.
"""

from clingo.backend import ExternalType, HeuristicType, TheorySequenceType
from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Function, Number
from util import MCB


class TestBackend:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the backend module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()
        self._ctl = Control(self._lib, ["0"])

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

    def test_rule(self):
        """
        Test add rule.
        """
        with self.ctl.backend as bck:
            p1 = Function(self.lib, "p", [Number(self.lib, 1)])
            p2 = Function(self.lib, "p", [Number(self.lib, 2)])
            p3 = Function(self.lib, "p", [Number(self.lib, 3)])
            p4 = Function(self.lib, "p", [Number(self.lib, 4)])
            a1 = bck.atom(p1)
            a2 = bck.atom(p2)
            a3 = bck.atom(p3)
            a4 = bck.atom(p4)
            bck.rule([a1])
            bck.rule([a2], choice=True)
            bck.rule([a3], [a2])
            bck.weight_rule([a4], 4, [(a1, 2), (a2, 1), (a3, 1)])

        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [["p(1)"], ["p(1)", "p(2)", "p(3)", "p(4)"]]

    def test_edge(self):
        """
        Test add edge.
        """
        with self.ctl.backend as bck:
            a = bck.atom(Function(self.lib, "a"))
            b = bck.atom(Function(self.lib, "b"))
            bck.rule([a, b], choice=True)
            bck.edge(1, 2, [a])
            bck.edge(2, 1, [b])

        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [[], ["a"], ["b"]]

    def test_external(self):
        """
        Test external.
        """

        def run(t):
            with self.ctl.backend as bck:
                a = bck.atom(Function(self.lib, "a"))
                bck.external(a, t)
            mcb = MCB()
            assert self.ctl.solve(on_model=mcb).satisfiable
            return mcb.symbols

        assert run(ExternalType.Free) == [[], ["a"]]
        assert run(ExternalType.False_) == [[]]
        assert run(ExternalType.True_) == [["a"]]
        assert run(ExternalType.Release) == [[]]

    def test_assume(self):
        """
        Test assume.
        """

        def run(t):
            with self.ctl.backend as bck:
                a = bck.atom(Function(self.lib, "a"))
                bck.assume([a if t else -a])
            mcb = MCB()
            self.ctl.solve(on_model=mcb).satisfiable
            return mcb.symbols

        with self.ctl.backend as bck:
            a = bck.atom(Function(self.lib, "a"))
            bck.rule([a], choice=True)

        assert run(True) == [["a"]]
        assert run(False) == [[]]

    def test_project(self):
        """
        Test project.
        """
        self.ctl.config.solve.project = "auto"
        with self.ctl.backend as bck:
            a = bck.atom(Function(self.lib, "a"))
            b = bck.atom(Function(self.lib, "b"))
            c = bck.atom(Function(self.lib, "c"))
            d = bck.atom(Function(self.lib, "d"))
            bck.rule([a, b, c, d])
            bck.project([a, b])
        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert len(mcb.symbols) == 3

    def test_minimize(self):
        """
        Test minimize.
        """
        with self.ctl.backend as bck:
            a = bck.atom(Function(self.lib, "a"))
            b = bck.atom(Function(self.lib, "b"))
            c = bck.atom(Function(self.lib, "c"))
            d = bck.atom(Function(self.lib, "d"))
            bck.rule([a, b, c, d], choice=True)
            bck.minimize([(a, 1), (b, -1), (c, 1), (d, -1)])

        with self.ctl.start_solve() as hnd:
            assert hnd.get().satisfiable
            last = hnd.last()
            assert last is not None
            syms = [str(sym) for sym in sorted(last.symbols(shown=True))]
            assert syms == ["b", "d"]

    def test_heuristic(self):
        """
        Test heuristic.
        """
        self.ctl.config.solver.heuristic = "domain"
        self.ctl.config.solve.models = "1"
        with self.ctl.backend as bck:
            a = bck.atom(Function(self.lib, "a"))
            b = bck.atom(Function(self.lib, "b"))
            c = bck.atom(Function(self.lib, "c"))
            d = bck.atom(Function(self.lib, "d"))
            bck.rule([a, b, c, d], choice=True)
            bck.heuristic(a, HeuristicType.True_, 1)
            bck.heuristic(b, HeuristicType.False_, 1)
            bck.heuristic(c, HeuristicType.True_, 1)
            bck.heuristic(d, HeuristicType.False_, 1)
        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [["a", "c"]]

    def test_theory(self):
        """
        Test adding theory atoms.
        """
        with self.ctl.backend as bck:
            n = Function(self.lib, "p", [Number(self.lib, 2)])
            txt = bck.theory_string("a")
            sym = bck.theory_symbol(n)
            num = bck.theory_number(1)
            lst = bck.theory_sequence(TheorySequenceType.List, [txt, num])
            lot = bck.theory_sequence(TheorySequenceType.Set, [num, sym])
            tup = bck.theory_sequence(TheorySequenceType.Tuple, [sym, txt])
            fun = bck.theory_function("f", [sym, txt, num])
            e = bck.theory_element([fun, lst, lot, tup], [])
            bck.theory_atom(0, n, [e])
        assert (
            str(self.ctl.base.theory[0])
            == "&p(2) { f(p(2),a,1),[a,1],{1,p(2)},(p(2),a) }"
        )
        assert self.ctl.solve().satisfiable

        with self.ctl.backend as bck:
            n = Function(self.lib, "p", [])
            num = bck.theory_number(1)
            bck.theory_atom(0, n, [], ("<=", num))
        assert str(self.ctl.base.theory[0]) == "&p { } <= 1"
        assert self.ctl.solve().satisfiable
