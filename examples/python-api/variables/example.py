from functools import singledispatch

from clingo import ast
from clingo.ast import body, head, statements, terms, theory
from clingo.core import Library


@singledispatch
def _get_global(stm, variables):
    stm.visit(_get_global, variables)


@_get_global.register(terms.TermVariable)
@_get_global.register(theory.TheoryTermVariable)
def _(term, variables):
    variables.add(term.name)


@_get_global.register(head.HeadDisjunction)
@_get_global.register(body.BodyConditionalLiteral)
@_get_global.register(statements.StatementOptimize)
def _(lit, variables):
    pass


@_get_global.register(body.BodyAggregate)
@_get_global.register(body.BodySetAggregate)
@_get_global.register(head.HeadAggregate)
@_get_global.register(head.HeadSetAggregate)
def _(lit, variables):
    if lit.left:
        _get_global(lit.left, variables)
    if lit.right:
        _get_global(lit.right, variables)


@_get_global.register(head.HeadTheoryAtom)
@_get_global.register(body.BodyTheoryAtom)
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


@_get_variables.register(terms.TermVariable)
@_get_variables.register(theory.TheoryTermVariable)
def _(term, variables):
    variables.add(term.name)


def get_variables(stm):
    variables = set()
    _get_variables(stm, variables)
    return variables


def run():
    lib = Library()
    stm = ast.parse_statement(lib, "p(X) :- q(X), Y = { p(X,Z) }.")
    print("statement:", stm)
    print("all variables:", ", ".join(sorted(get_variables(stm))))
    print("global variables:", ", ".join(sorted(get_global(stm))))


run()
