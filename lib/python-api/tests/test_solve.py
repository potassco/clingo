"""
Unit tests for clingo.solve module.
"""

from clingo.control import Control
from clingo.core import Library
from clingo.solve import Model, ModelType
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
        ofr = []
        with ctl.solve(on_model=mcb, on_finish=ofr.append) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        assert len(ofr) == 1
        assert ofr[0].satisfiable

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

    def test_control(self):
        """
        Test control.
        """
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("a. 1 {b; c; d} 1.")
        ctl.ground()

        def lit(name: str) -> Symbol:
            return Function(self.lib, name, [])

        atoms = [lit("b"), lit("c"), lit("d")]
        a = next(ctl.base[("a", 0)].values()).literal
        i = 0

        with ctl.solve(yield_=True) as hnd:
            for mdl in hnd:
                i += 1
                assert mdl.type == ModelType.StableModel
                assert mdl.number == i
                assert mdl.thread_id == 0
                assert mdl.contains(lit("a"))
                assert mdl.is_true(a)
                if len(atoms) == 3:
                    syms = [
                        atom.symbol
                        for base in mdl.control.base.values()
                        for atom in base.values()
                    ]
                    assert sorted(syms) == sorted(atoms + [lit("a")])
                    atoms = [atom for atom in atoms if not mdl.contains(atom)]
                    mdl.control.add_clause([(atoms.pop(), False)])
                else:
                    assert len(atoms) == 1
                    assert mdl.contains(atoms[0])
            assert hnd.get().satisfiable

    def test_optimize(self):
        """
        Test optimization.
        """
        ctl = Control(self.lib, [])
        ctl.parse_string("1 {a; b} 1. #minimize { 1:a; 2: b }.")
        ctl.ground()

        with ctl.solve(yield_=True) as hnd:
            for mdl in hnd:
                assert mdl.priorities == [0]
                assert not mdl.optimality_proven
            assert hnd.get().satisfiable
            last = hnd.last()
            assert last
            assert last.optimality_proven
            assert last.cost == [1]
            assert last.priorities == [0]

    def test_unsat(self):
        """
        Test lower bounds reported during optimization.
        """
        ctl = Control(self.lib, ["--opt-str=usc,oll,0", "--stats=2"])
        ctl.parse_string(
            "1 { p(X); q(X) } 1 :- X=1..3. #minimize { 1,p,X: p(X); 1,q,X: q(X) }.",
        )
        ctl.ground()
        lower = []
        with ctl.solve(on_unsat=lower.append) as hnd:
            assert hnd.get().satisfiable
        assert ctl.stats["summary"]["lower"] == [3.0]
        assert lower == [[1], [2], [3]]

    def test_consequence(self):
        """
        Test enumeration of consequences.
        """
        ctl = Control(self.lib, ["--enum-mode=brave"])
        ctl.parse_string("a. 1 {b; c} 1. {d}. :- d, b. :- d, c.")
        ctl.ground()

        def lit(name: str) -> int:
            return next(ctl.base[(name, 0)].values()).literal

        with ctl.solve(yield_=True) as hnd:
            it = iter(hnd)
            mdl = next(it)
            n1, n2 = "b", "c"
            if mdl.is_true(lit("c")):
                n2, n1 = n1, n2
            assert mdl.type == ModelType.BraveConsequences
            assert mdl.is_consequence(lit("a"))
            assert mdl.is_consequence(lit(n1))
            assert mdl.is_consequence(lit(n2)) is None
            assert mdl.is_consequence(lit("d")) is None
            mdl = next(it)
            assert mdl.type == ModelType.BraveConsequences
            assert mdl.is_consequence(lit("a"))
            assert mdl.is_consequence(lit(n1)) is True
            assert mdl.is_consequence(lit(n2)) is True
            assert mdl.is_consequence(lit("d")) is None
            try:
                mdl = next(it)
            except StopIteration:
                pass
            assert hnd.get().satisfiable
            last = hnd.last()
            assert last
            assert last.type == ModelType.BraveConsequences
            assert last.is_consequence(lit("a"))
            assert last.is_consequence(lit("b"))
            assert last.is_consequence(lit("c"))
            assert last.is_consequence(lit("d")) is False

    def test_extend(self):
        """
        Test extending models.
        """

        ctl = Control(self.lib, [])
        ctl.ground()
        ctl.parse_string("a.")
        ctl.ground()

        def sym(name: str) -> Symbol:
            return Function(self.lib, name, [])

        def extend(mdl: Model):
            mdl.extend([sym("b"), sym("c")])

        with ctl.solve(on_model=extend) as hnd:
            assert hnd.get().satisfiable
            last = hnd.last()
            assert last
            assert last.symbols(theory=True) == [sym("b"), sym("c")]
            assert last.symbols(shown=True) == [sym("a"), sym("b"), sym("c")]
