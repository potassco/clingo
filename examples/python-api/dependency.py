"""
Example computing the predicate dependency graph of a program.
"""
import os
from functools import singledispatchmethod

from clingo import ast
from clingo.core import Library


def rewrite(lib, prg):
    """
    Rewrite the given program.
    """
    prg_res = []
    prg_other = []
    params_const = []
    for stm in prg:
        if isinstance(stm, ast.StatementConst):
            params_const.append(stm.name)
            prg_res.append(stm)
        else:
            prg_other.append(stm)

    params = params_const
    for stm in prg_other:
        if isinstance(stm, ast.StatementProgram):
            params = params_const + stm.arguments
            prg_res.append(stm)
        else:
            prg_res.extend(ast.rewrite_statement(lib, stm, parameters=params))

    return prg_res


class DependencyBuilder:
    """
    Builder for the dependencies between predicates given by rules.
    """

    def __init__(self):
        self.graph = []
        self._head_predicates = []

    def _get_pred(self, lit):
        if isinstance(lit, ast.LiteralSymbolic):
            atom = lit.atom
            if isinstance(atom, ast.TermSymbolic):
                symbol = atom.symbol
                return [(symbol.name, symbol.arity, symbol.sign)]

            sign = False
            if isinstance(atom, ast.TermUnaryOperation):
                sign = True
                atom = atom.right
            return [(atom.name, len(atom.pool[0].arguments), sign)]
        return []

    @singledispatchmethod
    def _head(self, lit):
        assert False

    @_head.register
    def _(self, lit: ast.HeadSimpleLiteral):
        if isinstance(lit.literal, ast.LiteralSymbolic):
            self._head_predicates.extend(self._get_pred(lit.literal))

    @singledispatchmethod
    def _body(self, lit):
        return []

    @_body.register
    def _(self, lit: ast.BodySimpleLiteral):
        if isinstance(lit.literal, ast.LiteralSymbolic):
            return [
                (pred, lit.literal.sign != ast.Sign.NoSign)
                for pred in self._get_pred(lit.literal)
            ]
        return []

    def add(self, stm: ast.StatementRule):
        """
        Add dependencies for the given rule.
        """

        print("adding")
        print(" ", stm)
        self._head_predicates = []
        self._head(stm.head)
        for lit in stm.body:
            for head_pred in self._head_predicates:
                for body_pred, sign in self._body(lit):
                    self.graph.append((head_pred, body_pred, sign))

        print(" ", self._head_predicates)


def dependency(prg: list[ast.StatementRule]):
    """
    Compute the dependency graph of a program.
    """
    # TODO: add edges from body to head
    # {a} :- b.
    #   a :- b, not not a.
    #   for choice rules add edge (a, a, -)
    # {a: c} :- b.
    #   a :- b, c, not not a.
    #   for positive literal in condition add (a, c, +)
    # {a: c} > 1 :- b.
    #   {a: c} :- b.
    #   :- b, not {a: c} > 1.
    #   guards of head aggregates can be ignored
    bld = DependencyBuilder()
    for stm in prg:
        bld.add(stm)

    return bld.graph


def run():
    """
    Run the example.
    """
    with Library() as lib:
        files = [os.path.join(os.path.dirname(__file__), "example.lp")]
        with ast.Scanner(lib, files) as scanner:
            prg = list(scanner)

        prg = rewrite(lib, prg)

        dep = dependency([stm for stm in prg if isinstance(stm, ast.StatementRule)])

        print("dependency graph")
        for edge in dep:
            print(f"  {edge}")


run()
