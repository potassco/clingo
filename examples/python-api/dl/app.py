"""
This is a scaled down version of clingo-dl show casing how to implement a
propagator for difference logic.
"""

import gc
import heapq
import sys
from enum import StrEnum
from functools import singledispatch
from itertools import filterfalse
from typing import (
    Callable,
    List,
    MutableMapping,
    Optional,
    Sequence,
    Set,
    Tuple,
    TypeVar,
)

from clingo import ast
from clingo.app import App, AppOptions, clingo_main
from clingo.base import TheoryTerm, TheoryTermType
from clingo.control import Control, ControlMode
from clingo.core import Library
from clingo.propagate import Assignment, PropagateControl, PropagateInit, Propagator
from clingo.solve import Model
from clingo.symbol import Function, Number, Symbol, SymbolType, Tuple_, parse_term

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


class ErrorMessage(StrEnum):
    """
    Available error messages.
    """

    INVALID_BINARY_OP = "Invalid Binary Operation"
    DIVISION_BY_ZERO = "Division by Zero"
    INVALID_UNARY_OP = "Invalid Unary Operation"
    INVALID_SYNTAX = "Invalid Syntax"


_BOP: dict[str, Callable[[int, int], int]] = {
    "+": lambda a, b: a + b,
    "-": lambda a, b: a - b,
    "*": lambda a, b: a * b,
    "/": lambda a, b: a // b,
    "\\": lambda a, b: a % b,
}


def _evaluate_unary_op(lib: Library, term: TheoryTerm) -> Symbol:
    term_a = _evaluate(lib, term.arguments[0])

    if term_a.type == SymbolType.Number:
        return Number(lib, -term_a.number)

    if term_a.type == SymbolType.Function and term_a.name:
        return Function(lib, term_a.name, term_a.arguments, term_a.is_negative)

    raise RuntimeError(ErrorMessage.INVALID_UNARY_OP)


def _evaluate_binary_op(lib: Library, term: TheoryTerm) -> Symbol:
    term_a = _evaluate(lib, term.arguments[0])
    term_b = _evaluate(lib, term.arguments[1])
    if term_a.type != SymbolType.Number or term_b.type != SymbolType.Number:
        raise RuntimeError(ErrorMessage.INVALID_BINARY_OP)
    if term.name in ("/", "\\") and term_b.number == 0:
        raise RuntimeError(ErrorMessage.DIVISION_BY_ZERO)
    return Number(lib, _BOP[term.name](term_a.number, term_b.number))


def _evaluate(lib: Library, term: TheoryTerm) -> Symbol:
    """
    Evaluates the operators in a theory term in the same fashion as clingo
    evaluates its arithmetic functions.
    """
    match term.type:
        case TheoryTermType.Number:
            return Number(lib, term.number)
        case TheoryTermType.Symbol:
            return Function(lib, term.name)
        case TheoryTermType.Tuple:
            return Tuple_(lib, [_evaluate(lib, x) for x in term.arguments])
        case TheoryTermType.Function:
            if term.name in _BOP and len(term.arguments) == 2:
                return _evaluate_binary_op(lib, term)
            if term.name == "-" and len(term.arguments) == 1:
                return _evaluate_unary_op(lib, term)
            return Function(lib, term.name, [_evaluate(lib, x) for x in term.arguments])
        case _:
            raise RuntimeError(ErrorMessage.INVALID_SYNTAX)


def rewrite(lib: Library, files: Sequence[str]) -> ast.Program:
    """
    Transformer to tag head and body occurrences of `&diff` atoms.
    """
    TheoryAtom = TypeVar("TheoryAtom", ast.BodyTheoryAtom, ast.HeadTheoryAtom)

    def tag(atom: TheoryAtom) -> TheoryAtom:
        """
        Tag the theory atom by its type.
        """
        name = atom.name
        if isinstance(name, ast.TermFunction):
            name = name.update(lib, name=f"__{name.name}_h")
        else:
            assert isinstance(name, ast.TermSymbolic)
            symbol = name.symbol
            name = name.update(
                lib,
                symbol=Function(
                    lib, f"__{symbol.name}_h", symbol.arguments, symbol.is_positive
                ),
            )
        return atom.update(lib, name=name)

    @singledispatch
    def accept(expr):
        return expr.transform(lib, accept)

    @accept.register
    def _(atom: ast.BodyTheoryAtom) -> ast.BodyTheoryAtom:
        return tag(atom)

    @accept.register
    def _(atom: ast.HeadTheoryAtom) -> ast.HeadTheoryAtom:
        return tag(atom)

    prg = ast.Program(lib)
    prg.add(ast.parse_statement(lib, THEORY))
    ast.parse_files(lib, files, lambda stm: prg.add(accept(stm) or stm))
    return prg


class Graph:
    # pylint: disable=too-many-instance-attributes
    """
    This class captures a graph with weighted edges that can be extended
    incrementally.

    Adding an edge triggers a cycle check that will report negative cycles.
    """

    _lib: Library
    _potential: MapNodeWeight
    _graph: MutableMapping[Node, MapNodeWeight]
    _gamma: MapNodeWeight
    _last_edges: MutableMapping[Node, WeightedEdge]
    _previous_edge: MutableMapping[Level, MutableMapping[Edge, Weight]]
    _previous_potential: MutableMapping[Level, MapNodeWeight]
    _changed: Set[Node]
    _min_gamma: List[Tuple[Weight, Node]]

    def __init__(self, lib: Library):
        self._lib = lib
        self._potential = {}  # {node: potential}
        self._graph = {}  # {node: {node : weight}}
        self._gamma = {}  # {node: gamma}
        self._last_edges = {}  # {node: edge}
        self._previous_edge = {}  # {level: {(node, node): weight}}
        self._previous_potential = {}  # {level: {node: potential}}
        self._changed = set()  # {node}
        self._min_gamma = []  # [(weight, node)]

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

    def _pop_changed(self):
        """
        Advance to the next node that needs processing.
        """
        while self._min_gamma and self._min_gamma[0][1] in self._changed:
            heapq.heappop(self._min_gamma)
        return bool(self._min_gamma)

    def _init_check(self, level: Level, u: Node, v: Node, d: Weight):
        """
        Initialize the potentials and gammas of of nodes `u` and `v`.
        """
        if u not in self._potential:
            self._set_potential(level, u, 0)
        if v not in self._potential:
            self._set_potential(level, v, 0)
        self._gamma[u] = 0
        self._gamma[v] = self._potential[u] + d - self._potential[v]
        self._graph.setdefault(u, {})
        self._graph.setdefault(v, {})

        # enqueue v if its potential became negative
        if self._gamma[v] < 0:
            heapq.heappush(self._min_gamma, (self._gamma[v], v))
            self._last_edges[v] = (u, v, d)

    def _extract_cycle(
        self, level: Level, u: Node, v: Node, d: Weight
    ) -> Optional[List[WeightedEdge]]:
        """
        Check if there is a negative cycle.
        """
        # reset gammas
        has_cycle = self._gamma[u] < 0
        self._gamma[v] = 0
        while self._min_gamma:
            _, s = heapq.heappop(self._min_gamma)
            self._gamma[s] = 0
        self._changed.clear()

        # extract cycle
        if has_cycle:
            cycle = []
            x, y, c = self._last_edges[v]
            cycle.append((x, y, c))
            while v != x:
                x, y, c = self._last_edges[x]
                cycle.append((x, y, c))
            return cycle

        # add edge that did not introduce a cycle
        self._set_edge(level, (u, v), d)
        return None

    def add_edge(
        self, level: Level, edge: WeightedEdge
    ) -> Optional[List[WeightedEdge]]:
        """
        Add an edge to the graph and return a negative cycle (if there is one).
        """
        u, v, d = edge
        # prune redundant edges
        if u in self._graph and v in self._graph[u] and self._graph[u][v] <= d:
            return None

        self._init_check(level, u, v, d)

        # propagate negative potential changes
        while self._pop_changed() and self._gamma[u] == 0:
            _, s = heapq.heappop(self._min_gamma)
            self._set_potential(level, s, self._potential[s] + self._gamma[s])
            self._gamma[s] = 0
            self._changed.add(s)
            for t in filterfalse(self._changed.__contains__, self._graph[s]):
                gamma_t = self._potential[s] + self._graph[s][t] - self._potential[t]
                if gamma_t < self._gamma[t]:
                    self._gamma[t] = gamma_t
                    heapq.heappush(self._min_gamma, (gamma_t, t))
                    self._last_edges[t] = (s, t, self._graph[s][t])

        return self._extract_cycle(level, u, v, d)

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

    _lib: Library
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

    def init(self, assignment: Assignment, init: PropagateInit):
        """
        Initialize the propagator extracting difference constraints from the
        theory data.
        """
        assert assignment
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

    def propagate(
        self, assignment: Assignment, control: PropagateControl, changes: Sequence[int]
    ):
        """
        Add edges that became true to the graph to check for negative cycles.
        """
        state = self._state(assignment.thread_id)
        level = assignment.decision_level
        for lit in changes:
            for edge in self._l2e[lit]:
                cycle = state.add_edge(level, edge)
                if cycle is not None:
                    c = [self._literal(assignment, e) for e in cycle]
                    if control.add_nogood(c):
                        control.propagate()
                    return

    def undo(self, assignment: Assignment, changes: Sequence[int]):
        """
        Backtrack the last decision level propagated.
        """
        assert changes
        self._state(assignment.thread_id).backtrack(assignment.decision_level)

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

    def _literal(self, assignment: Assignment, edge: WeightedEdge):
        for lit in self._e2l[edge]:
            if assignment.is_true(lit):
                return lit
        assert False


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
        try:
            var = parse_term(self._lib, val)
        except RuntimeError as exe:
            raise ValueError("invalid minimize") from exe

        if var.type == SymbolType.Number:
            raise ValueError("invalid minimize")

        self._minimize = var

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

    def main(
        self,
        control: Control,
        files: Sequence[str],
    ) -> None:
        """
        Register the difference constraint propagator, and then ground and
        solve.
        """
        control.register_propagator(self._propagator)
        control.join(rewrite(self._lib, files))

        if control.mode in (ControlMode.Parse, ControlMode.Rewrite):
            control.main()
        elif self._minimize is None:
            control.ground(control.parts)
            with control.solve(on_model=self._on_model) as hnd:
                hnd.get()
        else:
            control.ground(control.parts)
            control.parse_string("#program bound(b, v). &__diff_h { v-0 } <= b.")
            while True:
                with control.solve(on_model=self._on_model) as hnd:
                    if not hnd.get().satisfiable:
                        break
                if self._bound is None:
                    break
                print(f"Found new bound: {self._bound}")
                num = Number(self._lib, self._bound - 1)
                control.ground([("bound", [num, self._minimize])])

            if self._bound is not None:
                print("Optimum found")

    @staticmethod
    def run():
        """
        Run the application.
        """
        with Library() as lib:
            clingo_main(lib, sys.argv[1:], DLApp(lib))
            gc.collect()  # optional: ensures all symbols are collected


if __name__ == "__main__":
    DLApp.run()
