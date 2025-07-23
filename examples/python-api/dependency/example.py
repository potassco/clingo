"""
Example computing the predicate dependency graph of a program.
"""

import sys
from dataclasses import dataclass, field
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


@dataclass
class PredicateNode:
    """
    A node capturing the outgoing edges of predicates.
    """

    name: Predicate
    neighbors: list[tuple["PredicateNode", bool]] = field(default_factory=list)
    visited: int = 0


@dataclass
class PredicateComponent:
    """
    A component containing a set of predicates.

    Parameters
    ----------
    predicates
        List of predicates in the component.
    depends
        List of *direct* dependencies.
    has_negative
        Whether the component contains a negative edge.
    is_domain
        Whether the component (transitively) depends on a component with a
        negative edge.
    """

    predicates: list[Predicate] = field(default_factory=list)
    depends: list[int] = field(default_factory=list)
    has_negative: bool = False
    is_domain: bool = True


class PredicateGraph:
    """
    The dependency graph of a program.
    """

    nodes_: dict[Predicate, PredicateNode]

    def __init__(self):
        self.nodes_ = {}

    def _add_node(self, a: Predicate) -> PredicateNode:
        return self.nodes_.setdefault(a, PredicateNode(a))

    def add_edge(self, a: Predicate, b: Predicate, sign: bool):
        """
        Add an edge between two predicates.
        """
        self._add_node(a).neighbors.append((self._add_node(b), sign))

    def _tarjan(self, start: PredicateNode, components: list[PredicateComponent]):
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
                        components.append(PredicateComponent())
                        while root:
                            y = t.pop()
                            y.visited = 1
                            components[-1].predicates.append(y.name)
                            root = x != y

    def analyze(self) -> list[PredicateComponent]:
        """
        Compute the strongly connected components of the graph.
        """
        components: list[PredicateComponent] = []
        for start in self.nodes_.values():
            if start.visited == 0:
                self._tarjan(start, components)

        occ: dict[Predicate, int] = {}
        for i, component in enumerate(components):
            predicates = set(component.predicates)
            component.predicates = list(sorted(predicates))

            depends = set()
            for pred in component.predicates:
                for node, sign in self.nodes_[pred].neighbors:
                    if node.name in predicates and sign:
                        component.has_negative = True
                        component.is_domain = False
                    j = occ.get(node.name, i)
                    if j != i:
                        depends.add(j)
                        if not components[j].is_domain:
                            component.is_domain = False
                occ[pred] = i
            component.depends = list(sorted(depends))

        return components

    def to_dot(self, components: list[PredicateComponent]) -> str:
        """
        Convert the graph with some extra information into dot format.
        """

        def p(pred):
            s = "-" if pred[2] else ""
            return f'"{s}{pred[0]}/{pred[1]}"'

        res = []
        res += ["digraph {"]
        res += ["  compound=true;"]
        for i, component in enumerate(components):
            res += [f"  subgraph cluster{i} {{"]
            res += [f'    label = "component {i}";']
            nodes = " ".join(p(pred) for pred in component.predicates)
            res += [f"    {nodes};"]
            res += ["  }"]
        for pred, node in self.nodes_.items():
            pos_edges = set()
            neg_edges = set()
            for x, sign in node.neighbors:
                if sign:
                    neg_edges.add(x.name)
                else:
                    pos_edges.add(x.name)

            nodes = ", ".join(p(x) for x in sorted(pos_edges - neg_edges))
            if nodes:
                res += [f"  {{ {nodes} }} -> {p(pred)} [style=solid];"]
            nodes = ", ".join(p(x) for x in sorted(neg_edges - pos_edges))
            if nodes:
                res += [f"  {{ {nodes} }} -> {p(pred)} [style=dotted];"]
            nodes = ", ".join(p(x) for x in sorted(pos_edges.intersection(neg_edges)))
            if nodes:
                res += [f"  {{ {nodes} }} -> {p(pred)} [style=dashed];"]

        for i, component in enumerate(components):
            start = ", ".join(p(x) for x in component.predicates)
            end = ", ".join(
                p(x) for j in component.depends for x in components[j].predicates
            )
            if start and end:
                res += ["  {"]
                res += ["    edge [style=invis];"]
                res += [f"    {{{end}}} -> {{{start}}};"]
                res += ["  }"]

        res += [f"  subgraph cluster{len(components)} {{"]
        res += ['    label = "Legend";']
        res += ['    node [shape="plaintext"];']
        res += [
            '    keys [label=<<table border="0" cellpadding="2" cellspacing="0" cellborder="0">'
        ]
        res += ['      <tr><td align="right">predicate</td><td>&#9675;</td></tr>']
        res += ['      <tr><td align="right">component</td><td>&#9633;</td></tr>']
        res += [
            '      <tr><td align="right" port="i1">positive edge</td><td>&#8594;</td></tr>'
        ]
        res += [
            '      <tr><td align="right" port="i2">negative edge</td><td>&#10513;</td></tr>'
        ]
        res += [
            '      <tr><td align="right" port="i3">postive and negative edge</td><td>&#10511;</td></tr>'
        ]
        res += ["      </table>>]"]
        res += ["  }"]
        res += ["}"]
        return "\n".join(res)


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
    opts = ast.RewriteContext(lib)
    for param in params:
        opts.add_param(param)
    for stm in prg_other:
        if isinstance(stm, ast.StatementProgram):
            params = params_const + stm.arguments
            opts.clear_params()
            for param in params:
                opts.add_param(param)
            prg_res.append(stm)
        else:
            prg_res.extend(ast.rewrite_statement(opts, stm))

    return prg_res


class DependencyBuilder:
    """
    Builder for the dependencies between predicates given by rules.
    """

    def __init__(self):
        self.graph = PredicateGraph()

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
        # Note: that here edges are added that only involve the conditionals in
        # the head. It would also be possible to add the predicates to the
        # result. Then, one could even change the interface to get the
        # predicates a rule provides/depends.
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
        head_preds = self._head(stm.head)
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
    Run the example and print a nice dot graph.
    """
    with Library() as lib:
        with ast.Scanner(lib, sys.argv[1:]) as scanner:
            prg = list(scanner)

        prg = rewrite(lib, prg)

        dep = dependency([stm for stm in prg if isinstance(stm, ast.StatementRule)])

        comps = dep.analyze()

        print(dep.to_dot(comps))


run()
