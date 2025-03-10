"""
This is a scaled down version of clingo-dl show casing how to implement a
propagator for difference logic.
"""

import heapq
import sys
from functools import singledispatchmethod
from typing import List, MutableMapping, Optional, Sequence, Set, Tuple, TypeVar, cast

from clingo import ast
from clingo.app import App, AppOptions, main
from clingo.base import TheoryTerm, TheoryTermType
from clingo.control import Control
from clingo.core import Library
from clingo.propagate import Assignment, PropagateControl, PropagateInit, Propagator
from clingo.solve import Model
from clingo.symbol import Function, Number, Symbol, SymbolType, Tuple_, parse_term

TheoryAtom = TypeVar("TheoryAtom", ast.BodyTheoryAtom, ast.HeadTheoryAtom)
Node = Symbol  # pylint: disable=invalid-name
Weight = int
Level = int
Edge = Tuple[Node, Node]
WeightedEdge = Tuple[Node, Node, Weight]
MapNodeWeight = MutableMapping[Node, Weight]

THEORY = """
#theory dl{
    diff_term {
    -  : 3, unary;
    ** : 2, binary, right;
    *  : 1, binary, left;
    /  : 1, binary, left;
    \\ : 1, binary, left;
    +  : 0, binary, left;
    -  : 0, binary, left
    };
    &__diff_h/0 : diff_term, {<=}, diff_term, head;
    &__diff_b/0 : diff_term, {<=}, diff_term, body
}.
"""

_BOP = {
    "+": lambda a, b: a + b,
    "-": lambda a, b: a - b,
    "*": lambda a, b: a * b,
    "**": lambda a, b: a**b,
    "\\": lambda a, b: a % b,
    "/": lambda a, b: a // b,
}


def _evaluate(lib: Library, term: TheoryTerm) -> Symbol:
    """
    Evaluates the operators in a theory term in the same fashion as clingo
    evaluates its arithmetic functions.
    """
    # tuples
    if term.type == TheoryTermType.Tuple:
        return Tuple_(lib, [_evaluate(lib, x) for x in term.arguments])

    # functions and arithmetic operations
    if term.type == TheoryTermType.Function:
        # binary operations
        if term.name in _BOP and len(term.arguments) == 2:
            term_a = _evaluate(lib, term.arguments[0])
            term_b = _evaluate(lib, term.arguments[1])

            if term_a.type != SymbolType.Number or term_b.type != SymbolType.Number:
                raise RuntimeError("Invalid Binary Operation")

            if term.name in ("/", "\\") and term_b.number == 0:
                raise RuntimeError("Division by Zero")

            return Number(lib, _BOP[term.name](term_a.number, term_b.number))

        # unary operations
        if term.name == "-" and len(term.arguments) == 1:
            term_a = _evaluate(lib, term.arguments[0])

            if term_a.type == SymbolType.Number:
                return Number(lib, -term_a.number)

            if term_a.type == SymbolType.Function and term_a.name:
                return Function(lib, term_a.name, term_a.arguments, term_a.sign)

            raise RuntimeError("Invalid Unary Operation")

        # functions
        return Function(lib, term.name, [_evaluate(lib, x) for x in term.arguments])

    # constants
    if term.type == TheoryTermType.Symbol:
        return Function(lib, term.name)

    # numbers
    if term.type == TheoryTermType.Number:
        return Number(lib, term.number)

    raise RuntimeError("Invalid Syntax")


class HeadBodyTransformer:
    """
    Transformer to tag head and body occurrences of `&diff` atoms.
    """

    def __init__(self, lib: Library):
        self._lib = lib

    @singledispatchmethod
    def _dispatch(self, expr):
        """
        Tag all theory atoms in the expression by their type.
        """
        return expr.transform(self._lib, self._dispatch)

    def _theory_atom(self, atom: TheoryAtom) -> TheoryAtom:
        """
        Tag the theory atom by its type.
        """
        name = atom.name
        if isinstance(name, ast.TermFunction):
            name = name.update(self._lib, name=f"__{name.name}_h")
        else:
            assert isinstance(name, ast.TermSymbolic)
            symbol = name.symbol
            name = name.update(
                self._lib,
                symbol=Function(
                    self._lib, f"__{symbol.name}_h", symbol.arguments, symbol.sign
                ),
            )
        return atom.update(self._lib, name=name)

    @_dispatch.register
    def _(self, atom: ast.BodyTheoryAtom) -> ast.BodyTheoryAtom:
        return self._theory_atom(atom)

    @_dispatch.register
    def _(self, atom: ast.HeadTheoryAtom) -> ast.HeadTheoryAtom:
        return self._theory_atom(atom)

    def __call__(self, files: Sequence[str]) -> ast.Program:
        """
        Parse the statements in the given files and output them in an orderly fashion.
        """
        prg = ast.Program(self._lib)
        prg.add(ast.parse_statement(self._lib, THEORY))
        with ast.Scanner(self._lib, list(files)) as scn:
            for stm in scn:
                prg.add(self._dispatch(stm) or stm)
        return prg


class Graph:
    """
    This class captures a graph with weighted edges that can be extended
    incrementally.

    Adding an edge triggers a cycle check that will report negative cycles.
    """

    _potential: MapNodeWeight
    _graph: MutableMapping[Node, MapNodeWeight]
    _gamma: MapNodeWeight
    _last_edges: MutableMapping[Node, WeightedEdge]
    _previous_edge: MutableMapping[Level, MutableMapping[Edge, Weight]]
    _previous_potential: MutableMapping[Level, MapNodeWeight]

    def __init__(self, lib: Library):
        self._lib = lib
        self._potential = {}  # {node: potential}
        self._graph = {}  # {node: {node : weight}}
        self._gamma = {}  # {node: gamma}
        self._last_edges = {}  # {node: edge}
        self._previous_edge = {}  # {level: {(node, node): weight}}
        self._previous_potential = {}  # {level: {node: potential}}

    @staticmethod
    def _set(level, key, val, previous, get_current):
        p = previous.setdefault(level, {})
        c, k = get_current(key)
        if key not in p:
            p[key] = c[k] if k in c else None
        c[k] = val

    @staticmethod
    def _reset(level, previous, get_current):
        if level in previous:
            for key, val in previous[level].items():
                c, k = get_current(key)
                if val is None:
                    del c[k]
                else:
                    c[k] = val
            del previous[level]

    def _reset_edge(self, level: Level):
        self._reset(
            level, self._previous_edge, lambda key: (self._graph[key[0]], key[1])
        )

    def _reset_potential(self, level: Level):
        self._reset(level, self._previous_potential, lambda key: (self._potential, key))

    def _set_edge(self, level: Level, key: Edge, val: Weight):
        self._set(
            level,
            key,
            val,
            self._previous_edge,
            lambda key: (self._graph[key[0]], key[1]),
        )

    def _set_potential(self, level: Level, key: Node, val: Weight):
        self._set(
            level,
            key,
            val,
            self._previous_potential,
            lambda key: (self._potential, key),
        )

    def add_edge(
        self, level: Level, edge: WeightedEdge
    ) -> Optional[List[WeightedEdge]]:
        """
        Add an edge to the graph and return a negative cycle (if there is one).
        """
        u, v, d = edge
        # If edge already exists from u to v with lower weight, new edge is redundant
        if u in self._graph and v in self._graph[u] and self._graph[u][v] <= d:
            return None

        # Initialize potential and graph
        if u not in self._potential:
            self._set_potential(level, u, 0)
        if v not in self._potential:
            self._set_potential(level, v, 0)
        self._graph.setdefault(u, {})
        self._graph.setdefault(v, {})

        changed: Set[Node] = set()  # Set of nodes for which potential has been changed
        min_gamma: List[Tuple[Weight, Node]] = []

        # Update potential change induced by new edge, 0 for other nodes
        self._gamma[u] = 0
        self._gamma[v] = self._potential[u] + d - self._potential[v]

        if self._gamma[v] < 0:
            heapq.heappush(min_gamma, (self._gamma[v], v))
            self._last_edges[v] = (u, v, d)

        # Propagate negative potential change
        while len(min_gamma) > 0 and self._gamma[u] == 0:
            _, s = heapq.heappop(min_gamma)
            if s not in changed:
                self._set_potential(level, s, self._potential[s] + self._gamma[s])
                self._gamma[s] = 0
                changed.add(s)
                for t in self._graph[s]:
                    if t not in changed:
                        gamma_t = (
                            self._potential[s] + self._graph[s][t] - self._potential[t]
                        )
                        if gamma_t < self._gamma[t]:
                            self._gamma[t] = gamma_t
                            heapq.heappush(min_gamma, (gamma_t, t))
                            self._last_edges[t] = (s, t, self._graph[s][t])

        cycle = None
        # Check if there is a negative cycle
        if self._gamma[u] < 0:
            cycle = []
            x, y, c = self._last_edges[v]
            cycle.append((x, y, c))
            while v != x:
                x, y, c = self._last_edges[x]
                cycle.append((x, y, c))
        else:
            self._set_edge(level, (u, v), d)

        # Ensure that all gamma values are zero
        self._gamma[v] = 0
        while len(min_gamma) > 0:
            _, s = heapq.heappop(min_gamma)
            self._gamma[s] = 0

        return cycle

    def get_assignment(self) -> List[Tuple[Node, Weight]]:
        """
        Get the current assignment to integer variables.
        """
        zero = Number(self._lib, 0)
        adjust = self._potential[zero] if zero in self._potential else 0
        return [
            (node, adjust - potential)
            for node, potential in self._potential.items()
            if node != zero
        ]

    def backtrack(self, level):
        """
        Backtrack the given level.
        """
        self._reset_edge(level)
        self._reset_potential(level)


class DLPropagator(Propagator):
    """
    A propagator for difference constraints.
    """

    _l2e: MutableMapping[int, List[WeightedEdge]]
    _e2l: MutableMapping[WeightedEdge, List[int]]
    _states: List[Graph]

    def __init__(self, lib: Library):
        super().__init__()
        self._lib = lib
        self._l2e = {}  # {literal: [(node, node, weight)]}
        self._e2l = {}  # {(node, node, weight): [literal]}
        self._states = []  # [Graph]

    def _add_edge(self, init: PropagateInit, lit: int, u: Node, v: Node, w: Weight):
        edge = (u, v, w)
        self._l2e.setdefault(lit, []).append(edge)
        self._e2l.setdefault(edge, []).append(lit)
        init.add_watch(lit)

    def init(self, init: PropagateInit):
        """
        Initialize the propagator extracting difference constraints from the
        theory data.
        """
        for atom in init.base.theory:
            term = atom.name
            if (term.name in ("__diff_h", "__diff_b")) and len(term.arguments) == 0:
                assert atom.guard is not None
                u = _evaluate(self._lib, atom.elements[0].tuple[0].arguments[0])
                v = _evaluate(self._lib, atom.elements[0].tuple[0].arguments[1])
                w = _evaluate(self._lib, atom.guard[1]).number
                lit = init.solver_literal(atom.literal)
                self._add_edge(init, lit, u, v, w)
                if term.name == "__diff_b":
                    self._add_edge(init, -lit, v, u, -w - 1)

    def propagate(self, control: PropagateControl, changes: Sequence[int]):
        """
        Add edges that became true to the graph to check for negative cycles.
        """
        state = self._state(control.thread_id)
        level = control.assignment.decision_level
        for lit in changes:
            for edge in self._l2e[lit]:
                cycle = state.add_edge(level, edge)
                if cycle is not None:
                    c = [self._literal(control, e) for e in cycle]
                    if control.add_nogood(c):
                        control.propagate()
                    return

    def undo(self, thread_id: int, assignment: Assignment, changes: Sequence[int]):
        """
        Backtrack the last decision level propagated.
        """
        # pylint: disable=unused-argument
        self._state(thread_id).backtrack(assignment.decision_level)

    def on_model(self, model: Model):
        """
        This function should be called when a model has been found to extend it
        with the integer variable assignments.
        """
        assignment = self._state(model.thread_id).get_assignment()
        model.extend(
            [
                Function(self._lib, "dl", [var, Number(self._lib, value)])
                for var, value in assignment
            ]
        )

    def _state(self, thread_id: int) -> Graph:
        while len(self._states) <= thread_id:
            self._states.append(Graph(self._lib))
        return self._states[thread_id]

    def _literal(self, control, edge):
        for lit in self._e2l[edge]:
            if control.assignment.is_true(lit):
                return lit
        raise RuntimeError("must not happen")


class DLApp(App):
    """
    Application extending clingo with difference constraints.
    """

    program_name: str = "clingo-dl"
    version: str = "1.0"

    _propagator: DLPropagator
    _minimize: Optional[Symbol]
    _bound: Optional[int]

    def __init__(self, lib: Library):
        super().__init__(DLApp.program_name, DLApp.version)
        self._lib = lib
        self._propagator = DLPropagator(lib)
        self._minimize = None
        self._bound = None

    def _parse_minimize(self, val):
        var = parse_term(self._lib, val)

        if var.type == SymbolType.Number:
            return False

        self._minimize = var
        return True

    def register_options(self, options: AppOptions):
        """
        Register application options.
        """
        group = "Clingo.DL Options"
        options.add(
            group,
            "minimize-variable",
            "Minimize the given variable",
            self._parse_minimize,
            argument="<var>",
        )

    def _on_model(self, model: Model):
        self._propagator.on_model(model)
        for symbol in model.symbols(theory=True):
            if symbol.match("dl", 2):
                n, v = symbol.arguments
                if n == self._minimize:
                    self._bound = v.number
                    break

    def main(self, control: Control, files: Sequence[str]) -> None:
        """
        Register the difference constraint propagator, and then ground and
        solve.
        """
        control.register_propagator(self._propagator)

        htb = HeadBodyTransformer(self._lib)
        control.join(htb(files))

        control.ground()
        if self._minimize is None:
            with control.solve(on_model=self._propagator.on_model) as hnd:
                hnd.get()
        else:
            # FIXME: parameters are not correctly replaced in theory atoms atm.
            control.parse_string("#program bound(b, v). &__diff_h { v-0 } <= b.")
            while True:
                with control.solve(on_model=self._on_model) as hnd:
                    if not hnd.get().satisfiable:
                        break
                print(f"Found new bound: {self._bound}")
                if self._bound is None:
                    break
                control.ground(
                    [
                        (
                            "bound",
                            [
                                Number(self._lib, cast(int, self._bound) - 1),
                                self._minimize,
                            ],
                        )
                    ]
                )

            if self._bound is not None:
                print("Optimum found")

    @staticmethod
    def run():
        """
        Run the application.
        """
        with Library() as lib:
            main(lib, sys.argv[1:], DLApp(lib))


if __name__ == "__main__":
    DLApp.run()
