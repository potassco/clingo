"""
Unit tests for the --convert option.
"""

from textwrap import dedent

import pytest
from clingo.control import Control
from clingo.core import Library


def param(value: str, arg_name="ctl") -> pytest.MarkDecorator:
    """
    Helper function to create parameterized tests for the convert option.
    """
    return pytest.mark.parametrize(arg_name, [value], indirect=True)


class TestConvert:
    """
    Tests for the --convert option.
    """

    @pytest.fixture
    def lib(self) -> Library:
        """
        Create library.
        """
        return Library()

    @pytest.fixture
    def ctl(self, lib: Library, request: pytest.FixtureRequest) -> Control:
        """
        Create control with convert option.
        """
        return Control(lib, [f"--convert={request.param}"])

    @param("smodels")
    def test_smodels(self, ctl: Control) -> None:
        """
        Test smodels conversion.
        """
        ctl.parse_string("1 {a; b} 1.")
        ctl.ground()
        ctl.solve()
        assert ctl.buffer == dedent("""\
            90 0
            1 2 0 0
            3 1 3 2 0 2 4
            3 1 5 2 0 2 4
            1 1 2 1 6 2
            2 7 2 0 1 8 9
            2 10 2 0 2 8 9
            1 6 2 1 10 7
            1 4 0 0
            1 8 2 0 4 3
            1 9 2 0 4 5
            0
            3 a
            5 b
            0
            B+
            0
            B-
            1
            0
            1
            """)

    @param("aspif")
    def test_aspif(self, ctl: Control) -> None:
        """
        Test aspif conversion.
        """
        ctl.parse_string("1 {a; b} 1.")
        ctl.ground()
        ctl.solve()
        assert ctl.buffer == dedent("""\
            asp 2 0 0 incremental
            1 0 1 1 0 0
            1 1 1 3 0 2 1 2
            1 1 1 4 0 2 1 2
            1 0 0 0 2 -7 1
            4 0 3 1 a
            4 0 4 1 b
            1 0 1 8 1 1 2 5 1 6 1
            1 0 1 9 1 2 2 5 1 6 1
            1 0 1 7 0 2 8 -9
            1 0 1 2 0 0
            1 0 1 5 0 2 2 3
            1 0 1 6 0 2 2 4
            0
            """)

    @param("reify")
    def test_reify(self, ctl: Control) -> None:
        """
        Test reify conversion.
        """
        ctl.parse_string("1 {a; b} 1.")
        ctl.ground()
        ctl.solve()
        assert ctl.buffer == dedent("""\
            tag(incremental).
            atom_tuple(0).
            atom_tuple(0,1).
            literal_tuple(0).
            rule(disjunction(0),normal(0)).
            atom_tuple(1).
            atom_tuple(1,3).
            literal_tuple(1).
            literal_tuple(1,1).
            literal_tuple(1,2).
            rule(choice(1),normal(1)).
            atom_tuple(2).
            atom_tuple(2,4).
            rule(choice(2),normal(1)).
            atom_tuple(3).
            literal_tuple(2).
            literal_tuple(2,-7).
            literal_tuple(2,1).
            rule(disjunction(3),normal(2)).
            outputAtom(a,3).
            outputAtom(b,4).
            atom_tuple(4).
            atom_tuple(4,8).
            weighted_literal_tuple(0).
            weighted_literal_tuple(0,5,1).
            weighted_literal_tuple(0,6,1).
            rule(disjunction(4),sum(0,1)).
            atom_tuple(5).
            atom_tuple(5,9).
            rule(disjunction(5),sum(0,2)).
            atom_tuple(6).
            atom_tuple(6,7).
            literal_tuple(3).
            literal_tuple(3,-9).
            literal_tuple(3,8).
            rule(disjunction(6),normal(3)).
            atom_tuple(7).
            atom_tuple(7,2).
            rule(disjunction(7),normal(0)).
            atom_tuple(8).
            atom_tuple(8,5).
            literal_tuple(4).
            literal_tuple(4,2).
            literal_tuple(4,3).
            rule(disjunction(8),normal(4)).
            atom_tuple(9).
            atom_tuple(9,6).
            literal_tuple(5).
            literal_tuple(5,2).
            literal_tuple(5,4).
            rule(disjunction(9),normal(5)).
            """)
