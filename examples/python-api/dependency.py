"""
Example computing the predicate dependency graph of a program.
"""
import os
from functools import singledispatchmethod
from typing import Union

from clingo import ast
from clingo.core import Library

# these variants will be added to the ast module
Literal = Union[ast.LiteralBoolean, ast.LiteralComparison, ast.LiteralSymbolic]
HeadLiteral = Union[
    ast.HeadAggregate,
    ast.HeadDisjunction,
    ast.HeadSetAggregate,
    ast.HeadSimpleLiteral,
    ast.HeadTheoryAtom,
]
BodyLiteral = Union[
    ast.BodyAggregate,
    ast.BodyConditionalLiteral,
    ast.BodySetAggregate,
    ast.BodySimpleLiteral,
    ast.BodyTheoryAtom,
]
Statement = Union[
    ast.StatementComment,
    ast.StatementConst,
    ast.StatementDefined,
    ast.StatementEdge,
    ast.StatementExternal,
    ast.StatementHeuristic,
    ast.StatementInclude,
    ast.StatementOptimize,
    ast.StatementProgram,
    ast.StatementProject,
    ast.StatementProjectSignature,
    ast.StatementRule,
    ast.StatementScript,
    ast.StatementShow,
    ast.StatementShowSignature,
    ast.StatementTheory,
    ast.StatementWeakConstraint,
]
Program = list[Statement]
Predicate = tuple[str, int, bool]


def rewrite(lib: Library, prg: Program) -> Program:
    """
    Rewrite the given program.
    """
    prg_res: list[Statement] = []
    prg_other: list[Statement] = []
    params_const: list[str] = []
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
        self.graph = set()

    def _get_pred(self, lit: Literal):
        if isinstance(lit, ast.LiteralSymbolic):
            atom = lit.atom
            if isinstance(atom, ast.TermSymbolic):
                symbol = atom.symbol
                return [(symbol.name, symbol.arity, symbol.sign)]

            sign = False
            if isinstance(atom, ast.TermUnaryOperation):
                sign = True
                atom = atom.right
            assert isinstance(atom, ast.TermFunction)

            return [(atom.name, len(atom.pool[0].arguments), sign)]
        return []

    def _get_body_pred(self, lit: Literal):
        return [(pred, lit.sign != ast.Sign.NoSign) for pred in self._get_pred(lit)]

    @singledispatchmethod
    def _head(self, lit) -> list[Predicate]:
        _ = lit
        return []

    @_head.register
    def _(self, lit: ast.HeadSimpleLiteral) -> list[Predicate]:
        if isinstance(lit.literal, ast.LiteralSymbolic):
            return self._get_pred(lit.literal)
        return []

    @_head.register(ast.HeadDisjunction)
    @_head.register(ast.HeadAggregate)
    def _(self, lit: Union[ast.HeadDisjunction, ast.HeadAggregate]) -> list[Predicate]:
        res = []
        for elem in lit.elements:
            if isinstance(elem, (ast.HeadConditionalLiteral, ast.HeadAggregateElement)):
                head_preds = self._get_pred(elem.literal)
                for slit in elem.condition:
                    for body_pred, sign in self._body(slit):
                        for head_pred in head_preds:
                            self.graph.add((head_pred, body_pred, sign))
            else:
                head_preds = self._get_pred(elem)
            for head_pred in head_preds:
                self.graph.add((head_pred, head_pred, True))
            res.extend(head_preds)
        return res

    @singledispatchmethod
    def _body(self, lit: BodyLiteral) -> list[tuple[Predicate, bool]]:
        _ = lit
        return []

    @_body.register
    def _(self, lit: ast.BodySimpleLiteral) -> list[tuple[Predicate, bool]]:
        return self._get_body_pred(lit.literal)

    @_body.register
    def _(self, lit: ast.BodyTheoryAtom) -> list[tuple[Predicate, bool]]:
        res = []
        for elem in lit.elements:
            for slit in elem.condition:
                res.extend(self._get_body_pred(slit))
        return res

    @_body.register
    def _(self, lit: ast.BodyConditionalLiteral) -> list[tuple[Predicate, bool]]:
        raise RuntimeError("implement me!!!")

    @_body.register
    def _(self, lit: ast.BodyAggregate) -> list[tuple[Predicate, bool]]:
        raise RuntimeError("implement me!!!")

    def add(self, stm: ast.StatementRule):
        """
        Add dependencies for the given rule.
        """
        print("adding")
        print(" ", stm)
        head_preds = self._head(stm.head)
        print(" ", head_preds)
        for lit in stm.body:
            for head_pred in head_preds:
                for body_pred, sign in self._body(lit):
                    self.graph.add((head_pred, body_pred, sign))


def dependency(prg: list[ast.StatementRule]):
    """
    Compute the dependency graph of a program.
    """
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
        for edge in sorted(dep):
            print(f"  {edge}")


run()
