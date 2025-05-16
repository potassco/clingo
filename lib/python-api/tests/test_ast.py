"""
Unit tests for clingo.ast module.
"""

from functools import singledispatch
from textwrap import dedent

from clingo import ast
from clingo.core import Library, Location, Position
from clingo.symbol import parse_term


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
        self._lib = Library()
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
        return ast.TermSymbolic(self.lib, self.loc, parse_term(self.lib, val))

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
        p = ast.Projection(self.lib, self.loc)

        assert p.location == self.loc
        assert str(p) == "*"

    def test_variable(self):
        """
        Test variable terms.
        """
        x = ast.TermVariable(self.lib, self.loc, "X", False)
        a = ast.TermVariable(self.lib, self.loc, "_", True)

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
        p = ast.TermSymbolic(self.lib, self.loc, s)

        assert p.location == self.loc
        assert p.symbol == s
        assert str(p) == "f(1,2)"

    def test_absolute(self):
        """
        Test absolute term.
        """
        p = ast.TermAbsolute(self.lib, self.loc, [self.sym("1"), self.sym("-2")])

        assert p.location == self.loc
        assert p.pool, [self.sym("1") == self.sym("-2")]
        assert str(p) == "|1;-2|"

    def test_unary(self):
        """
        Test unary term.
        """
        p = ast.TermUnaryOperation(
            self.lib, self.loc, ast.UnaryOperator.Minus, self.sym("-2")
        )

        assert p.location == self.loc
        assert p.operator_type == ast.UnaryOperator.Minus
        assert p.right == self.sym("-2")
        assert str(p) == "-(-2)"

    def test_binary(self):
        """
        Test binary term.
        """
        p = ast.TermBinaryOperation(
            self.lib, self.loc, self.sym("1"), ast.BinaryOperator.Plus, self.sym("-2")
        )

        assert p.location == self.loc
        assert p.left == self.sym("1")
        assert p.operator_type == ast.BinaryOperator.Plus
        assert p.right == self.sym("-2")
        assert str(p) == "1+(-2)"

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

        assert p.location == self.loc
        assert p.pool == a
        assert str(p) == "(1,2;3)"

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
        x = ast.TheoryTermVariable(self.lib, self.loc, "X", False)
        a = ast.TheoryTermVariable(self.lib, self.loc, "_", True)

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
        p = ast.TheoryTermSymbolic(self.lib, self.loc, s)

        assert p.location == self.loc
        assert p.symbol == s
        assert str(p) == "f(1,2)"

    def test_theory_tuple(self):
        """
        Test theory tuple term.
        """
        p = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "p(1,2)"))
        q = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))
        a = [p, q]
        b = ast.TheoryTermTuple(self.lib, self.loc, ast.TheoryTupleType.Set, a)

        assert b.location == self.loc
        assert b.arguments == a
        assert b.tuple_type == ast.TheoryTupleType.Set
        assert str(b) == "{p(1,2),q}"

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
        p = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "p(1,2)"))
        q = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "q"))

        a = ast.UnparsedElement(self.lib, ["+", "-"], p)
        b = ast.UnparsedElement(self.lib, ["*"], q)

        x = ast.TheoryTermUnparsed(self.lib, self.loc, [a, b])

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
        p = ast.LiteralBoolean(self.lib, self.loc, ast.Sign.Single, True)

        assert p.location == self.loc
        assert p.sign == ast.Sign.Single
        assert p.value
        assert str(p) == "not #true"

    def test_symbolic_literal(self):
        """
        Test symbolic literal.
        """
        a = ast.parse_term(self.lib, "-f(X)")
        p = ast.LiteralSymbolic(self.lib, self.loc, ast.Sign.Single, a)

        assert p.location == self.loc
        assert p.sign == ast.Sign.Single
        assert p.atom == a
        assert str(p) == "not -f(X)"

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

        assert p.location == self.loc
        assert p.sign == ast.Sign.Single
        assert p.left == a
        assert p.right == [b, c]
        assert str(p) == "not X<Y<=Z"

    def test_head_simple_literal(self):
        """
        Test simple head literal.
        """
        s = "not p(X)"
        p = ast.HeadSimpleLiteral(self.lib, ast.parse_literal(self.lib, s))
        q = ast.parse_literal(self.lib, s)

        assert p.literal == q
        assert str(p) == s

    def test_head_disjunction(self):
        """
        Test head disjunction literal.
        """
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        l3 = ast.HeadConditionalLiteral(self.lib, self.loc, l2, [l1])

        p = ast.HeadDisjunction(self.lib, self.loc, [l2, l3])

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
        e1 = ast.SetAggregateElement(self.lib, self.loc, l1, [l2])
        lg1 = ast.LeftGuard(self.lib, t1, ast.Relation.Less)
        rg1 = ast.RightGuard(self.lib, ast.Relation.LessEqual, t1)
        a1 = ast.HeadSetAggregate(self.lib, self.loc, None, [e1], None)
        a2 = ast.HeadSetAggregate(self.lib, self.loc, lg1, [e1], rg1)

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

        assert e1.location == self.loc
        assert e1.tuple == [t1, t2]
        assert e1.condition == [l1, l2]

        assert a1.location == self.loc
        assert a1.left is None
        assert a1.function == ast.AggregateFunction.Count
        assert a1.elements == [e1]
        assert a1.right is None

        assert a2.location == self.loc
        assert a2.left == lg1
        assert a2.function == ast.AggregateFunction.Sum
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
        tt1 = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "f(1,2)"))
        tt2 = ast.TheoryTermSymbolic(self.lib, self.loc, parse_term(self.lib, "5"))
        l1 = ast.parse_literal(self.lib, "not p(X)")
        l2 = ast.parse_literal(self.lib, "r(X)")
        e1 = ast.TheoryAtomElement(self.lib, self.loc, [tt1, tt2], [l1, l2])
        rg1 = ast.TheoryRightGuard(self.lib, "<>", tt2)
        a1 = ast.HeadTheoryAtom(self.lib, self.loc, t1, [e1], rg1)
        a2 = ast.HeadTheoryAtom(self.lib, self.loc, t1, [e1], None)

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
        p = ast.BodySimpleLiteral(self.lib, ast.parse_literal(self.lib, s))
        q = ast.parse_literal(self.lib, s)

        assert p.literal == q
        assert str(p) == s

    def test_body_conditional_literal(self):
        """
        Test body conditional literal.
        """
        s = ast.parse_literal(self.lib, "not p(X)")
        t = ast.parse_literal(self.lib, "r(X)")
        p = ast.BodyConditionalLiteral(self.lib, self.loc, s, [t])

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
        e1 = ast.SetAggregateElement(self.lib, self.loc, l1, [l2])
        lg1 = ast.LeftGuard(self.lib, t1, ast.Relation.Less)
        rg1 = ast.RightGuard(self.lib, ast.Relation.LessEqual, t1)
        a1 = ast.BodySetAggregate(self.lib, self.loc, ast.Sign.Single, None, [e1], None)
        a2 = ast.BodySetAggregate(self.lib, self.loc, ast.Sign.NoSign, lg1, [e1], rg1)

        assert e1.location == self.loc
        assert e1.literal == l1
        assert e1.condition == [l2]

        assert lg1.term == t1
        assert lg1.relation == ast.Relation.Less

        assert rg1.term == t1
        assert rg1.relation == ast.Relation.LessEqual

        assert a1.location == self.loc
        assert a1.sign == ast.Sign.Single
        assert a1.left is None
        assert a1.elements == [e1]
        assert a1.right is None

        assert a2.location == self.loc
        assert a2.sign == ast.Sign.NoSign
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

        assert e1.location == self.loc
        assert e1.tuple == [t1, t2]
        assert e1.condition == [l1, l2]

        assert a1.location == self.loc
        assert a1.sign == ast.Sign.Single
        assert a1.left is None
        assert a1.function == ast.AggregateFunction.Count
        assert a1.elements == [e1]
        assert a1.right is None

        assert a2.location == self.loc
        assert a2.sign == ast.Sign.NoSign
        assert a2.left == lg1
        assert a2.function == ast.AggregateFunction.Sum
        assert a2.elements == [e1]
        assert a2.right == rg1

        assert str(e1) == "5,X: not p(X), r(X)"
        assert str(a1) == "not #count { 5,X: not p(X), r(X) }"
        assert str(a2) == "5 < #sum { 5,X: not p(X), r(X) } <= 5"

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

        assert rg1.theory_operator == "<>"
        assert rg1.term == tt2

        assert e1.location == self.loc
        assert e1.tuple == [tt1, tt2]
        assert e1.condition == [l1, l2]

        assert a1.location == self.loc
        assert a1.sign == ast.Sign.Single
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
        h = ast.HeadSimpleLiteral(self.lib, ast.parse_literal(self.lib, "not q(X)"))
        b = ast.BodySimpleLiteral(self.lib, ast.parse_literal(self.lib, "p(X)"))
        r = ast.StatementRule(self.lib, self.loc, h, [b])

        assert r.head == h
        assert r.body == [b]
        assert str(r) == "not q(X) :- p(X)."

    def test_statement_theory(self):
        """
        Test theory definition.
        """
        od1 = ast.TheoryOperatorDefinition(
            self.lib, self.loc, "+", 3, ast.TheoryOperatorType.BinaryLeft
        )

        assert od1.location == self.loc
        assert od1.name == "+"
        assert od1.priority == 3
        assert od1.operator_type == ast.TheoryOperatorType.BinaryLeft
        assert str(od1) == "+ : 3, binary, left"

        td1 = ast.TheoryTermDefinition(self.lib, self.loc, "t", [od1])
        assert td1.location == self.loc
        assert td1.name == "t"
        assert td1.operators == [od1]
        assert str(td1) == "t { + : 3, binary, left }"

        gd1 = ast.TheoryGuardDefinition(self.lib, ["+", "-"], "t")
        assert gd1.operators == ["+", "-"]
        assert gd1.term == "t"
        assert str(gd1) == "{+,-}, t"

        ad1 = ast.TheoryAtomDefinition(
            self.lib, self.loc, "p", 1, "t", None, ast.TheoryAtomType.Directive
        )
        ad2 = ast.TheoryAtomDefinition(
            self.lib, self.loc, "p", 1, "t", gd1, ast.TheoryAtomType.Directive
        )
        assert ad1.location == self.loc
        assert ad1.name == "p"
        assert ad1.arity == 1
        assert ad1.term == "t"
        assert ad1.guard is None
        assert ad1.atom_type == ast.TheoryAtomType.Directive
        assert ad2.guard == gd1
        assert str(ad1) == "&p/1: t, directive"
        assert str(ad2) == "&p/1: t, {+,-}, t, directive"

        d1 = ast.StatementTheory(self.lib, self.loc, "t", [td1], [ad1, ad2])
        assert d1.location == self.loc
        assert d1.name == "t"
        assert d1.terms == [td1]
        assert d1.atoms, [ad1 == ad2]
        assert str(d1) == dedent(
            """\
            #theory t {
              t { + : 3, binary, left };
              &p/1: t, directive;
              &p/1: t, {+,-}, t, directive
            }."""
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
        assert t1.weight == weight
        assert t1.priority is None
        assert t1.terms == terms
        assert t2.priority == prio
        assert str(t1) == "5,X,Y"
        assert str(t2) == "5@2,X,Y"

        l1 = ast.parse_literal(self.lib, "p(X)")
        l2 = ast.parse_literal(self.lib, "q(X)")
        e1 = ast.OptimizeElement(self.lib, t1, [l1, l2])
        e2 = ast.OptimizeElement(self.lib, t2, [l1, l2])
        assert e1.tuple == t1
        assert e1.condition == [l1, l2]
        assert str(e1) == "5,X,Y: p(X), q(X)"
        assert str(e2) == "5@2,X,Y: p(X), q(X)"

        so1 = ast.StatementOptimize(
            self.lib, self.loc, [e1, e2], ast.OptimizeType.Minimize
        )
        assert so1.location == self.loc
        assert so1.elements, [e1 == e2]
        assert so1.optimize_type == ast.OptimizeType.Minimize
        assert str(so1) == "#minimize { 5,X,Y: p(X), q(X); 5@2,X,Y: p(X), q(X) }."

        body = [
            ast.BodySimpleLiteral(self.lib, l1),
            ast.BodySimpleLiteral(self.lib, l2),
        ]
        sw1 = ast.StatementWeakConstraint(self.lib, self.loc, body, t1)
        assert sw1.body == body
        assert sw1.tuple == t1
        assert str(sw1) == " :~ p(X); q(X). [5,X,Y]"

    def test_statement_show(self):
        """
        Test show statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementShow(self.lib, self.loc, t1, [l1, l2])
        assert s1.location == self.loc
        assert s1.term == t1
        assert s1.body == [l1, l2]
        assert str(s1) == "#show -p(X): q(X); p(X)."

        s2 = ast.StatementShowSignature(self.lib, self.loc, "p", 2)
        s3 = ast.StatementShowSignature(self.lib, self.loc, "q", 2, True)
        assert s2.location == self.loc
        assert s2.name == "p"
        assert s2.arity == 2
        assert not s2.sign
        assert s3.sign
        assert str(s2) == "#show p/2."
        assert str(s3) == "#show -q/2."

        s4 = ast.StatementShowNothing(self.lib, self.loc)
        assert str(s4) == "#show."

    def test_statement_project(self):
        """
        Test project statements.
        """
        t1 = ast.parse_term(self.lib, "-p(X)")
        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementProject(self.lib, self.loc, t1, [l1, l2])
        assert s1.location == self.loc
        assert s1.atom == t1
        assert s1.body == [l1, l2]
        assert str(s1) == "#project -p(X): q(X); p(X)."

        s2 = ast.StatementProjectSignature(self.lib, self.loc, "p", 2)
        s3 = ast.StatementProjectSignature(self.lib, self.loc, "q", 2, True)
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
        s2 = ast.StatementDefined(self.lib, self.loc, "p", 2)
        s3 = ast.StatementDefined(self.lib, self.loc, "q", 2, True)
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
        s1 = ast.StatementExternal(self.lib, self.loc, t1, [l1, l2])
        s2 = ast.StatementExternal(self.lib, self.loc, t1, [l1, l2], t2)
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
        e1 = ast.Edge(self.lib, u1, v1)
        e2 = ast.Edge(self.lib, u2, v2)
        assert e1.u == u1
        assert e1.v == v1
        assert e2.u == u2
        assert e2.v == v2
        assert str(e1) == "u,v"
        assert str(e2) == "x,y"

        l1 = ast.parse_body_literal(self.lib, "q(X)")
        l2 = ast.parse_body_literal(self.lib, "p(X)")
        s1 = ast.StatementEdge(self.lib, self.loc, [e1, e2], [l1, l2])
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
        s1 = ast.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m)
        s2 = ast.StatementHeuristic(self.lib, self.loc, a, [l1, l2], w, m, p)
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
        s1 = ast.StatementInclude(self.lib, self.loc, "file", ast.IncludeType.System)
        assert s1.location == self.loc
        assert s1.value == "file"
        assert s1.include_type == ast.IncludeType.System
        assert str(s1) == '#include "file".'

    def test_statement_program(self):
        """
        Test program statements.
        """
        s1 = ast.StatementProgram(self.lib, self.loc, "step", ["t", "k"])
        assert s1.location == self.loc
        assert s1.name == "step"
        assert s1.arguments == ["t", "k"]
        assert str(s1) == "#program step(t,k)."

    def test_statement_script(self):
        """
        Test script statements.
        """
        s1 = ast.StatementScript(self.lib, self.loc, "def p(x): return x", "python")
        assert s1.location == self.loc
        assert s1.value == "def p(x): return x"
        assert s1.script_type == "python"
        assert str(s1) == "#script (python)def p(x): return x#end."

    def test_statement_const(self):
        """
        Test const statements.
        """
        t1 = ast.parse_term(self.lib, "f(2+3)")
        s1 = ast.StatementConst(self.lib, self.loc, "x", t1, ast.Precedence.Override)
        assert s1.location == self.loc
        assert s1.name == "x"
        assert s1.value == t1
        assert s1.precedence == ast.Precedence.Override
        assert str(s1) == "#const x=f(2+3). [override]"

    def test_statement_parts(self):
        """
        Test parts statements.
        """
        a1 = [parse_term(self.lib, "1"), parse_term(self.lib, "2")]
        e1 = ast.ProgramPart(self.lib, "base", [])
        e2 = ast.ProgramPart(self.lib, "step", a1)
        s1 = ast.StatementParts(self.lib, self.loc, [e1, e2], ast.Precedence.Override)
        assert s1.location == self.loc
        assert e1.name == "base"
        assert not e1.arguments
        assert e2.name == "step"
        assert e2.arguments == a1
        assert s1.precedence == ast.Precedence.Override
        assert s1.elements == [e1, e2]
        assert str(s1) == "#parts base,step(1,2). [override]"

    def test_statement_comment(self):
        """
        Test const statements.
        """
        s1 = ast.StatementComment(
            self.lib, self.loc, "% something arbitrary", ast.CommentType.Line
        )
        assert s1.location == self.loc
        assert s1.value == "% something arbitrary"
        assert s1.comment_type == ast.CommentType.Line
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
        with ast.Scanner(self.lib, "a. b. c.") as scanner:
            assert [str(stm) for stm in scanner] == ["a.", "b.", "c."]

    def test_visit(self):
        """
        Test visiting.
        """
        variables = []

        @singledispatch
        def dispatch(arg, prefix):
            arg.visit(dispatch, prefix)

        @dispatch.register
        def _(var: ast.TermVariable, prefix):
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
        def _(var: ast.TermVariable, prefix):
            return var.update(self.lib, name=prefix + var.name)

        stm = ast.parse_statement(self.lib, "a(X) :- b(X).")
        assert str(dispatch(stm, "_")) == "a(_X) :- b(_X)."

    def test_cmp(self):
        """
        Test comparison functions.
        """
        x = ast.TermVariable(self.lib, self.loc, "X")
        a = ast.TermVariable(self.lib, self.loc, "_", True)

        assert x == x  # pylint: disable=comparison-with-itself
        assert a != x
        assert x < a
        assert x <= a
        assert a > x
        assert a >= x

        assert hash(x) == hash(x)
        assert hash(x) != hash(a)
