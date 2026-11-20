"""
Unit tests for clingo.ast module.
"""

from functools import singledispatch
from textwrap import dedent

import pytest
from clingo import ast
from clingo.ast import aggregates, body, head, literals, statements, terms, theory
from clingo.core import Library, Location, Position
from clingo.symbol import parse_term


def noop_logger(code, msg):
    """
    Noop logger that ignores all messages.
    """
    assert code is not None
    assert msg is not None


class TestAST:
    # pylint: disable=attribute-defined-outside-init
    """
    Unit tests for clingo.ast module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library(logger=noop_logger)
        self._loc = Location(
            Position(self._lib, "<a>", 1, 2), Position(self._lib, "<b>", 3, 4)
        )

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._loc = None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    @property
    def loc(self) -> Location:
        """
        Get a dummy location.
        """
        assert self._loc is not None
        return self._loc

    def sym(self, val):
        """
        Generate a symbol for testing.
        """
        return terms.TermSymbolic(self.lib, self.loc, parse_term(self.lib, val))

    def test_location(self):
        """
        Test location.
        """
        assert self.loc.begin.file == "<a>"
        assert self.loc.begin.line == 1
        assert self.loc.begin.column == 2
        assert self.loc.end.file == "<b>"
        assert self.loc.end.line == 3
        assert self.loc.end.column == 4

        assert self.loc.begin == self.loc.begin
        assert self.loc.begin != self.loc.end
        assert self.loc.begin < self.loc.end

        assert hash(self.loc) == hash(self.loc)
        assert hash(self.loc) != hash(Location(self.loc.begin, self.loc.begin))
        assert hash(self.loc.begin) != hash(self.loc.end)

        assert str(self.loc.begin) == "<a>:1:2"
        assert str(self.loc.end) == "<b>:3:4"
        assert str(self.loc) == "<a>:1:2-<b>:3:4"

        assert repr(self.loc.begin) == "Position('<a>',1,2)"
        assert repr(self.loc) == "Location(Position('<a>',1,2),Position('<b>',3,4))"

    def test_projection(self):
        """
        Test projection.
        """
        p = terms.Projection(self.lib, self.loc)

        assert p.location == self.loc
        assert str(p) == "*"

    def test_variable(self):
        """
        Test variable terms.
        """
        x = terms.TermVariable(self.lib, self.loc, "X", False)
        a = terms.TermVariable(self.lib, self.loc, "_", True)

        assert x.location == self.loc
        assert x.name == "X"
        assert not x.anonymous
        assert a.name == "_"
        assert a.anonymous
        assert str(x) == "X"
        assert str(a) == "_"

    def test_symbol(self):
        """
        Test symbolic term.
        """
        s = parse_term(self.lib, "f(1,2)")
        p = terms.TermSymbolic(self.lib, self.loc, s)

        assert p.location == self.loc
        assert p.symbol == s
        assert str(p) == "f(1,2)"

    def test_fstring(self):
        """
        Test format string terms.
        """
        t = ast.parse_term(self.lib, 'f"a{X:x}b"')
        assert isinstance(t, terms.TermFormatString)
        assert str(t) == 'f"a{X:x}b"'

        elems = t.elements
        assert len(elems) == 3
        assert isinstance(elems[0], terms.FormatFieldLiteral)
        assert str(elems[0]) == "a"
        assert isinstance(elems[1], terms.FormatFieldExpression)
        assert str(elems[1]) == "{X:x}"
        assert isinstance(elems[2], terms.FormatFieldLiteral)
        assert str(elems[2]) == "b"

        expr = elems[1]
        assert str(expr.left) == "X"
        assert expr.right == ":x"

        assert str(terms.TermFormatString(self.lib, self.loc, elems)) == 'f"a{X:x}b"'
        assert str(terms.FormatFieldLiteral(self.lib, self.loc, "a")) == "a"
        assert (
            str(
                terms.FormatFieldExpression(
                    self.lib, self.loc, ast.parse_term(self.lib, "X"), ":x"
                )
            )
            == "{X:x}"
        )

    def test_absolute(self):
        """
        Test absolute term.
        """
        p = terms.TermAbsolute(self.lib, self.loc, [self.sym("1"), self.sym("-2")])

        assert p.location == self.loc
        assert p.pool, [self.sym("1") == self.sym("-2")]
        assert str(p) == "|1;-2|"

    def test_unary(self):
        """
        Test unary term.
        """
        p = terms.TermUnaryOperation(
            self.lib, self.loc, terms.UnaryOperator.Minus, self.sym("-2")
        )

        assert p.location == self.loc
        assert p.operator_type == terms.UnaryOperator.Minus
        assert p.right == self.sym("-2")
        assert str(p) == "-(-2)"

    def test_binary(self):
        """
        Test binary term.
        """
        p = terms.TermBinaryOperation(
            self.lib, self.loc, self.sym("1"), terms.BinaryOperator.Plus, self.sym("-2")
        )

        assert p.location == self.loc
        assert p.left == self.sym("1")
        assert p.operator_type == terms.BinaryOperator.Plus
        assert p.right == self.sym("-2")
        assert str(p) == "1+(-2)"

    def test_tuple(self):
        """
        Test tuple term.
        """
        a = [
            terms.ArgumentTuple(self.lib, [self.sym("1"), self.sym("2")]),
            self.sym("3"),
        ]
        p = terms.TermTuple(
            self.lib,
            self.loc,
            a,
        )

        assert p.location == self.loc
        assert p.pool == a
        assert str(p) == "(1,2;3)"

    def test_function(self):
        """
        Test function term.
        """
        a = [
            terms.ArgumentTuple(self.lib, [self.sym("1"), self.sym("2")]),
            terms.ArgumentTuple(
                self.lib, [self.sym("3"), terms.Projection(self.lib, self.loc)]
            ),
        ]
        p = terms.TermFunction(self.lib, self.loc, "f", a, True)
        q = terms.TermFunction(self.lib, self.loc, "f", [terms.ArgumentTuple(self.lib)])

        assert p.location == self.loc
        assert p.name == "f"
        assert p.pool == a
        assert p.external
        assert not q.external
        assert str(p) == "@f(1,2;3,*)"

    def test_theory_variable(self):
        """
        Test theory variable terms.
        """
        x = theory.TheoryTermVariable(self.lib, self.loc, "X", False)
        a = theory.TheoryTermVariable(self.lib, self.loc, "_", True)

        assert x.location == self.loc
        assert x.name == "X"
        assert not x.anonymous
        assert a.name == "_"
        assert a.anonymous
        assert str(x) == "X"
        assert str(a) == "_"

    def test_theory_symbol(self):
        """
        Test theory_symbolic term.
        """
        s = parse_term(self.lib, "f(1,2)")
        p = theory.TheoryTermSymbolic(self.lib, self.loc, s)

        assert p.location == self.loc
        assert p.symbol == s
        assert str(p) == "f(1,2)"

    def test_theory_tuple(self):
        """
        Test theory tuple term.
        """
        p = theory.TheoryTermSymbolic(
            self.lib, self.loc, parse_term(self.lib, "p(1,2)")
        )
        q = theory.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))
        a = [p, q]
        b = theory.TheoryTermTuple(self.lib, self.loc, theory.TheoryTupleType.Set, a)

        assert b.location == self.loc
        assert b.arguments == a
        assert b.tuple_type == theory.TheoryTupleType.Set
        assert str(b) == "{p(1,2),q}"

    def test_theory_function(self):
        """
        Test theory function term.
        """
        p = theory.TheoryTermSymbolic(
            self.lib, self.loc, parse_term(self.lib, "p(1,2)")
        )
        q = theory.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))
        a = [p, q]
        b = theory.TheoryTermFunction(self.lib, self.loc, "a", a)
        c = theory.TheoryTermFunction(self.lib, self.loc, "++", a)
        d = theory.TheoryTermFunction(self.lib, self.loc, "not", [q])

        assert b.location == self.loc
        assert c.location == self.loc
        assert b.name == "a"
        assert c.name == "++"
        assert b.arguments == a
        assert c.arguments == a
        assert str(b) == "a(p(1,2),q)"
        assert str(c) == "(p(1,2) ++ q)"
        assert str(d) == "(not q)"

    def test_theory_unparsed(self):
        """
        Test theory function term.
        """
        p = theory.TheoryTermSymbolic(
            self.lib, self.loc, parse_term(self.lib, "p(1,2)")
        )
        q = theory.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))

        a = theory.UnparsedElement(self.lib, ["+", "-"], p)
        b = theory.UnparsedElement(self.lib, ["*"], q)

        x = theory.TheoryTermUnparsed(self.lib, self.loc, [a, b])

        assert x.location == self.loc
        assert p.location == self.loc
        assert q.location == self.loc
        assert a.operators == ["+", "-"]
        assert b.operators == ["*"]
        assert a.term == p
        assert b.term == q
        assert x.elements == [a, b]
        assert str(a) == "+ - p(1,2)"
        assert str(b) == "* q"
        assert str(x) == "(+ - p(1,2) * q)"

    def test_boolean(self):
        """
        Test Boolean literal.
        """
        p = literals.LiteralBoolean(self.lib, self.loc, literals.Sign.Single, True)

        assert p.location == self.loc
        assert p.sign == literals.Sign.Single
        assert p.value
        assert str(p) == "not #true"

    def test_symbolic_literal(self):
        """
        Test symbolic literal.
        """
        a = ast.parse_term(self.lib, "-f(X)")
        p = literals.LiteralSymbolic(self.lib, self.loc, literals.Sign.Single, a)

        assert p.location == self.loc
        assert p.sign == literals.Sign.Single
        assert p.atom == a
        assert str(p) == "not -f(X)"

    def test_comparison_literal(self):
        """
        Test comparison literal.
        """
        a = ast.parse_term(self.lib, "X")
        b = literals.RightGuard(
            self.lib, literals.Relation.Less, ast.parse_term(self.lib, "Y")
        )
        c = literals.RightGuard(
            self.lib, literals.Relation.LessEqual, ast.parse_term(self.lib, "Z")
        )
        p = literals.LiteralComparison(
            self.lib, self.loc, literals.Sign.Single, a, [b, c]
        )

        assert p.location == self.loc
        assert p.sign == literals.Sign.Single
        assert p.left == a
        assert p.right == [b, c]
        assert str(p) == "not X<Y<=Z"

    def test_head_simple_literal(self):
        """
        Test simple head literal.
        """
        s = "not p(X)"
        p = head.HeadSimpleLiteral(self.lib, ast.parse_literal(self.lib, s))
        q = ast.parse_literal(self.lib, s)

        assert p.literal == q
        assert str(p) == s

    def test_head_disjunction(self):
        """
        Test head disjunction literal.
        """
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        l3 = head.HeadConditionalLiteral(self.lib, self.loc, l2, [l1])

        p = head.HeadDisjunction(self.lib, self.loc, [l2, l3])

        assert l3.location == self.loc
        assert l3.literal == l2
        assert l3.condition == [l1]

        assert p.location == self.loc
        assert p.elements == [l2, l3]
        assert str(p) == "r(X); r(X): not p(X)"

    def test_head_set_aggregate(self):
        """
        Test head set aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = aggregates.SetAggregateElement(self.lib, self.loc, l1, [l2])
        lg1 = literals.LeftGuard(self.lib, t1, literals.Relation.Less)
        rg1 = literals.RightGuard(self.lib, literals.Relation.LessEqual, t1)
        a1 = head.HeadSetAggregate(self.lib, self.loc, None, [e1], None)
        a2 = head.HeadSetAggregate(self.lib, self.loc, lg1, [e1], rg1)

        assert a1.location == self.loc
        assert a1.left is None
        assert a1.elements == [e1]
        assert a1.right is None

        assert a2.location == self.loc
        assert a2.left == lg1
        assert a2.elements == [e1]
        assert a2.right == rg1

        assert str(e1) == "not p(X): r(X)"
        assert str(lg1) == "5 < "
        assert str(rg1) == " <= 5"
        assert str(a1) == "{ not p(X): r(X) }"
        assert str(a2) == "5 < { not p(X): r(X) } <= 5"

    def test_head_aggregate(self):
        """
        Test head aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        t2 = ast.parse_term(self.lib, "X")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        l3 = ast.parse_literal(self.lib, "q(X)")
        e1 = head.HeadAggregateElement(self.lib, self.loc, [t1, t2], l3, [l1, l2])
        lg1 = literals.LeftGuard(self.lib, t1, literals.Relation.Less)
        rg1 = literals.RightGuard(self.lib, literals.Relation.LessEqual, t1)
        a1 = head.HeadAggregate(
            self.lib,
            self.loc,
            None,
            aggregates.AggregateFunction.Count,
            [e1],
            None,
        )
        a2 = head.HeadAggregate(
            self.lib,
            self.loc,
            lg1,
            aggregates.AggregateFunction.Sum,
            [e1],
            rg1,
        )

        assert e1.location == self.loc
        assert e1.tuple == [t1, t2]
        assert e1.condition == [l1, l2]

        assert a1.location == self.loc
        assert a1.left is None
        assert a1.function == aggregates.AggregateFunction.Count
        assert a1.elements == [e1]
        assert a1.right is None

        assert a2.location == self.loc
        assert a2.left == lg1
        assert a2.function == aggregates.AggregateFunction.Sum
        assert a2.elements == [e1]
        assert a2.right == rg1

        assert str(e1) == "5,X: q(X): not p(X), r(X)"
        assert str(a1) == "#count { 5,X: q(X): not p(X), r(X) }"
        assert str(a2) == "5 < #sum { 5,X: q(X): not p(X), r(X) } <= 5"

    def test_head_theory_atom(self):
        """
        Test head theory atom.
        """
        t1 = ast.parse_term(self.lib, "f(X)")
        tt1 = theory.TheoryTermSymbolic(
            self.lib, self.loc, parse_term(self.lib, "f(1,2)")
        )
        tt2 = theory.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "5"))
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = theory.TheoryAtomElement(self.lib, self.loc, [tt1, tt2], [l1, l2])
        rg1 = theory.TheoryRightGuard(self.lib, "<>", tt2)
        a1 = head.HeadTheoryAtom(self.lib, self.loc, t1, [e1], rg1)
        a2 = head.HeadTheoryAtom(self.lib, self.loc, t1, [e1], None)

        assert rg1.theory_operator == "<>"
        assert rg1.term == tt2

        assert e1.location == self.loc
        assert e1.tuple == [tt1, tt2]
        assert e1.condition == [l1, l2]

        assert a1.location == self.loc
        assert a1.name == t1
        assert a1.elements == [e1]
        assert a1.right == rg1

        assert a2.right is None

        assert str(e1) == "f(1,2),5: not p(X), r(X)"
        assert str(rg1) == " <> 5"
        assert str(a1) == "&f(X) { f(1,2),5: not p(X), r(X) } <> 5"

    def test_body_simple_literal(self):
        """
        Test simple body literal.
        """
        s = "not p(X)"
        p = body.BodySimpleLiteral(self.lib, ast.parse_literal(self.lib, s))
        q = ast.parse_literal(self.lib, s)

        assert p.literal == q
        assert str(p) == s

    def test_body_conditional_literal(self):
        """
        Test body conditional literal.
        """
        s = ast.parse_literal(self.lib, "not p(X)")
        t = ast.parse_literal(self.lib, "r(X)")
        p = body.BodyConditionalLiteral(self.lib, self.loc, s, [t])

        assert p.location == self.loc
        assert p.literal == s
        assert p.condition == [t]
        assert str(p) == "not p(X): r(X)"

    def test_body_set_aggregate(self):
        """
        Test body set aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = aggregates.SetAggregateElement(self.lib, self.loc, l1, [l2])
        lg1 = literals.LeftGuard(self.lib, t1, literals.Relation.Less)
        rg1 = literals.RightGuard(self.lib, literals.Relation.LessEqual, t1)
        a1 = body.BodySetAggregate(
            self.lib, self.loc, literals.Sign.Single, None, [e1], None
        )
        a2 = body.BodySetAggregate(
            self.lib, self.loc, literals.Sign.NoSign, lg1, [e1], rg1
        )

        assert e1.location == self.loc
        assert e1.literal == l1
        assert e1.condition == [l2]

        assert lg1.term == t1
        assert lg1.relation == literals.Relation.Less

        assert rg1.term == t1
        assert rg1.relation == literals.Relation.LessEqual

        assert a1.location == self.loc
        assert a1.sign == literals.Sign.Single
        assert a1.left is None
        assert a1.elements == [e1]
        assert a1.right is None

        assert a2.location == self.loc
        assert a2.sign == literals.Sign.NoSign
        assert a2.left == lg1
        assert a2.elements == [e1]
        assert a2.right == rg1

        assert str(e1) == "not p(X): r(X)"
        assert str(lg1) == "5 < "
        assert str(rg1) == " <= 5"
        assert str(a1) == "not { not p(X): r(X) }"
        assert str(a2) == "5 < { not p(X): r(X) } <= 5"

    def test_body_aggregate(self):
        """
        Test body aggregate.
        """
        t1 = ast.parse_term(self.lib, "5")
        t2 = ast.parse_term(self.lib, "X")
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = body.BodyAggregateElement(self.lib, self.loc, [t1, t2], [l1, l2])
        lg1 = literals.LeftGuard(self.lib, t1, literals.Relation.Less)
        rg1 = literals.RightGuard(self.lib, literals.Relation.LessEqual, t1)
        a1 = body.BodyAggregate(
            self.lib,
            self.loc,
            literals.Sign.Single,
            None,
            aggregates.AggregateFunction.Count,
            [e1],
            None,
        )
        a2 = body.BodyAggregate(
            self.lib,
            self.loc,
            literals.Sign.NoSign,
            lg1,
            aggregates.AggregateFunction.Sum,
            [e1],
            rg1,
        )

        assert e1.location == self.loc
        assert e1.tuple == [t1, t2]
        assert e1.condition == [l1, l2]

        assert a1.location == self.loc
        assert a1.sign == literals.Sign.Single
        assert a1.left is None
        assert a1.function == aggregates.AggregateFunction.Count
        assert a1.elements == [e1]
        assert a1.right is None

        assert a2.location == self.loc
        assert a2.sign == literals.Sign.NoSign
        assert a2.left == lg1
        assert a2.function == aggregates.AggregateFunction.Sum
        assert a2.elements == [e1]
        assert a2.right == rg1

        assert str(e1) == "5,X: not p(X), r(X)"
        assert str(a1) == "not #count { 5,X: not p(X), r(X) }"
        assert str(a2) == "5 < #sum { 5,X: not p(X), r(X) } <= 5"

    def test_body_sort(self):
        """
        Test body sort literals.
        """
        left = ast.parse_term(self.lib, "(X,Y)")
        value = ast.parse_term(self.lib, "Z")
        condition = ast.parse_literal(self.lib, "p(Z)")
        element = body.BodyAggregateElement(self.lib, self.loc, [value], [condition])
        sort = body.BodySort(self.lib, self.loc, left, [element])

        assert sort.location == self.loc
        assert sort.left == left
        assert sort.elements == [element]
        assert str(sort) == "(X,Y) = #sort { Z: p(Z) }"

        parsed = ast.parse_body_literal(self.lib, "(X,Y) = #sort { Z: p(Z) }")
        assert isinstance(parsed, body.BodySort)
        assert parsed == sort

    def test_body_theory_atom(self):
        """
        Test body theory atom.
        """
        t1 = ast.parse_term(self.lib, "f(X)")
        tt1 = theory.TheoryTermSymbolic(
            self.lib, self.loc, parse_term(self.lib, "f(1,2)")
        )
        tt2 = theory.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "5"))
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = theory.TheoryAtomElement(self.lib, self.loc, [tt1, tt2], [l1, l2])
        rg1 = theory.TheoryRightGuard(self.lib, "<>", tt2)
        a1 = body.BodyTheoryAtom(
            self.lib, self.loc, literals.Sign.Single, t1, [e1], rg1
        )
        a2 = body.BodyTheoryAtom(
            self.lib, self.loc, literals.Sign.Single, t1, [e1], None
        )

        assert rg1.theory_operator == "<>"
        assert rg1.term == tt2

        assert e1.location == self.loc
        assert e1.tuple == [tt1, tt2]
        assert e1.condition == [l1, l2]

        assert a1.location == self.loc
        assert a1.sign == literals.Sign.Single
        assert a1.name == t1
        assert a1.elements == [e1]
        assert a1.right == rg1

        assert a2.right is None

        assert str(e1) == "f(1,2),5: not p(X), r(X)"
        assert str(rg1) == " <> 5"
        assert str(a1) == "not &f(X) { f(1,2),5: not p(X), r(X) } <> 5"

    def test_statement_rule(self):
        """
        Test rule.
        """
        h = head.HeadSimpleLiteral(self.lib, ast.parse_literal(self.lib, "not q(X)"))
        b = body.BodySimpleLiteral(self.lib, ast.parse_literal(self.lib, "p(X)"))
        r = statements.StatementRule(self.lib, self.loc, h, [b])

        assert r.head == h
        assert r.body == [b]
        assert str(r) == "not q(X) :- p(X)."

    def test_statement_theory(self):
        """
        Test theory definition.
        """
        od1 = statements.TheoryOperatorDefinition(
            self.lib, self.loc, "+", 3, statements.TheoryOperatorType.BinaryLeft
        )

        assert od1.location == self.loc
        assert od1.name == "+"
        assert od1.priority == 3
        assert od1.operator_type == statements.TheoryOperatorType.BinaryLeft
        assert str(od1) == "+ : 3, binary, left"

        td1 = statements.TheoryTermDefinition(self.lib, self.loc, "t", [od1])
        assert td1.location == self.loc
        assert td1.name == "t"
        assert td1.operators == [od1]
        assert str(td1) == "t { + : 3, binary, left }"

        gd1 = statements.TheoryGuardDefinition(self.lib, ["+", "-"], "t")
        assert gd1.operators == ["+", "-"]
        assert gd1.term == "t"
        assert str(gd1) == "{+,-}, t"

        ad1 = statements.TheoryAtomDefinition(
            self.lib, self.loc, "p", 1, "t", None, statements.TheoryAtomType.Directive
        )
        ad2 = statements.TheoryAtomDefinition(
            self.lib, self.loc, "p", 1, "t", gd1, statements.TheoryAtomType.Directive
        )
        assert ad1.location == self.loc
        assert ad1.name == "p"
        assert ad1.arity == 1
        assert ad1.term == "t"
        assert ad1.guard is None
        assert ad1.atom_type == statements.TheoryAtomType.Directive
        assert ad2.guard == gd1
        assert str(ad1) == "&p/1: t, directive"
        assert str(ad2) == "&p/1: t, {+,-}, t, directive"

        d1 = statements.StatementTheory(self.lib, self.loc, "t", [td1], [ad1, ad2])
        assert d1.location == self.loc
        assert d1.name == "t"
        assert d1.terms == [td1]
        assert d1.atoms, [ad1 == ad2]
        assert str(d1) == dedent("""\
            #theory t {
              t { + : 3, binary, left };
              &p/1: t, directive;
              &p/1: t, {+,-}, t, directive
            }.""")

    def test_statement_optimize(self):
        """
        Test optimization statements.
        """
        tup = [ast.parse_term(self.lib, "X"), ast.parse_term(self.lib, "Y")]
        weight = ast.parse_term(self.lib, "5")
        prio = ast.parse_term(self.lib, "2")
        t1 = statements.OptimizeTuple(self.lib, weight, None, tup)
        t2 = statements.OptimizeTuple(self.lib, weight, prio, tup)
        assert t1.weight == weight
        assert t1.priority is None
        assert t1.terms == tup
        assert t2.priority == prio
        assert str(t1) == "5,X,Y"
        assert str(t2) == "5@2,X,Y"

        l1 = ast.parse_literal(self.lib, "p(X)")
        l2 = ast.parse_literal(self.lib, "q(X)")
        e1 = statements.OptimizeElement(self.lib, t1, [l1, l2])
        e2 = statements.OptimizeElement(self.lib, t2, [l1, l2])
        assert e1.tuple == t1
        assert e1.condition == [l1, l2]
        assert str(e1) == "5,X,Y: p(X), q(X)"
        assert str(e2) == "5@2,X,Y: p(X), q(X)"

        so1 = statements.StatementOptimize(
            self.lib, self.loc, [e1, e2], statements.OptimizeType.Minimize
        )
        assert so1.location == self.loc
        assert so1.elements, [e1 == e2]
        assert so1.optimize_type == statements.OptimizeType.Minimize
        assert str(so1) == "#minimize { 5,X,Y: p(X), q(X); 5@2,X,Y: p(X), q(X) }."

        lits = [
            body.BodySimpleLiteral(self.lib, l1),
            body.BodySimpleLiteral(self.lib, l2),
        ]
        sw1 = statements.StatementWeakConstraint(self.lib, self.loc, lits, t1)
        assert sw1.body == lits
        assert sw1.tuple == t1
        assert str(sw1) == " :~ p(X); q(X). [5,X,Y]"

    def test_statement_show(self):
        """
        Test show statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = statements.StatementShow(self.lib, self.loc, t1, [l1, l2])
        assert s1.location == self.loc
        assert s1.term == t1
        assert s1.body == [l1, l2]
        assert str(s1) == "#show -p(X): q(X); p(X)."

        s2 = statements.StatementShowSignature(self.lib, self.loc, "p", 2)
        s3 = statements.StatementShowSignature(self.lib, self.loc, "q", 2, True)
        s4 = statements.StatementShowSignature(self.lib, self.loc, "p", 2, False, False)
        assert s2.location == self.loc
        assert s2.name == "p"
        assert s2.arity == 2
        assert not s2.sign
        assert s2.value
        assert s3.sign
        assert s3.value
        assert not s4.value
        assert str(s2) == "#show p/2. [true]"
        assert str(s3) == "#show -q/2. [true]"
        assert str(s4) == "#show p/2. [false]"

        s5 = statements.StatementShowNothing(self.lib, self.loc)
        assert str(s5) == "#show."

    def test_statement_project(self):
        """
        Test project statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = statements.StatementProject(self.lib, self.loc, t1, [l1, l2])
        assert s1.location == self.loc
        assert s1.atom == t1
        assert s1.body == [l1, l2]
        assert str(s1) == "#project -p(X): q(X); p(X)."

        s2 = statements.StatementProjectSignature(self.lib, self.loc, "p", 2)
        s3 = statements.StatementProjectSignature(self.lib, self.loc, "q", 2, True)
        assert s2.location == self.loc
        assert s2.name == "p"
        assert s2.arity == 2
        assert not s2.sign
        assert s3.sign
        assert str(s2) == "#project p/2."
        assert str(s3) == "#project -q/2."

    def test_statement_defined(self):
        """
        Test defined statements.
        """
        s2 = statements.StatementDefined(self.lib, self.loc, "p", 2)
        s3 = statements.StatementDefined(self.lib, self.loc, "q", 2, True)
        assert s2.location == self.loc
        assert s2.name == "p"
        assert s2.arity == 2
        assert not s2.sign
        assert s3.sign
        assert str(s2) == "#defined p/2."
        assert str(s3) == "#defined -q/2."

    def test_statement_external(self):
        """
        Test external statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        t2 = ast.parse_term(self.lib, "true")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = statements.StatementExternal(self.lib, self.loc, t1, [l1, l2])
        s2 = statements.StatementExternal(self.lib, self.loc, t1, [l1, l2], t2)
        assert s1.location == self.loc
        assert s1.atom == t1
        assert s1.body == [l1, l2]
        assert s1.external_type is None
        assert s2.external_type == t2
        assert str(s1) == "#external -p(X): q(X); p(X)."
        assert str(s2) == "#external -p(X): q(X); p(X). [true]"

    def test_statement_edge(self):
        """
        Test external statements.
        """
        u1 = ast.parse_term(self.lib, "u")
        v1 = ast.parse_term(self.lib, "v")
        u2 = ast.parse_term(self.lib, "x")
        v2 = ast.parse_term(self.lib, "y")
        e1 = statements.Edge(self.lib, u1, v1)
        e2 = statements.Edge(self.lib, u2, v2)
        assert e1.u == u1
        assert e1.v == v1
        assert e2.u == u2
        assert e2.v == v2
        assert str(e1) == "u,v"
        assert str(e2) == "x,y"

        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = statements.StatementEdge(self.lib, self.loc, [e1, e2], [l1, l2])
        assert s1.location == self.loc
        assert s1.pool == [e1, e2]
        assert s1.body == [l1, l2]
        assert str(s1) == "#edge (u,v;x,y): q(X); p(X)."

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
        s1 = statements.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m)
        s2 = statements.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m, p)
        assert s1.atom == a
        assert s1.weight == w
        assert s1.modifier == m
        assert s1.priority is None
        assert s2.priority == p
        assert str(s1) == "#heuristic a: q(X); p(X). [w,m]"
        assert str(s2) == "#heuristic a: q(X); p(X). [w@p,m]"

    def test_statement_include(self):
        """
        Test include statements.
        """
        s1 = statements.StatementInclude(
            self.lib, self.loc, "file", statements.IncludeType.System
        )
        assert s1.location == self.loc
        assert s1.value == "file"
        assert s1.include_type == statements.IncludeType.System
        assert str(s1) == '#include "file".'

    def test_statement_program(self):
        """
        Test program statements.
        """
        s1 = statements.StatementProgram(self.lib, self.loc, "step", ["t", "k"])
        assert s1.location == self.loc
        assert s1.name == "step"
        assert s1.arguments == ["t", "k"]
        assert str(s1) == "#program step(t,k)."

    def test_statement_script(self):
        """
        Test script statements.
        """
        s1 = statements.StatementScript(
            self.lib, self.loc, "def p(x): return x", "python"
        )
        assert s1.location == self.loc
        assert s1.value == "def p(x): return x"
        assert s1.script_type == "python"
        assert str(s1) == "#script (python)def p(x): return x#end."

    def test_statement_const(self):
        """
        Test const statements.
        """
        t1 = ast.parse_term(self.lib, "f(2+3)")
        s1 = statements.StatementConst(
            self.lib, self.loc, "x", t1, statements.Precedence.Override
        )
        assert s1.location == self.loc
        assert s1.name == "x"
        assert s1.value == t1
        assert s1.precedence == statements.Precedence.Override
        assert str(s1) == "#const x=f(2+3). [override]"

    def test_statement_parts(self):
        """
        Test parts statements.
        """
        a1 = [parse_term(self.lib, "1"), parse_term(self.lib, "2")]
        e1 = statements.ProgramPart(self.lib, "base", [])
        e2 = statements.ProgramPart(self.lib, "step", a1)
        s1 = statements.StatementParts(
            self.lib, self.loc, [e1, e2], statements.Precedence.Override
        )
        assert s1.location == self.loc
        assert e1.name == "base"
        assert not e1.arguments
        assert e2.name == "step"
        assert e2.arguments == a1
        assert s1.precedence == statements.Precedence.Override
        assert s1.elements == [e1, e2]
        assert str(s1) == "#parts base,step(1,2). [override]"

    def test_statement_comment(self):
        """
        Test const statements.
        """
        s1 = statements.StatementComment(
            self.lib, self.loc, "% something arbitrary", statements.CommentType.Line
        )
        assert s1.location == self.loc
        assert s1.value == "% something arbitrary"
        assert s1.comment_type == statements.CommentType.Line
        assert str(s1) == "% something arbitrary"

    def test_parse(self):
        """
        Test parsing of asts.
        """
        term = "-f(X+Y,3)"
        assert str(ast.parse_term(self.lib, term)) == term
        theory_term = "(f ** X)"
        assert str(ast.parse_theory_term(self.lib, theory_term)) == theory_term
        lit = "not not p(X+2)"
        assert str(ast.parse_literal(self.lib, lit)) == lit
        head_lit = "a; b: c"
        assert str(ast.parse_head_literal(self.lib, head_lit)) == head_lit
        body_lit = "b: c"
        assert str(ast.parse_body_literal(self.lib, body_lit)) == body_lit
        stm = "a; b: c :- d: e."
        assert str(ast.parse_statement(self.lib, stm)) == stm

    def test_rewrite(self):
        """
        Test rewriting of statements.
        """

        def simp(stm, params=()):
            ctx = ast.RewriteContext(self.lib)
            ctx.project_anonymous = True
            for param in params:
                ctx.add_param(param)

            return [
                str(x)
                for x in ast.rewrite_statement(ctx, ast.parse_statement(self.lib, stm))
            ]

        stm = "a; b: c :- d: e."
        assert simp(stm) == [stm]

        stm = "p(X;Y) :- q(X,2*3); r(Y)."
        assert simp(stm) == ["p(X) :- q(X,6); r(*).", "p(Y) :- q(*,6); r(Y)."]

        stm = "p(X;Y) :- q(X+1,2*3), r(Y,t+1)."
        assert simp(stm, ["t"]) == [
            "p(X) :- q(1*X+1,6); r(*,1*t+1).",
            "p(Y) :- q(1*X+1,6); r(Y,1*t+1).",
        ]

        stm = "p(t)."
        assert simp(stm, ["t"]) == ["p(t)."]

        stm = " :- not p(_)."
        assert simp(stm, ["t"]) == [" :- not p(*)."]

        stm = "p(1;t) :- #false: p(t)."
        assert simp(stm, ["t"]) == ["p(1) :- #false: p(t).", "p(t) :- #false: p(t)."]

    def test_scan(self):
        """
        Test the statement scanner.
        """
        stms = []
        ast.parse_string(self.lib, "a. b. c.", lambda stm: stms.append(str(stm)))
        assert stms == ["#program base.", "a.", "b.", "c."]

    def test_visit(self):
        """
        Test visiting.
        """
        variables = []

        @singledispatch
        def dispatch(arg, prefix):
            arg.visit(dispatch, prefix)

        @dispatch.register
        def _(var: terms.TermVariable, prefix):
            variables.append(prefix + var.name)

        stm = ast.parse_statement(self.lib, "p(A) :- q(B,C), D = {}.")
        dispatch(stm, "_")
        assert variables == ["_A", "_B", "_C", "_D"]

    def test_transform(self):
        """
        Test transformation and update.
        """

        @singledispatch
        def dispatch(expr, prefix):
            return expr.transform(self.lib, dispatch, prefix)

        @dispatch.register
        def _(var: terms.TermVariable, prefix):
            return var.update(self.lib, name=prefix + var.name)

        stm = ast.parse_statement(self.lib, "a(X) :- b(X).")
        assert str(dispatch(stm, "_")) == "a(_X) :- b(_X)."

    def test_cmp(self):
        """
        Test comparison functions.
        """
        x = terms.TermVariable(self.lib, self.loc, "X")
        a = terms.TermVariable(self.lib, self.loc, "_", True)

        assert x == x  # pylint: disable=comparison-with-itself
        assert a != x
        assert x < a
        assert x <= a
        assert a > x
        assert a >= x

        assert hash(x) == hash(x)
        assert hash(x) != hash(a)

    def test_rewrite_error(self):
        """
        Test rewriting with error.
        """
        ctx = ast.RewriteContext(self.lib)
        stm = ast.parse_statement(self.lib, "p(X).")
        with pytest.raises(RuntimeError, match="rewriting failed"):
            ast.rewrite_statement(ctx, stm)
