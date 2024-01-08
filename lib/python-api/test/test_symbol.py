"""
Unit tests for clingo.core module.
"""
from unittest import TestCase

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


class TestSymbol(TestCase):
    """
    Unit tests for clingo.core module.
    """

    def setUp(self):
        self.lib = Library()

    def tearDown(self):
        self.lib = None

    def test_number(self):
        """
        Test number symbols.
        """
        small = 13
        big = 10000000000000000000000003
        self.assertEqual(Number(self.lib, small).number, small)
        self.assertEqual(Number(self.lib, big).number, big)

    def test_infsup(self):
        """
        Test inf/sup symbols.
        """
        self.assertEqual(str(Infimum), "#inf")
        self.assertEqual(str(Supremum), "#sup")

    def test_string(self):
        """
        Test string symbols.
        """
        val = "sh2354nshoeuinsh"
        self.assertEqual(String(self.lib, val).string, val)

    def test_tuple(self):
        """
        Test tuple symbols.
        """
        args = [Number(self.lib, 1), String(self.lib, "a")]
        self.assertEqual(Tuple_(self.lib, args).arguments, args)

    def test_function(self):
        """
        Test function symbols.
        """
        name = "f"
        args = [Number(self.lib, 1), String(self.lib, "a")]
        sign = True
        f = Function(self.lib, name, args, sign)
        self.assertEqual(f.name, name)
        self.assertEqual(f.arguments, args)
        self.assertEqual(f.sign, sign)

    def test_match(self):
        """
        Test match functions.
        """
        a = Number(self.lib, 2)
        b = Function(self.lib, "f", [a], True)
        c = Tuple_(self.lib, [a, b])
        self.assertFalse(a.match(1))
        self.assertFalse(a.match("f", 1))
        self.assertTrue(b.match("f", 1, True))
        self.assertFalse(b.match("f", 1, False))
        self.assertTrue(c.match(2))
        self.assertFalse(c.match(1))

    def test_parse(self):
        """
        Test parse_term and string conversion.
        """
        val = "f((1,),2,a,-b,(1,2),#inf,#sup)"
        self.assertEqual(str(parse_term(self.lib, val)), val)

    def test_compare(self):
        """
        Test comparison functions.
        """
        a = Number(self.lib, 1)
        b = Number(self.lib, 2)
        self.assertEqual(a, a)
        self.assertNotEqual(a, b)
        self.assertLess(a, b)
        self.assertLessEqual(a, b)
        self.assertGreater(b, a)
        self.assertGreaterEqual(b, a)

    def test_hash(self):
        """
        Test hash functions.
        """
        a = Number(self.lib, 1)
        b = Number(self.lib, 2)
        self.assertNotEqual(hash(a), hash(b))
