"""
Example computing the predicate dependency graph of a program.
"""
import os
from functools import singledispatchmethod
from typing import Optional, Union

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


class Node:
    """
    A node capturing the outgoing edges of predicates.
    """

    neighbors: list[tuple["Node", bool]]
    name: Predicate
    visited: int

    def __init__(self, name):
        self.neighbors = []
        self.name = name
        self.visited = 0


class Component:
    """
    A component containing a set of predicates.
    """

    predicates: list[Predicate]

    def __init__(self):
        self.predicates = []


class Graph:
    """
    The dependency graph of a program.
    """

    nodes_: dict[Predicate, Node]

    def __init__(self):
        self.nodes_ = {}

    def _add_node(self, a: Predicate) -> Node:
        return self.nodes_.setdefault(a, Node(a))

    def add_edge(self, a: Predicate, b: Predicate, sign: bool):
        """
        Add an edge between two predicates.
        """
        self._add_node(a).neighbors.append((self._add_node(b), sign))

    def _tarjan(self, start: Node, sccs: list[Component]):
        s = []
        t = []

        visited = 2
        s.append(start)

        while s:
            x = s[-1]
            if x.visited == 0:
                x.visited = visited
                visited += 1
                t.append(x)
                for y, _ in x.neighbors:
                    if y.visited == 0:
                        s.append(y)
            else:
                s.pop()
                if x.visited > 1:
                    root = True
                    for y, _ in x.neighbors:
                        if y.visited > 1 and y.visited < x.visited:
                            root = False
                            x.visited = y.visited
                    if root:
                        sccs.append(Component())
                        while root:
                            y = t.pop()
                            y.visited = 1
                            sccs[-1].predicates.append(y.name)
                            root = x != y

    def analyze(self) -> list[Component]:
        """
        Compute the strongly connected components of the graph.
        """

        sccs: list[Component] = []
        for start in self.nodes_.values():
            if start.visited != 0:
                self._tarjan(start, sccs)

        # TODO: add some extra info to scc

        return sccs


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
        self.graph = Graph()

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

    def _get_body_pred(self, lit: Literal, force_negative=False):
        res = [(pred, lit.sign != ast.Sign.NoSign) for pred in self._get_pred(lit)]
        if force_negative:
            res += [(pred, True) for pred, sign in res if not sign]
        return res

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
                            self.graph.add_edge(head_pred, body_pred, sign)
            else:
                head_preds = self._get_pred(elem)
            for head_pred in head_preds:
                self.graph.add_edge(head_pred, head_pred, True)
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
                res.extend(self._get_body_pred(slit, True))
        return res

    @_body.register
    def _(self, lit: ast.BodyConditionalLiteral) -> list[tuple[Predicate, bool]]:
        res = self._get_body_pred(lit.literal)
        for slit in lit.condition:
            res.extend(self._get_body_pred(slit, True))
        return res

    def _is_monotone(
        self,
        left: Optional[ast.LeftGuard],
        fun: ast.AggregateFunction,
        right: Optional[ast.RightGuard],
    ) -> bool:
        if fun != ast.AggregateFunction.Sum:
            rel_left = (ast.Relation.Less, ast.Relation.LessEqual)
            rel_right = (ast.Relation.Greater, ast.Relation.GreaterEqual)
            if fun == ast.AggregateFunction.Min:
                rel_left, rel_right = rel_right, rel_left
            return (not left or left.relation in rel_left) and (
                not right or right.relation in rel_right
            )
        return False

    @_body.register
    def _(self, lit: ast.BodyAggregate) -> list[tuple[Predicate, bool]]:
        res = []
        force_negative = not self._is_monotone(lit.left, lit.function, lit.right)
        for elem in lit.elements:
            for slit in elem.condition:
                res.extend(self._get_body_pred(slit, force_negative))
        return res

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
                    self.graph.add_edge(head_pred, body_pred, sign)


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

        sccs = dep.analyze()

        for scc in sccs:
            print("scc:")
            for pred in scc.predicates:
                print(f"  {pred}")


run()
