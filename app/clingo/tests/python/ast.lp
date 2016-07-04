#script (python)

import clingo
import clingo.ast as ast

def add(r):
    return [(n, clingo.Function("ok" if str(x) == s and x.type == t else "fail")) for n, (x, s, t) in zip(range(len(r)), r)]

def test():
    loc = None
    id_i = ast.Id(loc, "i")
    term_v = ast.Variable(loc, "X")
    term_s = ast.Symbol(loc, 2)
    term_u = ast.UnaryOperation(loc, ast.UnaryOperator.Absolute, term_v)
    term_b = ast.BinaryOperation(loc, ast.BinaryOperator.Multiplication, term_u, term_s)
    term_f = ast.Function(loc, "f", [term_v,term_s,term_u,term_b], True)
    term_p = ast.Pool(loc, [term_s,term_v])
    csp_p = ast.CSPProduct(loc, term_s, term_v)
    csp_s = ast.CSPSum(loc, [csp_p, csp_p])
    csp_g = ast.CSPGuard(ast.ComparisonOperator.Equal, csp_s)
    lit_b = ast.BooleanLiteral(loc, True);
    lit_s = ast.SymbolicLiteral(loc, ast.Sign.Negation, ast.Function(loc, "p", [term_s], False));
    lit_r = ast.ComparisonLiteral(loc, ast.ComparisonOperator.LessThan, term_v, term_s)
    lit_c = ast.CSPLiteral(loc, csp_s, [csp_g])
    lit_l = ast.ConditionalLiteral(loc, lit_s, [lit_b])
    agg_g = ast.AggregateGuard(ast.ComparisonOperator.NotEqual, term_s)
    agg_l = ast.BodyLiteralAggregate(loc, ast.Sign.Negation, None, [lit_l], agg_g)
    agg_e = ast.BodyTupleAggregateElement([term_s, term_v], [lit_b])
    agg_t = ast.BodyTupleAggregate(loc, ast.Sign.DoubleNegation, agg_g, ast.AggregateFunction.Sum, [agg_e], None)
    agg_m = ast.HeadLiteralAggregate(loc, agg_g, [lit_l], agg_g)
    agg_f = ast.HeadTupleAggregateElement([term_s, term_v], lit_l)
    agg_u = ast.HeadTupleAggregate(loc, agg_g, ast.AggregateFunction.Sum, [agg_f], None)
    agg_d = ast.Disjunction(loc, [lit_l, lit_l])
    agg_h = ast.DisjointElement(loc, [term_v, term_s], csp_s, [lit_b, lit_s])
    agg_i = ast.Disjoint(loc, ast.Sign.DoubleNegation, [agg_h])
    print "{0}-{1}".format(agg_i, agg_i.type)
    return add([
        (id_i, "i", ast.ASTType.Id),
        (term_v, "X", ast.ASTType.Variable),
        (term_s, "2", ast.ASTType.Symbol),
        (term_u, "|X|", ast.ASTType.UnaryOperation),
        (term_b, "(|X|*2)", ast.ASTType.BinaryOperation),
        (term_f, "@f(X,2,|X|,(|X|*2))", ast.ASTType.Function),
        (term_p, "(2;X)", ast.ASTType.Pool),
        (csp_p, "2$*$X", ast.ASTType.CSPProduct),
        (csp_s, "2$*$X$+2$*$X", ast.ASTType.CSPSum),
        (csp_g, "$=2$*$X$+2$*$X", ast.ASTType.CSPGuard),
        (lit_b, "#true", ast.ASTType.BooleanLiteral),
        (lit_s, "not p(2)", ast.ASTType.SymbolicLiteral),
        (lit_r, "X<2", ast.ASTType.ComparisonLiteral),
        (lit_c, "2$*$X$+2$*$X$=2$*$X$+2$*$X", ast.ASTType.CSPLiteral),
        (lit_l, "not p(2) : #true", ast.ASTType.ConditionalLiteral),
        (agg_g, "AggregateGuard(!=, 2)", ast.ASTType.AggregateGuard),
        (agg_l, "not { not p(2) : #true } != 2", ast.ASTType.BodyLiteralAggregate),
        (agg_e, "2,X : #true", ast.ASTType.BodyTupleAggregateElement),
        (agg_t, "not not 2 != #sum { 2,X : #true }", ast.ASTType.BodyTupleAggregate),
        (agg_m, "2 != { not p(2) : #true } != 2", ast.ASTType.HeadLiteralAggregate),
        (agg_f, "2,X : not p(2) : #true", ast.ASTType.HeadTupleAggregateElement),
        (agg_u, "2 != #sum { 2,X : not p(2) : #true }", ast.ASTType.HeadTupleAggregate),
        (agg_d, "not p(2) : #true; not p(2) : #true", ast.ASTType.Disjunction),
        (agg_h, "X,2 : 2$*$X$+2$*$X : #true,not p(2)", ast.ASTType.DisjointElement),
        (agg_i, "not not #disjoint { X,2 : 2$*$X$+2$*$X : #true,not p(2) }", ast.ASTType.Disjoint),
    ])

def main(prg):
    prg.ground([("base", [])])
    prg.solve()

#end.

test(N,R) :- (N,R) = @test().
