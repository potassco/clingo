from functools import singledispatch

from clingo import ast
from clingo.core import Library


@singledispatch
def _get_global(stm, variables):
    stm.visit(_get_global, variables)


@_get_global.register(ast.TermVariable)
@_get_global.register(ast.TheoryTermVariable)
def _(term, variables):
    variables.add(term.name)


@_get_global.register(ast.HeadDisjunction)
@_get_global.register(ast.BodyConditionalLiteral)
@_get_global.register(ast.StatementOptimize)
def _(lit, variables):
    pass


@_get_global.register(ast.BodyAggregate)
@_get_global.register(ast.BodySetAggregate)
@_get_global.register(ast.HeadAggregate)
@_get_global.register(ast.HeadSetAggregate)
def _(lit, variables):
    if lit.left:
        _get_global(lit.left, variables)
    if lit.right:
        _get_global(lit.right, variables)


@_get_global.register(ast.HeadTheoryAtom)
@_get_global.register(ast.BodyTheoryAtom)
def _(lit, variables):
    _get_global(lit.name, variables)
    if lit.right:
        _get_global(lit.right, variables)


def get_global(stm):
    variables = set()
    _get_global(stm, variables)
    return variables


@singledispatch
def _get_variables(stm, variables):
    stm.visit(_get_variables, variables)


@_get_variables.register(ast.TermVariable)
@_get_variables.register(ast.TheoryTermVariable)
def _(term, variables):
    variables.add(term.name)


def get_variables(stm):
    variables = set()
    _get_variables(stm, variables)
    return variables


def run():
    with Library() as lib:
        stm = ast.parse_statement(lib, "p(X) :- q(X), Y = { p(X,Z) }.")
        print("statement:", stm)
        print("all variables:", ", ".join(sorted(get_variables(stm))))
        print("global variables:", ", ".join(sorted(get_global(stm))))


run()
