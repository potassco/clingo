"""
Unit tests for clingo.backend module.
"""

from clingo.backend import TheorySequenceType
from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Function, Number


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

    def test_backend(self):
        """
        Test add statetments.
        """

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
        with self.ctl.solve() as hnd:
            assert hnd.get().satisfiable

        with self.ctl.backend as bck:
            n = Function(self.lib, "p", [])
            num = bck.theory_number(1)
            bck.theory_atom(0, n, [], ("<=", num))
        assert str(self.ctl.base.theory[0]) == "&p { } <= 1"
        with self.ctl.solve() as hnd:
            assert hnd.get().satisfiable
