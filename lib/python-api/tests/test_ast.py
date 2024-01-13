"""
Unit tests for clingo.core module.
"""
from unittest import TestCase

from clingo import ast
from clingo.core import Library


class TestSymbol(TestCase):
    """
    Unit tests for clingo.core module.
    """

    def setUp(self):
        self.lib = Library()
        self.loc = ast.Location(
            ast.Position(self.lib, "<a>", 1, 2), ast.Position(self.lib, "<b>", 3, 4)
        )

    def tearDown(self):
        self.lib = None

    def test_location(self):
        """
        Test location.
        """
        self.assertEqual(self.loc.begin.file, "<a>")
        self.assertEqual(self.loc.begin.line, 1)
        self.assertEqual(self.loc.begin.column, 2)
        self.assertEqual(self.loc.end.file, "<b>")
        self.assertEqual(self.loc.end.line, 3)
        self.assertEqual(self.loc.end.column, 4)

        self.assertEqual(self.loc.begin, self.loc.begin)
        self.assertNotEqual(self.loc.begin, self.loc.end)
        self.assertLess(self.loc.begin, self.loc.end)

    def test_projection(self):
        """
        Test projection..
        """
        p = ast.Projection(self.lib, self.loc)

        # TODO: self.assertEqual(x.location, self.loc)
        self.assertEqual(str(p), "*")

    def test_variable(self):
        """
        Test variable terms.
        """
        x = ast.TermVariable(self.lib, self.loc, "X", False)
        a = ast.TermVariable(self.lib, self.loc, "_", True)

        # TODO: self.assertEqual(x.location, self.loc)
        self.assertEqual(x.name, "X")
        self.assertFalse(x.anonymous)
        self.assertEqual(a.name, "_")
        self.assertTrue(a.anonymous)
        self.assertEqual(str(x), "X")
        self.assertEqual(str(a), "_")
