"""
Unit tests for clingo.propagate module.
"""

from typing import Sequence

from clingo.control import Control
from clingo.core import Library
from clingo.propagate import PropagateControl, PropagateInit, Propagator
from clingo.symbol import Function
from util import MCB


class AIFFB(Propagator):
    """
    Simple propagator ensuring a iff b.
    """

    slit_a: int
    slit_b: int

    def __init__(self) -> None:
        super().__init__()
        self.slit_a = 0
        self.slit_b = 0

    def init(self, init: PropagateInit) -> None:
        """
        Add watches for atoms `a` and `b`.
        """

        def watch(p):
            plit = init.base[Function(init.library, p)].literal
            slit = init.solver_literal(plit)
            init.add_watch(slit)
            return slit

        self.slit_a = watch("a")
        self.slit_b = watch("b")

    def propagate(self, control: PropagateControl, changes: Sequence[int]) -> None:
        """
        Propagate solver literals `a` and `b`.
        """

        def propagate(p, q):
            # propagate a implies b
            if p in changes:
                assert control.assignment.is_true(p)
                control.add_clause([-p, q])

        propagate(self.slit_a, self.slit_b)
        propagate(self.slit_b, self.slit_a)


class TestPropgate:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the propagate module.
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

    def test_aiffb(self):
        """
        Test the example a iff b propagator.
        """
        self.ctl.register_propagator(AIFFB())
        self.ctl.parse_string("1 { a; b }.")
        self.ctl.ground()
        mcb = MCB()
        with self.ctl.solve(on_model=mcb) as hnd:
            assert hnd.get().satisfiable
        assert mcb.symbols == [["a", "b"]]
