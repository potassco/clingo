"""
Unit tests for evaluating arithmetic terms during grounding.

The expected values are computed with Python integers, which, like clingo's
numbers, have arbitrary precision and use floor division.
"""

from collections.abc import Callable

import pytest
from clingo.control import Control
from clingo.core import Library

# values around the boundaries of 32bit integers
VALUES = [
    -(2**31),
    -(2**31) + 1,
    -46341,
    -7,
    -2,
    -1,
    0,
    1,
    2,
    7,
    46341,
    65536,
    2**31 - 2,
    2**31 - 1,
]

BINARY: dict[str, tuple[str, Callable[[int, int], int | None]]] = {
    "plus": ("X+Y", lambda x, y: x + y),
    "minus": ("X-Y", lambda x, y: x - y),
    "times": ("X*Y", lambda x, y: x * y),
    "div": ("X/Y", lambda x, y: x // y if y != 0 else None),
    "mod": ("X\\Y", lambda x, y: x % y if y != 0 else None),
    "pow": ("X**Y", lambda x, y: x**y if 0 <= y <= 3 else None),
    "and": ("X&Y", lambda x, y: x & y),
    "or": ("X?Y", lambda x, y: x | y),
    "xor": ("X^Y", lambda x, y: x ^ y),
}

UNARY: dict[str, tuple[str, Callable[[int], int]]] = {
    "minus": ("-X", lambda x: -x),
    "abs": ("|X|", abs),
    "negate": ("~X", lambda x: ~x),
}

LINEAR: dict[str, tuple[str, Callable[[int], int]]] = {
    "inc": ("X+1", lambda x: x + 1),
    "dec": ("X-1", lambda x: x - 1),
    "neg": ("5-X", lambda x: 5 - x),
    "times": ("2*X+3", lambda x: 2 * x + 3),
    "big": ("1000*X-7", lambda x: 1000 * x - 7),
}


class TestArith:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for evaluating arithmetic terms.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self.messages: list[str] = []
        self._lib = Library(logger=lambda _, msg: self.messages.append(msg))

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

    def ground(self, program: str) -> set[str]:
        """
        Ground the given program together with the values and return its atoms.
        """
        ctl = Control(self.lib)
        values = ";".join(str(v) for v in VALUES)
        ctl.parse_string(f"v({values}). {program}")
        ctl.ground()
        atoms: set[str] = set()
        ctl.solve(
            on_model=lambda mdl: atoms.update(str(s) for s in mdl.symbols(shown=True))
        )
        return atoms

    @pytest.mark.parametrize("name", BINARY)
    def test_binary(self, name: str):
        """
        Test evaluating binary operations.
        """
        term, fun = BINARY[name]
        # NOTE: large exponents would produce huge numbers
        cond = ", 0 <= Y, Y <= 3" if name == "pow" else ""
        atoms = self.ground(f"r(X,Y,{term}) :- v(X), v(Y){cond}. #show r/3.")
        expected = {
            f"r({x},{y},{res})"
            for x in VALUES
            for y in VALUES
            if (res := fun(x, y)) is not None
        }
        assert atoms == expected

    @pytest.mark.parametrize("name", UNARY)
    def test_unary(self, name: str):
        """
        Test evaluating unary operations.
        """
        term, fun = UNARY[name]
        atoms = self.ground(f"r(X,{term}) :- v(X). #show r/2.")
        assert atoms == {f"r({x},{fun(x)})" for x in VALUES}

    def test_unary_match(self):
        """
        Test matching the unary minus against values.
        """
        atoms = self.ground("r(X) :- v(-X). #show r/1.")
        assert atoms == {f"r({-y})" for y in VALUES}

    @pytest.mark.parametrize("name", LINEAR)
    def test_linear_eval(self, name: str):
        """
        Test evaluating linear terms.
        """
        term, fun = LINEAR[name]
        atoms = self.ground(f"r(X,{term}) :- v(X). #show r/2.")
        assert atoms == {f"r({x},{fun(x)})" for x in VALUES}

    @pytest.mark.parametrize("name", LINEAR)
    def test_linear_match(self, name: str):
        """
        Test matching linear terms against values.
        """
        term, fun = LINEAR[name]
        # the variable only occurs in the linear term, which has to be solved
        atoms = self.ground(
            f"r(X) :- v({term}). s(X,Y) :- v(Y), Y = {term}. #show r/1. #show s/2."
        )
        expected = {f"r({x})" for y in VALUES for x in self.solve(fun, y)}
        expected |= {f"s({x},{y})" for y in VALUES for x in self.solve(fun, y)}
        assert atoms == expected

    @staticmethod
    def solve(fun: Callable[[int], int], y: int) -> list[int]:
        """
        Solve fun(x) == y for integral x given a linear function.
        """
        n = fun(0)
        m = fun(1) - n
        return [(y - n) // m] if (y - n) % m == 0 else []

    def test_division_by_zero(self):
        """
        Test that division by zero is reported and does not produce atoms.
        """
        atoms = self.ground("r(X/Y) :- v(X), v(Y), X = 1, Y = 0. #show r/1.")
        assert not any(a.startswith("r(") for a in atoms)
        assert any("operation undefined" in msg for msg in self.messages)
