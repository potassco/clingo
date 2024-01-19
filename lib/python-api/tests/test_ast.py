"""
Unit tests for clingo.ast module.
"""
from unittest import TestCase

from clingo import ast
from clingo.core import Library, Location, Position
from clingo.symbol import parse_term


class TestAST(TestCase):
    """
    Unit tests for clingo.ast module.
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
        p = ast.TermFunction(self.lib, self.loc, "f", a, True)
        q = ast.TermFunction(self.lib, self.loc, "f", [ast.ArgumentTuple(self.lib)])

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.name, "f")
        self.assertEqual(p.pool, a)
        self.assertTrue(p.external)
        self.assertFalse(q.external)
        self.assertEqual(str(p), "@f(1,2;3,*)")

    def test_theory_variable(self):
        """
        Test theory variable terms.
        """
        x = ast.TheoryTermVariable(self.lib, self.loc, "X", False)
        a = ast.TheoryTermVariable(self.lib, self.loc, "_", True)

        self.assertEqual(x.location, self.loc)
        self.assertEqual(x.name, "X")
        self.assertFalse(x.anonymous)
        self.assertEqual(a.name, "_")
        self.assertTrue(a.anonymous)
        self.assertEqual(str(x), "X")
        self.assertEqual(str(a), "_")

    def test_theory_symbol(self):
        """
        Test theory_symbolic term.
        """
        s = parse_term(self.lib, "f(1,2)")
        p = ast.TheoryTermSymbolic(self.lib, self.loc, s)

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.symbol, s)
        self.assertEqual(str(p), "f(1,2)")

    def test_theory_tuple(self):
        """
        Test theory tuple term.
        """
        p = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "p(1,2)"))
        q = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))
        a = [p, q]
        b = ast.TheoryTermTuple(self.lib, self.loc, ast.TheoryTupleType.Set, a)

        self.assertEqual(b.location, self.loc)
        self.assertEqual(b.arguments, a)
        self.assertEqual(b.tuple_type, ast.TheoryTupleType.Set)
        self.assertEqual(str(b), "{p(1,2),q}")

    def test_theory_function(self):
        """
        Test theory function term.
        """
        p = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "p(1,2)"))
        q = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))
        a = [p, q]
        b = ast.TheoryTermFunction(self.lib, self.loc, "a", a)
        c = ast.TheoryTermFunction(self.lib, self.loc, "++", a)
        d = ast.TheoryTermFunction(self.lib, self.loc, "not", [q])

        self.assertEqual(b.location, self.loc)
        self.assertEqual(c.location, self.loc)
        self.assertEqual(b.name, "a")
        self.assertEqual(c.name, "++")
        self.assertEqual(b.arguments, a)
        self.assertEqual(c.arguments, a)
        self.assertEqual(str(b), "a(p(1,2),q)")
        self.assertEqual(str(c), "(p(1,2) ++ q)")
        self.assertEqual(str(d), "(not q)")

    def test_theory_unparsed(self):
        """
        Test theory function term.
        """
        p = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "p(1,2)"))
        q = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))

        a = ast.UnparsedElement(self.lib, ["+", "-"], p)
        b = ast.UnparsedElement(self.lib, ["*"], q)

        x = ast.TheoryTermUnparsed(self.lib, self.loc, [a, b])

        self.assertEqual(x.location, self.loc)
        self.assertEqual(p.location, self.loc)
        self.assertEqual(q.location, self.loc)
        self.assertEqual(a.operators, ["+", "-"])
        self.assertEqual(b.operators, ["*"])
        self.assertEqual(a.term, p)
        self.assertEqual(b.term, q)
        self.assertEqual(x.elements, [a, b])
        self.assertEqual(str(a), "+ - p(1,2)")
        self.assertEqual(str(b), "* q")
        self.assertEqual(str(x), "(+ - p(1,2) * q)")

    def test_boolean(self):
        """
        Test Boolean literal.
        """
        p = ast.LiteralBoolean(self.lib, self.loc, ast.Sign.Single, True)

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.sign, ast.Sign.Single)
        self.assertTrue(p.value)
        self.assertEqual(str(p), "not #true")

    def test_symbolic_literal(self):
        """
        Test symbolic literal.
        """
        a = ast.parse_term(self.lib, "-f(X)")
        p = ast.LiteralSymbolic(self.lib, self.loc, ast.Sign.Single, a)

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.sign, ast.Sign.Single)
        self.assertEqual(p.atom, a)
        self.assertEqual(str(p), "not -f(X)")

    def test_comparison_literal(self):
        """
        Test comparison literal.
        """
        a = ast.parse_term(self.lib, "X")
        b = ast.RightGuard(self.lib, ast.Relation.Less, ast.parse_term(self.lib, "Y"))
        c = ast.RightGuard(
            self.lib, ast.Relation.LessEqual, ast.parse_term(self.lib, "Z")
        )
        p = ast.LiteralComparison(self.lib, self.loc, ast.Sign.Single, a, [b, c])

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.sign, ast.Sign.Single)
        self.assertEqual(p.left, a)
        self.assertEqual(p.right, [b, c])
        self.assertEqual(str(p), "not X<Y<=Z")

    def test_body_simple_literal(self):
        """
        Test simple body literal.
        """
        s = "not p(X)"
        p = ast.BodySimpleLiteral(self.lib, ast.parse_literal(self.lib, s))

        # TODO: attributes
        self.assertEqual(str(p), "not p(X)")

    def test_body_conditional_literal(self):
        """
        Test body conditional literal.
        """
        s = ast.parse_literal(self.lib, "not p(X)")
        t = ast.parse_literal(self.lib, "r(X)")
        p = ast.BodyConditionalLiteral(self.lib, self.loc, s, [t])

        # TODO: attributes
        self.assertEqual(str(p), "not p(X): r(X)")

    def test_body_set_aggregate(self):
        """
        Test body set aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = ast.SetAggregateElement(self.lib, self.loc, l1, [l2])
        lg1 = ast.LeftGuard(self.lib, t1, ast.Relation.Less)
        rg1 = ast.RightGuard(self.lib, ast.Relation.LessEqual, t1)
        a1 = ast.BodySetAggregate(self.lib, self.loc, ast.Sign.Single, None, [e1], None)
        a2 = ast.BodySetAggregate(self.lib, self.loc, ast.Sign.NoSign, lg1, [e1], rg1)

        # TODO: attributes
        self.assertEqual(str(e1), "not p(X): r(X)")
        self.assertEqual(str(lg1), "5 < ")
        self.assertEqual(str(rg1), " <= 5")
        self.assertEqual(str(a1), "not { not p(X): r(X) }")
        self.assertEqual(str(a2), "5 < { not p(X): r(X) } <= 5")

    def test_body_aggregate(self):
        """
        Test body aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        t2 = ast.parse_term(self.lib, "X")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = ast.BodyAggregateElement(self.lib, self.loc, [t1, t2], [l1, l2])
        lg1 = ast.LeftGuard(self.lib, t1, ast.Relation.Less)
        rg1 = ast.RightGuard(self.lib, ast.Relation.LessEqual, t1)
        a1 = ast.BodyAggregate(
            self.lib,
            self.loc,
            ast.Sign.Single,
            None,
            ast.AggregateFunction.Count,
            [e1],
            None,
        )
        a2 = ast.BodyAggregate(
            self.lib,
            self.loc,
            ast.Sign.NoSign,
            lg1,
            ast.AggregateFunction.Sum,
            [e1],
            rg1,
        )

        # TODO: attributes
        self.assertEqual(str(e1), "5,X: not p(X), r(X)")
        self.assertEqual(str(a1), "not #count { 5,X: not p(X), r(X) }")
        self.assertEqual(str(a2), "5 < #sum { 5,X: not p(X), r(X) } <= 5")

    def test_body_theory_atom(self):
        """
        Test body theory atom.
        """
        t1 = ast.parse_term(self.lib, "f(X)")
        tt1 = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "f(1,2)"))
        tt2 = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "5"))
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = ast.TheoryAtomElement(self.lib, self.loc, [tt1, tt2], [l1, l2])
        rg1 = ast.TheoryRightGuard(self.lib, "<>", tt2)
        a1 = ast.BodyTheoryAtom(self.lib, self.loc, ast.Sign.Single, t1, [e1], rg1)

        # TODO: attributes
        self.assertEqual(str(e1), "f(1,2),5: not p(X), r(X)")
        self.assertEqual(str(rg1), " <> 5")
        self.assertEqual(str(a1), "not &f(X) { f(1,2),5: not p(X), r(X) } <> 5")

    def test_parse(self):
        """
        Test parsing of asts.
        """
        term = "-f(X+Y,3)"
        self.assertEqual(str(ast.parse_term(self.lib, term)), term)
        lit = "not not p(X+2)"
        self.assertEqual(str(ast.parse_literal(self.lib, lit)), lit)

    def test_cmp(self):
        """
        Test comparision functions.
        """
        x = ast.TermVariable(self.lib, self.loc, "X")
        a = ast.TermVariable(self.lib, self.loc, "_", True)

        self.assertEqual(x, x)
        self.assertNotEqual(a, x)
        self.assertLess(x, a)
        self.assertLessEqual(x, a)
        self.assertGreater(a, x)
        self.assertGreaterEqual(a, x)

        self.assertEqual(hash(x), hash(x))
        self.assertNotEqual(hash(x), hash(a))
