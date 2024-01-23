"""
Unit tests for clingo.ast module.
"""
from textwrap import dedent
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

    def test_head_simple_literal(self):
        """
        Test simple head literal.
        """
        s = "not p(X)"
        p = ast.HeadSimpleLiteral(self.lib, ast.parse_literal(self.lib, s))

        self.assertEqual(p.literal, ast.parse_literal(self.lib, s))
        self.assertEqual(str(p), "not p(X)")

    def test_head_disjunction(self):
        """
        Test head disjunction literal.
        """
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        l3 = ast.HeadConditionalLiteral(self.lib, self.loc, l2, [l1])

        p = ast.HeadDisjunction(self.lib, self.loc, [l2, l3])

        self.assertEqual(l3.location, self.loc)
        self.assertEqual(l3.literal, l2)
        self.assertEqual(l3.condition, [l1])

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.elements, [l2, l3])
        self.assertEqual(str(p), "r(X); r(X): not p(X)")

    def test_head_set_aggregate(self):
        """
        Test head set aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = ast.SetAggregateElement(self.lib, self.loc, l1, [l2])
        lg1 = ast.LeftGuard(self.lib, t1, ast.Relation.Less)
        rg1 = ast.RightGuard(self.lib, ast.Relation.LessEqual, t1)
        a1 = ast.HeadSetAggregate(self.lib, self.loc, None, [e1], None)
        a2 = ast.HeadSetAggregate(self.lib, self.loc, lg1, [e1], rg1)

        self.assertEqual(a1.location, self.loc)
        self.assertIsNone(a1.left)
        self.assertEqual(a1.elements, [e1])
        self.assertIsNone(a1.right)

        self.assertEqual(a2.location, self.loc)
        self.assertEqual(a2.left, lg1)
        self.assertEqual(a2.elements, [e1])
        self.assertEqual(a2.right, rg1)

        self.assertEqual(str(e1), "not p(X): r(X)")
        self.assertEqual(str(lg1), "5 < ")
        self.assertEqual(str(rg1), " <= 5")
        self.assertEqual(str(a1), "{ not p(X): r(X) }")
        self.assertEqual(str(a2), "5 < { not p(X): r(X) } <= 5")

    def test_head_aggregate(self):
        """
        Test head aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        t2 = ast.parse_term(self.lib, "X")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        l3 = ast.parse_literal(self.lib, "q(X)")
        e1 = ast.HeadAggregateElement(self.lib, self.loc, [t1, t2], l3, [l1, l2])
        lg1 = ast.LeftGuard(self.lib, t1, ast.Relation.Less)
        rg1 = ast.RightGuard(self.lib, ast.Relation.LessEqual, t1)
        a1 = ast.HeadAggregate(
            self.lib,
            self.loc,
            None,
            ast.AggregateFunction.Count,
            [e1],
            None,
        )
        a2 = ast.HeadAggregate(
            self.lib,
            self.loc,
            lg1,
            ast.AggregateFunction.Sum,
            [e1],
            rg1,
        )

        self.assertEqual(e1.location, self.loc)
        self.assertEqual(e1.tuple, [t1, t2])
        self.assertEqual(e1.condition, [l1, l2])

        self.assertEqual(a1.location, self.loc)
        self.assertIsNone(a1.left)
        self.assertEqual(a1.function, ast.AggregateFunction.Count)
        self.assertEqual(a1.elements, [e1])
        self.assertIsNone(a1.right)

        self.assertEqual(a2.location, self.loc)
        self.assertEqual(a2.left, lg1)
        self.assertEqual(a2.function, ast.AggregateFunction.Sum)
        self.assertEqual(a2.elements, [e1])
        self.assertEqual(a2.right, rg1)

        self.assertEqual(str(e1), "5,X: q(X): not p(X), r(X)")
        self.assertEqual(str(a1), "#count { 5,X: q(X): not p(X), r(X) }")
        self.assertEqual(str(a2), "5 < #sum { 5,X: q(X): not p(X), r(X) } <= 5")

    def test_head_theory_atom(self):
        """
        Test head theory atom.
        """
        t1 = ast.parse_term(self.lib, "f(X)")
        tt1 = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "f(1,2)"))
        tt2 = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "5"))
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = ast.TheoryAtomElement(self.lib, self.loc, [tt1, tt2], [l1, l2])
        rg1 = ast.TheoryRightGuard(self.lib, "<>", tt2)
        a1 = ast.HeadTheoryAtom(self.lib, self.loc, t1, [e1], rg1)
        a2 = ast.HeadTheoryAtom(self.lib, self.loc, t1, [e1], None)

        self.assertEqual(rg1.theory_operator, "<>")
        self.assertEqual(rg1.term, tt2)

        self.assertEqual(e1.location, self.loc)
        self.assertEqual(e1.tuple, [tt1, tt2])
        self.assertEqual(e1.condition, [l1, l2])

        self.assertEqual(a1.location, self.loc)
        self.assertEqual(a1.name, t1)
        self.assertEqual(a1.elements, [e1])
        self.assertEqual(a1.right, rg1)

        self.assertIsNone(a2.right)

        self.assertEqual(str(e1), "f(1,2),5: not p(X), r(X)")
        self.assertEqual(str(rg1), " <> 5")
        self.assertEqual(str(a1), "&f(X) { f(1,2),5: not p(X), r(X) } <> 5")

    def test_body_simple_literal(self):
        """
        Test simple body literal.
        """
        s = "not p(X)"
        p = ast.BodySimpleLiteral(self.lib, ast.parse_literal(self.lib, s))

        self.assertEqual(p.literal, ast.parse_literal(self.lib, s))
        self.assertEqual(str(p), "not p(X)")

    def test_body_conditional_literal(self):
        """
        Test body conditional literal.
        """
        s = ast.parse_literal(self.lib, "not p(X)")
        t = ast.parse_literal(self.lib, "r(X)")
        p = ast.BodyConditionalLiteral(self.lib, self.loc, s, [t])

        self.assertEqual(p.location, self.loc)
        self.assertEqual(p.literal, s)
        self.assertEqual(p.condition, [t])
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

        self.assertEqual(e1.location, self.loc)
        self.assertEqual(e1.literal, l1)
        self.assertEqual(e1.condition, [l2])

        self.assertEqual(lg1.term, t1)
        self.assertEqual(lg1.relation, ast.Relation.Less)

        self.assertEqual(rg1.term, t1)
        self.assertEqual(rg1.relation, ast.Relation.LessEqual)

        self.assertEqual(a1.location, self.loc)
        self.assertEqual(a1.sign, ast.Sign.Single)
        self.assertIsNone(a1.left)
        self.assertEqual(a1.elements, [e1])
        self.assertIsNone(a1.right)

        self.assertEqual(a2.location, self.loc)
        self.assertEqual(a2.sign, ast.Sign.NoSign)
        self.assertEqual(a2.left, lg1)
        self.assertEqual(a2.elements, [e1])
        self.assertEqual(a2.right, rg1)

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

        self.assertEqual(e1.location, self.loc)
        self.assertEqual(e1.tuple, [t1, t2])
        self.assertEqual(e1.condition, [l1, l2])

        self.assertEqual(a1.location, self.loc)
        self.assertEqual(a1.sign, ast.Sign.Single)
        self.assertIsNone(a1.left)
        self.assertEqual(a1.function, ast.AggregateFunction.Count)
        self.assertEqual(a1.elements, [e1])
        self.assertIsNone(a1.right)

        self.assertEqual(a2.location, self.loc)
        self.assertEqual(a2.sign, ast.Sign.NoSign)
        self.assertEqual(a2.left, lg1)
        self.assertEqual(a2.function, ast.AggregateFunction.Sum)
        self.assertEqual(a2.elements, [e1])
        self.assertEqual(a2.right, rg1)

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
        a2 = ast.BodyTheoryAtom(self.lib, self.loc, ast.Sign.Single, t1, [e1], None)

        self.assertEqual(rg1.theory_operator, "<>")
        self.assertEqual(rg1.term, tt2)

        self.assertEqual(e1.location, self.loc)
        self.assertEqual(e1.tuple, [tt1, tt2])
        self.assertEqual(e1.condition, [l1, l2])

        self.assertEqual(a1.location, self.loc)
        self.assertEqual(a1.sign, ast.Sign.Single)
        self.assertEqual(a1.name, t1)
        self.assertEqual(a1.elements, [e1])
        self.assertEqual(a1.right, rg1)

        self.assertIsNone(a2.right)

        self.assertEqual(str(e1), "f(1,2),5: not p(X), r(X)")
        self.assertEqual(str(rg1), " <> 5")
        self.assertEqual(str(a1), "not &f(X) { f(1,2),5: not p(X), r(X) } <> 5")

    def test_statement_rule(self):
        """
        Test rule.
        """
        h = ast.HeadSimpleLiteral(self.lib, ast.parse_literal(self.lib, "not q(X)"))
        b = ast.BodySimpleLiteral(self.lib, ast.parse_literal(self.lib, "p(X)"))
        r = ast.StatementRule(self.lib, self.loc, h, [b])

        self.assertEqual(r.head, h)
        self.assertEqual(r.body, [b])
        self.assertEqual(str(r), "not q(X) :- p(X).")

    def test_statement_theory(self):
        """
        Test theory definition.
        """
        od1 = ast.TheoryOperatorDefinition(
            self.lib, self.loc, "+", 3, ast.TheoryOperatorType.BinaryLeft
        )

        self.assertEqual(od1.location, self.loc)
        self.assertEqual(od1.name, "+")
        self.assertEqual(od1.priority, 3)
        self.assertEqual(od1.operator_type, ast.TheoryOperatorType.BinaryLeft)
        self.assertEqual(str(od1), "+ : 3, binary, left")

        td1 = ast.TheoryTermDefinition(self.lib, self.loc, "t", [od1])
        self.assertEqual(td1.location, self.loc)
        self.assertEqual(td1.name, "t")
        self.assertEqual(td1.operators, [od1])
        self.assertEqual(str(td1), "t { + : 3, binary, left }")

        gd1 = ast.TheoryGuardDefinition(self.lib, ["+", "-"], "t")
        self.assertEqual(gd1.operators, ["+", "-"])
        self.assertEqual(gd1.term, "t")
        self.assertEqual(str(gd1), "{+,-}, t")

        ad1 = ast.TheoryAtomDefinition(
            self.lib, self.loc, "p", 1, "t", None, ast.TheoryAtomType.Directive
        )
        ad2 = ast.TheoryAtomDefinition(
            self.lib, self.loc, "p", 1, "t", gd1, ast.TheoryAtomType.Directive
        )
        self.assertEqual(ad1.location, self.loc)
        self.assertEqual(ad1.name, "p")
        self.assertEqual(ad1.arity, 1)
        self.assertEqual(ad1.term, "t")
        self.assertIsNone(ad1.guard)
        self.assertEqual(ad1.atom_type, ast.TheoryAtomType.Directive)
        self.assertEqual(ad2.guard, gd1)
        self.assertEqual(str(ad1), "&p/1: t, directive")
        self.assertEqual(str(ad2), "&p/1: t, {+,-}, t, directive")

        d1 = ast.StatementTheory(self.lib, self.loc, "t", [td1], [ad1, ad2])
        self.assertEqual(d1.location, self.loc)
        self.assertEqual(d1.name, "t")
        self.assertEqual(d1.terms, [td1])
        self.assertEqual(d1.atoms, [ad1, ad2])
        self.assertEqual(
            str(d1),
            dedent(
                """\
                #theory t {
                  t { + : 3, binary, left };
                  &p/1: t, directive;
                  &p/1: t, {+,-}, t, directive
                }."""
            ),
        )

    def test_statement_optimize(self):
        """
        Test optimization statements.
        """
        terms = [ast.parse_term(self.lib, "X"), ast.parse_term(self.lib, "Y")]
        weight = ast.parse_term(self.lib, "5")
        prio = ast.parse_term(self.lib, "2")
        t1 = ast.OptimizeTuple(self.lib, weight, None, terms)
        t2 = ast.OptimizeTuple(self.lib, weight, prio, terms)
        self.assertEqual(t1.weight, weight)
        self.assertIsNone(t1.priority)
        self.assertEqual(t1.terms, terms)
        self.assertEqual(t2.priority, prio)
        self.assertEqual(str(t1), "5,X,Y")
        self.assertEqual(str(t2), "5@2,X,Y")

        l1 = ast.parse_literal(self.lib, "p(X)")
        l2 = ast.parse_literal(self.lib, "q(X)")
        e1 = ast.OptimizeElement(self.lib, t1, [l1, l2])
        e2 = ast.OptimizeElement(self.lib, t2, [l1, l2])
        self.assertEqual(e1.tuple, t1)
        self.assertEqual(e1.condition, [l1, l2])
        self.assertEqual(str(e1), "5,X,Y: p(X), q(X)")
        self.assertEqual(str(e2), "5@2,X,Y: p(X), q(X)")

        so1 = ast.StatementOptimize(
            self.lib, self.loc, [e1, e2], ast.OptimizeType.Minimize
        )
        self.assertEqual(so1.location, self.loc)
        self.assertEqual(so1.elements, [e1, e2])
        self.assertEqual(so1.optimize_type, ast.OptimizeType.Minimize)
        self.assertEqual(
            str(so1), "#minimize { 5,X,Y: p(X), q(X); 5@2,X,Y: p(X), q(X) }."
        )

        body = [
            ast.BodySimpleLiteral(self.lib, l1),
            ast.BodySimpleLiteral(self.lib, l2),
        ]
        sw1 = ast.StatementWeakConstraint(self.lib, self.loc, body, t1)
        self.assertEqual(sw1.body, body)
        self.assertEqual(sw1.tuple, t1)
        self.assertEqual(str(sw1), " :~ p(X); q(X). [5,X,Y]")

    def test_statement_show(self):
        """
        Test show statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementShow(self.lib, self.loc, t1, [l1, l2])
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.term, t1)
        self.assertEqual(s1.body, [l1, l2])
        self.assertEqual(str(s1), "#show -p(X): q(X); p(X).")

        s2 = ast.StatementShowSignature(self.lib, self.loc, "p", 2)
        s3 = ast.StatementShowSignature(self.lib, self.loc, "q", 2, True)
        self.assertEqual(s2.location, self.loc)
        self.assertEqual(s2.name, "p")
        self.assertEqual(s2.arity, 2)
        self.assertFalse(s2.sign)
        self.assertTrue(s3.sign)
        self.assertEqual(str(s2), "#show p/2.")
        self.assertEqual(str(s3), "#show -q/2.")

    def test_statement_project(self):
        """
        Test project statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementProject(self.lib, self.loc, t1, [l1, l2])
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.atom, t1)
        self.assertEqual(s1.body, [l1, l2])
        self.assertEqual(str(s1), "#project -p(X): q(X); p(X).")

        s2 = ast.StatementProjectSignature(self.lib, self.loc, "p", 2)
        s3 = ast.StatementProjectSignature(self.lib, self.loc, "q", 2, True)
        self.assertEqual(s2.location, self.loc)
        self.assertEqual(s2.name, "p")
        self.assertEqual(s2.arity, 2)
        self.assertFalse(s2.sign)
        self.assertTrue(s3.sign)
        self.assertEqual(str(s2), "#project p/2.")
        self.assertEqual(str(s3), "#project -q/2.")

    def test_statement_defined(self):
        """
        Test defined statements.
        """
        s2 = ast.StatementDefined(self.lib, self.loc, "p", 2)
        s3 = ast.StatementDefined(self.lib, self.loc, "q", 2, True)
        self.assertEqual(s2.location, self.loc)
        self.assertEqual(s2.name, "p")
        self.assertEqual(s2.arity, 2)
        self.assertFalse(s2.sign)
        self.assertTrue(s3.sign)
        self.assertEqual(str(s2), "#defined p/2.")
        self.assertEqual(str(s3), "#defined -q/2.")

    def test_statement_external(self):
        """
        Test external statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        t2 = ast.parse_term(self.lib, "true")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementExternal(self.lib, self.loc, t1, [l1, l2])
        s2 = ast.StatementExternal(self.lib, self.loc, t1, [l1, l2], t2)
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.atom, t1)
        self.assertEqual(s1.body, [l1, l2])
        self.assertIsNone(s1.external_type)
        self.assertEqual(s2.external_type, t2)
        self.assertEqual(str(s1), "#external -p(X): q(X); p(X).")
        self.assertEqual(str(s2), "#external -p(X): q(X); p(X). [true]")

    def test_statement_edge(self):
        """
        Test external statements.
        """
        u1 = ast.parse_term(self.lib, "u")
        v1 = ast.parse_term(self.lib, "v")
        u2 = ast.parse_term(self.lib, "x")
        v2 = ast.parse_term(self.lib, "y")
        e1 = ast.Edge(self.lib, u1, v1)
        e2 = ast.Edge(self.lib, u2, v2)
        self.assertEqual(e1.u, u1)
        self.assertEqual(e1.v, v1)
        self.assertEqual(e2.u, u2)
        self.assertEqual(e2.v, v2)
        self.assertEqual(str(e1), "u,v")
        self.assertEqual(str(e2), "x,y")

        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementEdge(self.lib, self.loc, [e1, e2], [l1, l2])
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.pool, [e1, e2])
        self.assertEqual(s1.body, [l1, l2])
        self.assertEqual(str(s1), "#edge (u,v;x,y): q(X); p(X).")

    def test_statement_heuristic(self):
        """
        Test heuristic statements.
        """
        a = ast.parse_term(self.lib, "a")
        w = ast.parse_term(self.lib, "w")
        p = ast.parse_term(self.lib, "p")
        m = ast.parse_term(self.lib, "m")

        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m)
        s2 = ast.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m, p)
        self.assertEqual(s1.atom, a)
        self.assertEqual(s1.weight, w)
        self.assertEqual(s1.modifier, m)
        self.assertIsNone(s1.priority)
        self.assertEqual(s2.priority, p)
        self.assertEqual(str(s1), "#heuristic a: q(X); p(X). [w,m]")
        self.assertEqual(str(s2), "#heuristic a: q(X); p(X). [w@p,m]")

    def test_statement_include(self):
        """
        Test include statements.
        """
        s1 = ast.StatementInclude(self.lib, self.loc, "file", ast.IncludeType.System)
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.value, "file")
        self.assertEqual(s1.include_type, ast.IncludeType.System)
        self.assertEqual(str(s1), '#include "file".')

    def test_statement_program(self):
        """
        Test program statements.
        """
        s1 = ast.StatementProgram(self.lib, self.loc, "step", ["t", "k"])
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.name, "step")
        self.assertEqual(s1.arguments, ["t", "k"])
        self.assertEqual(str(s1), "#program step(t,k).")

    def test_statement_script(self):
        """
        Test script statements.
        """
        s1 = ast.StatementScript(self.lib, self.loc, "def p(x): return x", "python")
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.value, "def p(x): return x")
        self.assertEqual(s1.script_type, "python")
        self.assertEqual(str(s1), "#script (python)def p(x): return x#end.")

    def test_statement_const(self):
        """
        Test const statements.
        """
        t1 = ast.parse_term(self.lib, "f(2+3)")
        s1 = ast.StatementConst(self.lib, self.loc, "x", t1, ast.ConstType.Override)
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.name, "x")
        self.assertEqual(s1.value, t1)
        self.assertEqual(s1.const_type, ast.ConstType.Override)
        self.assertEqual(str(s1), "#const x=f(2+3). [override]")

    def test_statement_comment(self):
        """
        Test const statements.
        """
        s1 = ast.StatementComment(
            self.lib, self.loc, "% something arbitrary", ast.CommentType.Line
        )
        self.assertEqual(s1.location, self.loc)
        self.assertEqual(s1.value, "% something arbitrary")
        self.assertEqual(s1.comment_type, ast.CommentType.Line)
        self.assertEqual(str(s1), "% something arbitrary")

    def test_parse(self):
        """
        Test parsing of asts.
        """
        term = "-f(X+Y,3)"
        self.assertEqual(str(ast.parse_term(self.lib, term)), term)
        theory_term = "(f ** X)"
        self.assertEqual(str(ast.parse_theory_term(self.lib, theory_term)), theory_term)
        lit = "not not p(X+2)"
        self.assertEqual(str(ast.parse_literal(self.lib, lit)), lit)
        head_lit = "a; b: c"
        self.assertEqual(str(ast.parse_head_literal(self.lib, head_lit)), head_lit)
        body_lit = "b: c"
        self.assertEqual(str(ast.parse_body_literal(self.lib, body_lit)), body_lit)
        stm = "a; b: c :- d: e."
        self.assertEqual(str(stm), stm)

    def test_scan(self):
        """
        Test the statement scanner.
        """
        with ast.Scanner(self.lib, "a. b. c.") as scanner:
            self.assertEqual([str(stm) for stm in scanner], ["a.", "b.", "c."])

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
