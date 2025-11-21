"""
Functions and classes to observe or add ground statements.

Examples
--------
The first example shows how to add a fact to a program:

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function
>>> from clingo.control import Control
...
>>> lib = Library()
>>> ctl = Control(lib)
>>> sym = Function(lib, "a")
>>> with ctl.backend as bck:
...     atm_a = bck.atom(sym)
...     bck.rule([atm_a])
>>> ctl.base.is_fact(ctl.base[sym].literal)
True
>>> with ctl.solve(on_model=print) as hnd:
...     hnd.get()
a
SAT
```

The next example shows how to add theory atoms to a program:
```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function
>>> from clingo.control import Control
...
>>> lib = Library()
>>> ctl = Control(lib)
...
>>> with ctl.backend as bck:
...     n = Function(lib, "p")
...     a = bck.theory_string("a")
...     o = bck.theory_number(1)
...     f = bck.theory_function("f", [a, o])
...     e = bck.theory_element([f], [])
...     bck.theory_atom(0, n, [e])
...
>>> print(ctl.base.theory[0])
&p { f(a,1) }
```
"""

from __future__ import annotations

import enum
import typing

import clingo.base
import clingo.symbol

__all__ = [
    "Backend",
    "BackendManager",
    "ExternalType",
    "HeuristicType",
    "Observer",
    "TheorySequenceType",
]

class ExternalType(enum.IntEnum):
    """
    Available external types.
    """

    False_: typing.ClassVar[ExternalType]  # value = <ExternalType.False_: 2>
    Free: typing.ClassVar[ExternalType]  # value = <ExternalType.Free: 0>
    Release: typing.ClassVar[ExternalType]  # value = <ExternalType.Release: 3>
    True_: typing.ClassVar[ExternalType]  # value = <ExternalType.True_: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class HeuristicType(enum.IntEnum):
    """
    Available heuristic types.
    """

    Factor: typing.ClassVar[HeuristicType]  # value = <HeuristicType.Factor: 2>
    False_: typing.ClassVar[HeuristicType]  # value = <HeuristicType.False_: 5>
    Init: typing.ClassVar[HeuristicType]  # value = <HeuristicType.Init: 3>
    Level: typing.ClassVar[HeuristicType]  # value = <HeuristicType.Level: 0>
    Sign: typing.ClassVar[HeuristicType]  # value = <HeuristicType.Sign: 1>
    True_: typing.ClassVar[HeuristicType]  # value = <HeuristicType.True_: 4>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class TheorySequenceType(enum.IntEnum):
    """
    Available theory sequence types.
    """

    List: typing.ClassVar[TheorySequenceType]  # value = <TheorySequenceType.List: 2>
    Set: typing.ClassVar[TheorySequenceType]  # value = <TheorySequenceType.Set: 1>
    Tuple: typing.ClassVar[TheorySequenceType]  # value = <TheorySequenceType.Tuple: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Backend:
    """
    Provides an interface to extend a logic program.

    This class offers methods to add various types of rules, set external atoms,
    specify heuristics, define optimization statements, and extend the underlying
    theory. It allows for low-level manipulation of logic programs.

    See Also:
        clingo.control.Control.backend
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def assume(self, literals: typing.Sequence[int | typing.SupportsIndex]) -> None:
        """
        Add an assumption directive to the solver.

        Specifies literals to be assumed true or false for the next solving call.
        Positive literals are treated as true, while negative literals are treated
        as false. These assumptions apply only to the next solve call.

        Args:
            literals: Sequence of program literals to assume.
        """

    def atom(self, symbol: clingo.symbol.Symbol | None = None) -> int:
        """
        Return a fresh program atom or the atom associated with the given symbol.

        If the given symbol does not exist in the atom base, it is added first. These
        atoms will be used in subsequent calls to ground for instantiation.

        Args:
                symbol: The symbol associated with the atom.

        Returns:
                The program atom representing the atom.
        """

    def edge(
        self,
        node_u: int | typing.SupportsIndex,
        node_v: int | typing.SupportsIndex,
        condition: typing.Sequence[int | typing.SupportsIndex],
    ) -> None:
        """
        Add an edge directive.

        Adds an edge from `node_u` to `node_v` to the graph. The edge is subject to the
        specified condition.

        Args:
            node_u: The start node of the edge.
            node_v: The end node of the edge.
            condition: Sequence of literals representing the edge condition.
        """

    def external(self, atom: int | typing.SupportsIndex, type: ExternalType) -> None:
        """
        Add an external directive.

        Declares an atom as external and sets its truth value according to the
        specified type. External atoms can be used as assumptions or for incremental
        solving. The special value `ExternalType.Release` can be used to permanently
        set an external atom to false.

        Args:
            atom: The external atom (must be a positive literal).
            type: The type determining the truth value of the atom.
        """

    def heuristic(
        self,
        atom: int | typing.SupportsIndex,
        type: HeuristicType,
        weight: int | typing.SupportsIndex,
        priority: int | typing.SupportsIndex = 0,
        condition: typing.Sequence[int | typing.SupportsIndex] = [],
    ) -> None:
        """
        Add a heuristic directive for an atom.

        Adds a heuristic directive for the given atom, influencing the solver's search
        process. The directive's effect depends on its type, weight, and priority. The
        condition, if provided, determines when the heuristic should be applied.

        Args:
                atom: The atom to which the heuristic applies.
                type: The type of the heuristic.
                weight: The weight of the heuristic.
                priority: The priority of the heuristic (default: 0).
                condition: Sequence of literals representing the condition (default: []).
        """

    def minimize(
        self,
        literals: typing.Sequence[tuple[int, int]],
        priority: int | typing.SupportsIndex = 0,
    ) -> None:
        """
        Add a minimize constraint to the program.

        Adds a minimize constraint, which instructs the solver to minimize the sum of
        weights of true literals in the given sequence. Multiple minimize constraints
        with different priorities can be added, with lower priority values considered
        more important.

        Args:
                literals: Sequence of (literal, weight) tuples to minimize.
                priority: Priority of the constraint (default: 0).
        """

    def project(self, atoms: typing.Sequence[int | typing.SupportsIndex]) -> None:
        """
        Add a projection directive to the program.

        Specifies which atoms should be considered in stable models. When projection
        is used, stable models are treated as equivalent if they agree on the truth
        values of projected atoms, regardless of other atoms in the model.

        Args:
                atoms: Sequence of atoms to project on.
        """

    def rule(
        self,
        head: typing.Sequence[int | typing.SupportsIndex],
        body: typing.Sequence[int | typing.SupportsIndex] = [],
        choice: bool = False,
    ) -> None:
        """
        Add a rule to the program.

        Creates a disjunctive or choice rule based on the `choice` parameter. The
        head and body are specified as sequences of literals.

        Rule types based on head length (when choice is False):
        - Empty head: Integrity constraint
        - Single literal head: Normal rule
        - Multiple literals head: Disjunctive rule

        Args:
                head: Sequence of literals in the rule head.
                body: Sequence of literals in the rule body (default: []).
                choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
        """

    def theory_atom(
        self,
        atom: int | typing.SupportsIndex | None,
        name: clingo.symbol.Symbol,
        elements: typing.Sequence[int | typing.SupportsIndex],
        guard: tuple[str, int | typing.SupportsIndex] | None = None,
    ) -> int:
        """
        Create a theory atom and return its associated program atom.

        If the theory atom does not exist yet, the method assigns a program atom based
        on these rules:
        - If a specific atom value is provided, that value is used.
        - If None is given, a fresh program atom is introduced.
        - For program directives, value zero is used.

        Args:
                atom: The program atom to assign, or None to assign a fresh one.
                name: The name of the theory atom.
                elements: A sequence of ids representing theory elements.
                guard: A tuple containing the guard operator and the id of the guard term, or None if there is no guard.

        Returns:
                The program atom associated with the created theory atom.
        """

    def theory_element(
        self,
        tuple: typing.Sequence[int | typing.SupportsIndex],
        condition: typing.Sequence[int | typing.SupportsIndex],
    ) -> int:
        """
        Create a theory element.

        The method creates a theory element consisting of a tuple and a condition, and
        returns its unique identifier. This id can be used in other theory-related
        methods to reference this element.

        Args:
                tuple: A sequence of ids representing theory terms.
                condition: A sequence of program literals.

        Returns:
                The unique id of the created theory element.
        """

    def theory_function(
        self, name: str, arguments: typing.Sequence[int | typing.SupportsIndex]
    ) -> int:
        """
        Create a function theory term.

        The method creates a function theory term with the given name and arguments,
        and returns its unique identifier. This id can be used in other theory-related
        methods to reference this term.

        Args:
                name: The name of the function.
                arguments: A sequence of ids of theory terms.

        Returns:
                The unique id of the created function theory term.
        """

    def theory_number(self, number: int | typing.SupportsIndex) -> int:
        """
        Create a numeric theory term.

        The function creates a numeric theory term with the given value and return its
        unique identifier. This id can be used in other theory-related methods to
        reference this term.

        Args:
            number: The numeric value of the theory term to create.

        Returns:
                The unique id of the created theory term.
        """

    def theory_sequence(
        self,
        type: TheorySequenceType,
        elements: typing.Sequence[int | typing.SupportsIndex],
    ) -> int:
        """
        Create a sequence theory term.

        The method creates a sequence theory term of the specified type, containing the
        given elements, and returns its unique identifier. This id can be used in other
        theory-related methods to reference this term.

        Args:
                type: The type of the sequence (e.g., tuple, list, set).
                elements: A sequence of ids representing theory terms.

        Returns:
                The unique id of the created sequence theory term.
        """

    def theory_string(self, string: str) -> int:
        """
        Create a string theory term.

        The method creates a string theory term with the given value and returns its
        unique identifier. This id can be used in other theory-related methods to
        reference this term.

        Args:
                string: The string value of the theory term to create.

        Returns:
                The unique id of the created string theory term.
        """

    def theory_symbol(self, symbol: clingo.symbol.Symbol) -> int:
        """
        Convert a symbol into a theory term.

        The method converts the given symbol into a theory term and returns its unique
        identifier. This id can be used in other theory-related methods to reference
        this term.

        Args:
                symbol: The symbol to create the theory term from.

        Returns:
                The unique id of the created theory term.
        """

    def weight_rule(
        self,
        head: typing.Sequence[int | typing.SupportsIndex],
        lower_bound: int | typing.SupportsIndex,
        body: typing.Sequence[tuple[int, int]],
        choice: bool = False,
    ) -> None:
        """
        Add a weight rule to the program.

        Adds a weight rule, where the body is a weight constraint. The head is either a
        disjunction ro choice (see Backend.rule for more details).

        A weight constraint is satisfied if the sum of weights of the true literals
        meets or exceeds the lower bound.

        Args:
                head: Sequence of literals in the rule head.
                lower_bound: The lower bound of the weight constraint.
                body: Sequence of (literal, weight) tuples forming the weight constraint.
                choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
        """

class BackendManager:
    """
    A context manager to initialize and finalize a backend.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __enter__(self) -> Backend:
        """
        Initialize backend the backend.
        """

    def __exit__(
        self, type: type | None, value: typing.Any | None, traceback: typing.Any | None
    ) -> None:
        """
        Finalize the backend.
        """

class Observer:
    """

    ABC to inspect aspif directives.

    Not all functions of the interface have to be implemented and can be omitted if
    not needed.

    See Also:
        `clingo.control.Control.observe`
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __init__(self) -> None: ...
    def assume(self, literals: typing.Sequence[int | typing.SupportsIndex]) -> None:
        """
        Called for assumptions in the solver.

        See also `Backend.assume`.

        Args:
            literals: Sequence of program literals to assume.
        """

    def begin_step(self) -> None:
        """
        Called at the beginning of a step.
        """

    def edge(
        self,
        node_u: int | typing.SupportsIndex,
        node_v: int | typing.SupportsIndex,
        condition: typing.Sequence[int | typing.SupportsIndex],
    ) -> None:
        """
        Called for edge directives in the program.

        See also `Backend.edge`.

        Args:
            node_u: The start node of the edge.
            node_v: The end node of the edge.
            condition: Sequence of literals representing the edge condition.
        """

    def end_step(self, base: clingo.base.Base) -> None:
        """
        Called at the end of a step.
        """

    def external(self, atom: int | typing.SupportsIndex, type: ExternalType) -> None:
        """
        Called for external directives in the program.

        See also `Backend.external`.

        Args:
            atom: The external atom (must be a positive literal).
            type: The type determining the truth value of the atom.
        """

    def heuristic(
        self,
        atom: int | typing.SupportsIndex,
        type: HeuristicType,
        weight: int | typing.SupportsIndex,
        priority: int | typing.SupportsIndex,
        condition: typing.Sequence[int | typing.SupportsIndex],
    ) -> None:
        """
        Called for heuristic directives in the program.

        See also `Backend.heuristic`.

        Args:
                atom: The atom to which the heuristic applies.
                type: The type of the heuristic.
                weight: The weight of the heuristic.
                priority: The priority of the heuristic (default: 0).
                condition: Sequence of literals representing the condition (default: []).
        """

    def init_program(self, incremental: bool) -> None:
        """
        The first directive in a program.

        The parameter `incremental` indicates whether the program can consist of more
        than one step.

        Args:
                incremental: Whether the program is incremental.
        """

    def minimize(
        self,
        literals: typing.Sequence[tuple[int, int]],
        priority: int | typing.SupportsIndex,
    ) -> None:
        """
        Called for minimize constraints in the program.

        See also `Backend.minimize`.

        Args:
                literals: Sequence of (literal, weight) tuples to minimize.
                priority: Priority of the constraint (default: 0).
        """

    def project(self, atoms: typing.Sequence[int | typing.SupportsIndex]) -> None:
        """
        Called for projection directives in the program.

        See also `Backend.project`.

        Args:
                atoms: Sequence of atoms to project on.
        """

    def rule(
        self,
        head: typing.Sequence[int | typing.SupportsIndex],
        body: typing.Sequence[int | typing.SupportsIndex],
        choice: bool,
    ) -> None:
        """
        Called for rules in the program.

        See also `Backend.rule`.

        Args:
                head: Sequence of literals in the rule head.
                body: Sequence of literals in the rule body (default: []).
                choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
        """

    def weight_rule(
        self,
        head: typing.Sequence[int | typing.SupportsIndex],
        lower_bound: int | typing.SupportsIndex,
        body: typing.Sequence[tuple[int, int]],
        choice: bool,
    ) -> None:
        """
        Called for weight rules in the program.

        See also `Backend.weight_rule`.

        Args:
                head: Sequence of literals in the rule head.
                lower_bound: The lower bound of the weight constraint.
                body: Sequence of (literal, weight) tuples forming the weight constraint.
                choice: If True, adds a choice rule; otherwise, a disjunctive rule (default: False).
        """
