#script (python)

import clingo
import clingo.ast as ast

def get_type(x):
    return x.atom.type if x.type == ast.ASTType.Literal else x.type

def add(r):
    return [(n, clingo.Function("ok" if str(x) == s and get_type(x) == t else "fail")) for n, (x, s, t) in zip(range(len(r)), r)]

def test():
    loc = None
    id_i = ast.Id(loc, "i")
    trm_v = ast.Variable(loc, "X")
    trm_s = ast.Symbol(loc, 2)
    trm_u = ast.UnaryOperation(loc, ast.UnaryOperator.Absolute, trm_v)
    trm_b = ast.BinaryOperation(loc, ast.BinaryOperator.Multiplication, trm_u, trm_s)
    trm_e = ast.BinaryOperation(loc, ast.BinaryOperator.Power, trm_u, trm_s)
    trm_f = ast.Function(loc, "f", [trm_v,trm_s,trm_u,trm_b], True)
    trm_p = ast.Pool(loc, [trm_s,trm_v])
    csp_p = ast.CSPProduct(loc, trm_s, trm_v)
    csp_s = ast.CSPSum(loc, [csp_p, csp_p])
    csp_g = ast.CSPGuard(ast.ComparisonOperator.Equal, csp_s)
    lit_b = ast.BooleanConstant(True);
    lit_s = ast.Literal(loc, ast.Sign.Negation, ast.SymbolicAtom(ast.Function(loc, "p", [trm_s], False)));
    lit_t = ast.Literal(loc, ast.Sign.NoSign, ast.SymbolicAtom(ast.Function(loc, "q", [trm_s], False)));
    lit_r = ast.Comparison(ast.ComparisonOperator.LessThan, trm_v, trm_s)
    lit_c = ast.CSPLiteral(loc, csp_s, [csp_g])
    lit_l = ast.ConditionalLiteral(loc, lit_s, [lit_b])
    agg_g = ast.AggregateGuard(ast.ComparisonOperator.NotEqual, trm_s)
    agg_l = ast.Literal(loc, ast.Sign.Negation, ast.Aggregate(loc, None, [lit_l], agg_g))
    agg_e = ast.BodyAggregateElement([trm_s, trm_v], [lit_b])
    agg_t = ast.Literal(loc, ast.Sign.DoubleNegation, ast.BodyAggregate(loc, agg_g, ast.AggregateFunction.Sum, [agg_e], None))
    agg_m = ast.Aggregate(loc, agg_g, [lit_l], agg_g)
    agg_f = ast.HeadAggregateElement([trm_s, trm_v], lit_l)
    agg_u = ast.HeadAggregate(loc, agg_g, ast.AggregateFunction.Sum, [agg_f], None)
    agg_d = ast.Disjunction(loc, [lit_l, lit_l])
    agg_h = ast.DisjointElement(loc, [trm_v, trm_s], csp_s, [lit_b, lit_s])
    agg_i = ast.Literal(loc, ast.Sign.DoubleNegation, ast.Disjoint(loc, [agg_h]))
    tha_s = ast.TheorySequence(loc, ast.TheorySequenceType.Set, [trm_v, trm_s])
    tha_f = ast.TheoryFunction(loc, "f", [tha_s, trm_s])
    tha_e = ast.TheoryUnparsedTermElement(["+", "-"], trm_s)
    tha_u = ast.TheoryUnparsedTerm(loc, [tha_e, tha_e])
    tha_g = ast.TheoryGuard("!!", trm_s)
    tha_h = ast.TheoryAtomElement([trm_v, trm_s], [lit_b])
    tha_a = ast.TheoryAtom(loc, trm_v, [tha_h], tha_g)
    thd_o = ast.TheoryOperatorDefinition(loc, "-", 7, ast.TheoryOperatorType.Unary)
    thd_t = ast.TheoryTermDefinition(loc, "t", [thd_o])
    thd_g = ast.TheoryGuardDefinition([">", "<"], "t")
    thd_a = ast.TheoryAtomDefinition(loc, ast.TheoryAtomType.Any, "p", 2, "t", thd_g)
    thd_d = ast.TheoryDefinition(loc, "x", [thd_t], [thd_a])
    stm_r = ast.Rule(loc, lit_s, [lit_s, lit_t])
    stm_c = ast.Definition(loc, "x", trm_s, True)
    stm_s = ast.ShowSignature(loc, "x", 2, True, True)
    stm_i = ast.Defined(loc, "i", 2, True)
    stm_t = ast.ShowTerm(loc, trm_s, [lit_s], True)
    stm_m = ast.Minimize(loc, trm_s, trm_s, [trm_v], [lit_s])
    stm_p = ast.Script(loc, ast.ScriptType.Python, "blub!")
    stm_q = ast.Program(loc, "p", [id_i])
    stm_e = ast.External(loc, trm_s, [lit_s], ast.Function(loc, "false", [], False))
    stm_a = ast.Edge(loc, trm_s, trm_s, [lit_s])
    stm_h = ast.Heuristic(loc, trm_s, [lit_s], trm_s, trm_s, trm_s)
    stm_x = ast.ProjectAtom(loc, trm_s, [lit_s])
    stm_y = ast.ProjectSignature(loc, "p", 2, True)
    return add([
        (id_i, "i", ast.ASTType.Id),
        (trm_v, "X", ast.ASTType.Variable),
        (trm_s, "2", ast.ASTType.Symbol),
        (trm_u, "|X|", ast.ASTType.UnaryOperation),
        (trm_b, "(|X|*2)", ast.ASTType.BinaryOperation),
        (trm_e, "(|X|**2)", ast.ASTType.BinaryOperation),
        (trm_f, "@f(X,2,|X|,(|X|*2))", ast.ASTType.Function),
        (trm_p, "(2;X)", ast.ASTType.Pool),
        (csp_p, "2$*$X", ast.ASTType.CSPProduct),
        (csp_s, "2$*$X$+2$*$X", ast.ASTType.CSPSum),
        (csp_g, "$=2$*$X$+2$*$X", ast.ASTType.CSPGuard),
        (lit_b, "#true", ast.ASTType.BooleanConstant),
        (lit_s, "not p(2)", ast.ASTType.SymbolicAtom),
        (lit_r, "X<2", ast.ASTType.Comparison),
        (lit_c, "2$*$X$+2$*$X$=2$*$X$+2$*$X", ast.ASTType.CSPLiteral),
        (lit_l, "not p(2) : #true", ast.ASTType.ConditionalLiteral),
        (agg_g, "AggregateGuard(!=, 2)", ast.ASTType.AggregateGuard),
        (agg_l, "not { not p(2) : #true } != 2", ast.ASTType.Aggregate),
        (agg_e, "2,X : #true", ast.ASTType.BodyAggregateElement),
        (agg_t, "not not 2 != #sum { 2,X : #true }", ast.ASTType.BodyAggregate),
        (agg_m, "2 != { not p(2) : #true } != 2", ast.ASTType.Aggregate),
        (agg_f, "2,X : not p(2) : #true", ast.ASTType.HeadAggregateElement),
        (agg_u, "2 != #sum { 2,X : not p(2) : #true }", ast.ASTType.HeadAggregate),
        (agg_d, "not p(2) : #true; not p(2) : #true", ast.ASTType.Disjunction),
        (agg_h, "X,2 : 2$*$X$+2$*$X : #true,not p(2)", ast.ASTType.DisjointElement),
        (agg_i, "not not #disjoint { X,2 : 2$*$X$+2$*$X : #true,not p(2) }", ast.ASTType.Disjoint),
        (tha_s, "{X,2}", ast.ASTType.TheorySequence),
        (tha_f, "f({X,2},2)", ast.ASTType.TheoryFunction),
        (tha_e, "+ - 2", ast.ASTType.TheoryUnparsedTermElement),
        (tha_u, "(+ - 2 + - 2)", ast.ASTType.TheoryUnparsedTerm),
        (tha_g, "!! 2", ast.ASTType.TheoryGuard),
        (tha_h, "X,2 : #true", ast.ASTType.TheoryAtomElement),
        (tha_a, "&X { X,2 : #true } !! 2", ast.ASTType.TheoryAtom),
        (thd_o, "- : 7, unary", ast.ASTType.TheoryOperatorDefinition),
        (thd_t, "t {\n  - : 7, unary\n}", ast.ASTType.TheoryTermDefinition),
        (thd_g, "{ >, < }, t", ast.ASTType.TheoryGuardDefinition),
        (thd_a, "&p/2 : t, { >, < }, t, any", ast.ASTType.TheoryAtomDefinition),
        (thd_d, "#theory x {\n  t {\n    - : 7, unary\n  };\n  &p/2 : t, { >, < }, t, any\n}.", ast.ASTType.TheoryDefinition),
        (stm_r, "not p(2) :- not p(2); q(2).", ast.ASTType.Rule),
        (stm_c, "#const x = 2.", ast.ASTType.Definition),
        (stm_s, "#show $x/2.", ast.ASTType.ShowSignature),
        (stm_i, "#defined i/2.", ast.ASTType.Defined),
        (stm_t, "#show $2 : not p(2).", ast.ASTType.ShowTerm),
        (stm_m, ":~ not p(2). [2@2,X]", ast.ASTType.Minimize),
        (stm_p, "blub!", ast.ASTType.Script),
        (stm_q, "#program p(i).", ast.ASTType.Program),
        (stm_e, "#external 2 : not p(2). [false]", ast.ASTType.External),
        (stm_a, "#edge (2,2) : not p(2).", ast.ASTType.Edge),
        (stm_h, "#heuristic 2 : not p(2). [2@2,2]", ast.ASTType.Heuristic),
        (stm_x, "#project 2 : not p(2).", ast.ASTType.ProjectAtom),
        (stm_y, "#project p/2.", ast.ASTType.ProjectSignature),
    ])

def main(prg):
    prg.ground([("base", [])])
    prg.solve()

#end.

test(N,R) :- (N,R) = @test().
