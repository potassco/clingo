"""
Unit tests for clingo.core module.
"""
from unittest import TestCase

from clingo import ast
from clingo.core import Library, Location, Position
from clingo.symbol import parse_term


class TestSymbol(TestCase):
    """
    Unit tests for clingo.core module.
    """

    def setUp(self):
        self.lib = Library()
        self.loc = Location(
            Position(self.lib, "<a>", 1, 2), Position(self.lib, "<b>", 3, 4)
        )

    def tearDown(self):
        self.lib = None

    def sym(self, val):
        """
        Generate a symbol for testing.
        """
        return ast.TermSymbolic(self.lib, self.loc, parse_term(self.lib, val))

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

        self.assertEqual(hash(self.loc), hash(self.loc))
        self.assertNotEqual(
            hash(self.loc), hash(Location(self.loc.begin, self.loc.begin))
        )
        self.assertNotEqual(hash(self.loc.begin), hash(self.loc.end))

        self.assertEqual(str(self.loc.begin), "<a>:1:2")
        self.assertEqual(str(self.loc.end), "<b>:3:4")
        self.assertEqual(str(self.loc), "<a>:1:2-<b>:3:4")

        self.assertEqual(repr(self.loc.begin), "Position('<a>',1,2)")
        self.assertEqual(
            repr(self.loc), "Location(Position('<a>',1,2),Position('<b>',3,4))"
        )

    def test_projection(self):
        """
        Test projection.
        """
        p = ast.Projection(self.lib, self.loc)

        self.assertEqual(p.location, self.loc)
        self.assertEqual(str(p), "*")

    def test_variable(self):
        """
        Test variable terms.
        """
        x = ast.TermVariable(self.lib, self.loc, "X", False)
        a = ast.TermVariable(self.lib, self.loc, "_", True)

        self.assertEqual(x.location, self.loc)
        self.assertEqual(x.name, "X")
        self.assertFalse(x.anonymous)
        self.assertEqual(a.name, "_")
        self.assertTrue(a.anonymous)
        self.assertEqual(str(x), "X")
        self.assertEqual(str(a), "_")

    def test_symbol(self):
        """
        Test symbolic term.
        """
        s = parse_term(self.lib, "f(1,2)")
        p = ast.TermSymbolic(self.lib, self.loc, s)

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.symbol, s)
        self.assertEqual(str(p), "f(1,2)")

    def test_absolute(self):
        """
        Test absolute term.
        """
        p = ast.TermAbsolute(self.lib, self.loc, [self.sym("1"), self.sym("-2")])

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.pool, [self.sym("1"), self.sym("-2")])
        self.assertEqual(str(p), "|1;-2|")

    def test_unary(self):
        """
        Test unary term.
        """
        p = ast.TermUnaryOperation(
            self.lib, self.loc, ast.UnaryOperator.Minus, self.sym("-2")
        )

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.operator_type, ast.UnaryOperator.Minus)
        self.assertEqual(p.right, self.sym("-2"))
        self.assertEqual(str(p), "-(-2)")

    def test_binary(self):
        """
        Test binary term.
        """
        p = ast.TermBinaryOperation(
            self.lib, self.loc, self.sym("1"), ast.BinaryOperator.Plus, self.sym("-2")
        )

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.left, self.sym("1"))
        self.assertEqual(p.operator_type, ast.BinaryOperator.Plus)
        self.assertEqual(p.right, self.sym("-2"))
        self.assertEqual(str(p), "1+(-2)")

    def test_tuple(self):
        """
        Test tuple term.
        """
        a = [
            ast.ArgumentTuple(self.lib, [self.sym("1"), self.sym("2")]),
            self.sym("3"),
        ]
        p = ast.TermTuple(
            self.lib,
            self.loc,
            a,
        )

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.pool, a)
        self.assertEqual(str(p), "(1,2;3)")

    def test_function(self):
        """
        Test function term.
        """
        a = [
            ast.ArgumentTuple(self.lib, [self.sym("1"), self.sym("2")]),
            ast.ArgumentTuple(
                self.lib, [self.sym("3"), ast.Projection(self.lib, self.loc)]
            ),
        ]
        p = ast.TermFunction(self.lib, self.loc, "f", a, False)

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.name, "f")
        self.assertEqual(p.pool, a)
        self.assertFalse(p.external)
        self.assertEqual(str(p), "f(1,2;3,*)")

    def test_cmp(self):
        """
        Test comparision functions.
        """
        x = ast.TermVariable(self.lib, self.loc, "X", False)
        a = ast.TermVariable(self.lib, self.loc, "_", True)

        self.assertEqual(x, x)
        self.assertNotEqual(a, x)
        self.assertLess(x, a)
        self.assertLessEqual(x, a)
        self.assertGreater(a, x)
        self.assertGreaterEqual(a, x)

        self.assertEqual(hash(x), hash(x))
        self.assertNotEqual(hash(x), hash(a))
