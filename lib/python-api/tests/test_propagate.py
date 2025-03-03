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
        lib = init.library
        # get program literals for atoms `a` and `b`
        plit_a = init.base[Function(lib, "a")].literal
        plit_b = init.base[Function(lib, "b")].literal
        # get solver literals for program literals `a` and `b`
        self.slit_a = init.solver_literal(plit_a)
        self.slit_b = init.solver_literal(plit_b)
        # add watches for solver literals `a` and `b`
        init.add_watch(self.slit_a)
        init.add_watch(self.slit_b)

    def propagate(self, control: PropagateControl, changes: Sequence[int]) -> None:
        """
        Propagate solver literals `a` and `b`.
        """
        # if `a` is true imply `b`
        if self.slit_a in changes:
            assert control.assignment.is_true(self.slit_a)
            control.add_clause([-self.slit_a, self.slit_b])
        # if `b` is true imply `a`
        if self.slit_b in changes:
            assert control.assignment.is_true(self.slit_b)
            control.add_clause([-self.slit_b, self.slit_a])


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
