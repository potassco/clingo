"""
Unit tests for clingo.core module.
"""

from clingo.core import Library
from clingo.symbol import (
    Function,
    Infimum,
    Number,
    String,
    Supremum,
    Tuple_,
    parse_term,
)


class TestSymbol:
    # pylint: disable=attribute-defined-outside-init
    """
    Unit tests for clingo.core module.
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

    def test_number(self):
        """
        Test number symbols.
        """
        small = 13
        big = 10000000000000000000000003
        assert Number(self.lib, small).number == small
        assert Number(self.lib, big).number == big

    def test_infsup(self):
        """
        Test inf/sup symbols.
        """
        assert str(Infimum) == "#inf"
        assert str(Supremum) == "#sup"

    def test_string(self):
        """
        Test string symbols.
        """
        val = 'sh2354n\nshoeu"insh'
        assert String(self.lib, val).string == val
        assert str(String(self.lib, val)) == '"sh2354n\\nshoeu\\"insh"'

    def test_tuple(self):
        """
        Test tuple symbols.
        """
        args = [Number(self.lib, 1), String(self.lib, "a")]
        assert Tuple_(self.lib, args).arguments == args

    def test_function(self):
        """
        Test function symbols.
        """
        name = "f"
        args = [Number(self.lib, 1), String(self.lib, "a")]
        for is_positive in [True, False]:
            f = Function(self.lib, name, args, is_positive)
            assert f.name == name
            assert f.arguments == args
            assert f.is_positive == is_positive
            assert f.is_negative != is_positive

    def test_match(self):
        """
        Test match functions.
        """
        a = Number(self.lib, 2)
        b = Function(self.lib, "f", [a], True)
        c = Tuple_(self.lib, [a, b])
        assert not a.match(1)
        assert not a.match("f", 1)
        assert b.match("f", 1, True)
        assert not b.match("f", 1, False)
        assert c.match(2)
        assert not c.match(1)

    def test_parse(self):
        """
        Test parse_term and string conversion.
        """
        val = "f((1,),2,a,-b,(1,2),#inf,#sup)"
        assert str(parse_term(self.lib, val)) == val

    def test_compare(self):
        """
        Test comparison functions.
        """
        a = Number(self.lib, 1)
        b = Number(self.lib, 2)
        assert a == a  # pylint: disable=comparison-with-itself
        assert a != b
        assert a < b
        assert a <= b
        assert b > a
        assert b >= a

    def test_hash(self):
        """
        Test hash functions.
        """
        a = Number(self.lib, 1)
        b = Number(self.lib, 2)
        assert hash(a) != hash(b)
