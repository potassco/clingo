"""
Functions and classes to implement custom propagators.

# Example

```python
>>> from typing import Sequence
>>> from clingo.control import Control
>>> from clingo.core import Library
>>> from clingo.propagate import Assignment, PropagateControl, PropagateInit, Propagator
>>> from clingo.symbol import Function
>>>
>>> LIB = Library()
>>>
>>> class AIFFB(Propagator):
...     slit_a: int
...     slit_b: int
...
...     def __init__(self) -> None:
...         super().__init__()
...         self.slit_a = 0
...         self.slit_b = 0
...
...     # add watches for atoms `a` and `b`
...     def init(self, assignment: Assignment, init: PropagateInit) -> None:
...         # get program literals for atoms `a` and `b`
...         plit_a = init.base[Function(LIB, "a")].literal
...         plit_b = init.base[Function(LIB, "b")].literal
...         # get solver literals for program literals `a` and `b`
...         self.slit_a = init.solver_literal(plit_a)
...         self.slit_b = init.solver_literal(plit_b)
...         # add watches for solver literals `a` and `b`
...         init.add_watch(self.slit_a)
...         init.add_watch(self.slit_b)
...
...     # propagate solver literals `a` and `b`
...     def propagate(self, assignment: Assignment, control: PropagateControl, changes: Sequence[int]) -> None:
...         # if `a` is true imply `b`
...         if self.slit_a in changes:
...             assert assignment.is_true(self.slit_a)
...             control.add_clause([-self.slit_a, self.slit_b])
...         # if `b` is true imply `a`
...         if self.slit_b in changes:
...             assert assignment.is_true(self.slit_b)
...             control.add_clause([-self.slit_b, self.slit_a])
...
>>> ctl = Control(LIB, ["0"])
>>> ctl.register_propagator(AIFFB())
>>> ctl.parse_string("1 { a; b }.")
>>> ctl.ground()
>>> with ctl.solve(on_model=print) as hnd:
>>>     print(hnd.get())
a b
SAT
```
"""

from __future__ import annotations

import collections.abc
import enum
import typing

import clingo.base
import clingo.core

__all__ = [
    "Assignment",
    "CheckMode",
    "PropagateControl",
    "PropagateInit",
    "Propagator",
    "Trail",
    "UndoMode",
    "WeightConstraintType",
]

class CheckMode(enum.IntEnum):
    """
    Enumeration of check modes.
    """

    Both: typing.ClassVar[CheckMode]  # value = <CheckMode.Both: 3>
    Fixpoint: typing.ClassVar[CheckMode]  # value = <CheckMode.Fixpoint: 2>
    Off: typing.ClassVar[CheckMode]  # value = <CheckMode.Off: 0>
    Total: typing.ClassVar[CheckMode]  # value = <CheckMode.Total: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class UndoMode(enum.IntEnum):
    """
    Enumeration of undo modes.
    """

    Always: typing.ClassVar[UndoMode]  # value = <UndoMode.Always: 1>
    Default: typing.ClassVar[UndoMode]  # value = <UndoMode.Default: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class WeightConstraintType(enum.IntEnum):
    """
    Enumeration of weight constraint types.
    """

    Equivalence: typing.ClassVar[
        WeightConstraintType
    ]  # value = <WeightConstraintType.Equivalence: 0>
    LeftImplication: typing.ClassVar[
        WeightConstraintType
    ]  # value = <WeightConstraintType.LeftImplication: -1>
    RightImplication: typing.ClassVar[
        WeightConstraintType
    ]  # value = <WeightConstraintType.RightImplication: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Assignment:
    """
    Provides information about the current state of literals in the solver.

    It provides methods to inspect and query the assignment, which is essential for
    implementing custom propagators.

    Key concepts:
    - Each literal is either true, false, or unassigned.
    - Each assigned literal has a decision level.
    - There is exactly one decision literal per level.
    - All other literals on the same level are implied by the decision literal and
      literals from earlier levels.
    - The current decision level is the highest level at which atoms are assigned.
    - The root level is the lowest decision level that can be backtracked to.

    Implements `Sequence[int]` to access the solver literals in the assignment.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __contains__(self, value: int | typing.SupportsIndex) -> bool:
        """
        Check whether the sequence contains the given value.
        """

    def __getitem__(self, index: int | typing.SupportsIndex) -> int:
        """
        Get the value at the given index.
        """

    def __iter__(self) -> collections.abc.Iterator[int]:
        """
        Get an iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.
        """

    def __reversed__(self) -> collections.abc.Iterator[int]:
        """
        Get a reverse iterator for the sequence.
        """

    def count(self, value: int | typing.SupportsIndex) -> int:
        """
        Count how often the given value occurs in the sequence.
        """

    def decision(self, level: int | typing.SupportsIndex) -> int:
        """
        Returns the decision literal of the given level.

        Each level has exactly one decision literal, which implies other literals on
        the same level.

        Args:
            level: The decision level.
        Returns:
            The decision literal.
        """

    def index(self, value: int | typing.SupportsIndex) -> int:
        """
        Get the index of the given value in the sequence.
        """

    def is_false(self, literal: int | typing.SupportsIndex) -> bool:
        """
        Check if the given literal is false.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is false.
        """

    def is_fixed(self, literal: int | typing.SupportsIndex) -> bool:
        """
        Checks if the truth value of the literal is fixed.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is fixed.
        """

    def is_free(self, literal: int | typing.SupportsIndex) -> bool:
        """
        Check if the given literal is free.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is free.
        """

    def is_true(self, literal: int | typing.SupportsIndex) -> bool:
        """
        Check if the given literal is true.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is true.
        """

    def level(self, literal: int | typing.SupportsIndex) -> int:
        """
        Returns the decision level of the given literal.

        The decision level indicates when the literal was assigned or implied during
        the solving process.

        Args:
            literal: The solver literal.
        Returns:
            The decision level of the literal.
        """

    def value(self, literal: int | typing.SupportsIndex) -> bool | None:
        """
        Returns the truth value of the literal, or None if unassigned.

        Args:
            literal: The solver literal.
        Returns:
            The truth value of the literal.
        """

    @property
    def decision_level(self) -> int:
        """
        Get the current decision level.
        """

    @property
    def has_conflict(self) -> bool:
        """
        Check if the assignment is conflicting.
        """

    @property
    def is_total(self) -> bool:
        """
        Check if all literals in the assigment are assigned.
        """

    @property
    def root_level(self) -> int:
        """
        Get the current root level.

        The root level is the lowest decision level that can be backtracked to.
        Literals on the root level or below can be considered fixed by a propagator.
        They include, for example, assumptions.
        """

    @property
    def thread_id(self) -> int:
        """
        Get the id of the thread to which the assignment belongs.

        Thread ids are consecutive numbers starting with zero.
        """

    @property
    def trail(self) -> Trail:
        """
        Get the trail of literals.
        """

class PropagateControl:
    """
    Class for controlling propagators.

    This class provides methods for adding clauses, literals, and nogoods, as well
    as managing watches and performing propagation.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def add_clause(
        self,
        literals: typing.Sequence[int | typing.SupportsIndex],
        tag: bool = False,
        lock: bool = False,
    ) -> bool:
        """
        Add a clause to the solver.

        Tagged clauses are deleted after the current solve call finishes while locked
        clauses are exempt from the solvers clause deletion strategy.

        See `propagate()` for how to handle the case that the function returns false.

        Args:
            literals:
                A sequence of solver literals representing the clause.
            tag:
                Whether to tag the clause.
            lock:
                Whether to lock the clause.

        Returns:
            Whether the clause could be integrated without conflict.
        """

    def add_literal(self, freeze: bool = True) -> int:
        """
        Add a literal to the solver.

        If the function is called in the context of a particular solver thread, the literal is only added to
        that thread and deleted after the active solve call.

        Args:
            freeze:
                Whether to freeze the literal.

        Returns:
            A fresh solver literal.
        """

    def add_nogood(
        self,
        literals: typing.Sequence[int | typing.SupportsIndex],
        tag: bool = False,
        lock: bool = False,
    ) -> bool:
        """
        A shortcut for `add_clause([-literal for literal in literals], tag, lock)`.

        Args:
            literals:
                A sequence of solver literals representing the nogood.
            tag:
                Whether to tag the nogood.
            lock:
                Whether to lock the nogood.

        Returns:
            Whether the nogood could be integrated without conflict.
        """

    def add_watch(self, literal: int | typing.SupportsIndex) -> None:
        """
        Add a watch for the given solver literal.

        Args:
            literal: The literal to watch.
        """

    def add_weight_constraint(
        self,
        literal: int | typing.SupportsIndex,
        literals: typing.Sequence[tuple[int, int]],
        bound: int | typing.SupportsIndex,
        type: WeightConstraintType = WeightConstraintType.Equivalence,
    ) -> bool:
        """
        Add a weight constraint to the solver.

        See `add_clause` for how to interpret the return value.

        Args:
            literal:
                The literal associated with the constraint.
            literals:
                A sequence of (literal, weight) tuples.
            bound:
                The bound of the weight constraint.
            type:
                The type of the weight constraint.

        Returns:
            Whether the weight constraint could be added without conflict.
        """

    def has_watch(self, literal: int | typing.SupportsIndex) -> bool:
        """
        Check if a watch exists for the given solver literal.

        Args:
            literal: The literal to check.

        Returns:
            True if a watch exists for the literal, False otherwise.
        """

    def propagate(self) -> bool:
        """
        Perform propagation in the solver.

        If this function returns False, the propagator must add no further
        clauses/literals and immediately return from the corresponding
        `Propagator.init()`, `Propagator.propagate()` or `Propagator.check()` call.

        Returns:
            True if propagation was successful, False otherwise.
        """

    def remove_watch(self, literal: int | typing.SupportsIndex) -> None:
        """
        Remove the watch for the given literal.

        This function has no effect if the literal is not watched.

        Args:
            literal: The literal to remove the watch for.
        """

class PropagateInit(PropagateControl):
    """
    Class for initializing a propagator.

    This class extends PropagateControl and additionally provides methods for freezing and looking up literals, adding
    constraints, and configuring the propagator's behavior.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def add_minimize(
        self,
        literal: int | typing.SupportsIndex,
        weight: int | typing.SupportsIndex,
        priority: int | typing.SupportsIndex = 0,
    ) -> None:
        """
        Add a weighted literal to minimize to the solver.

        Args:
            literal:
                The literal to minimize.
            weight:
                The weight of the literal.
            priority:
                The priority of the literal.
        """

    def freeze_literal(self, literal: int | typing.SupportsIndex) -> None:
        """
        Freeze the given literal.

        Frozen literals are exempt from simplification. This is important for literals
        whose truth values a propagator has to track.

        Args:
            literal:
                The literal to freeze.
        """

    def solver_literal(self, literal: int | typing.SupportsIndex) -> int:
        """
        Map the given program literal to a solver literal.

        Args:
            literal:
                The program literal to map.

        Returns:
            The corresponding solver literal.
        """

    @property
    def base(self) -> clingo.base.Base:
        """
        The base object to inspect the grounder's base.
        """

    @property
    def check_mode(self) -> CheckMode:
        """
        Get/set the check mode for the propagator.
        """

    @check_mode.setter
    def check_mode(self, arg1: CheckMode) -> None: ...
    @property
    def library(self) -> clingo.core.Library:
        """
        The library object managing symbols.
        """

    @property
    def number_of_threads(self) -> int:
        """
        The number of solver threads.
        """

    @property
    def undo_mode(self) -> UndoMode:
        """
        Get/set the undo mode for the propagator.
        """

    @undo_mode.setter
    def undo_mode(self, arg1: UndoMode) -> None: ...

class Propagator:
    """

    Interface for implementing propagators.

    This class defines methods that can be implemented to create custom propagators
    for use with the solver. They can be left empty to use their default
    implementation.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __init__(self) -> None: ...
    def attach(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Initialize solver thread with given id.

        This function is called once for each solving thread before solving of the
        current step is started.
        It can be used to add thread-specific watches and literals, or
        initialize thread-specific data structures.

        Args:
            assignment:
                The current assignment.
            control:
                The propagate control object for managing propagation.
        """

    def check(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Check if the current assignment is valid.

        This method is called on propagation fixpoints or total assignments (see
        `PropagateInit.check_mode`). A propagator should add clauses to implement its
        constraints here.

        Args:
            assignment:
                The current assignment.
            control:
                The propagate control object for managing propagation.
        """

    def decide(
        self, assignment: Assignment, fallback: int | typing.SupportsIndex
    ) -> int:
        """
        Make a decision on the next literal to assign.

        This method is called to decide on a literal to be assigned next.

        Args:
            assignment:
                The current assignment.
            fallback:
                The literal choosen by the solver's heuristic.

        Returns:
            The literal to assign or 0 if no decision was made.
        """

    def init(self, assignment: Assignment, init: PropagateInit) -> None:
        """
        Initialize the propagator.

        This method is called once before each solving step. It is used to map
        relevant program literals to solver literals, add watches for solver
        literals, and initialize the propagator's internal state.

        Args:
            assignment:
                The current assignment.
            init:
                The propagate init object for initializing the propagator.
        """

    def propagate(
        self,
        assignment: Assignment,
        control: PropagateControl,
        changes: typing.Sequence[int | typing.SupportsIndex],
    ) -> None:
        """
        Propagate given set of changes.

        This method is called during propagation with a non-empty list of watched literals
        that have been assigned truth values. A propagator should add clauses to
        propagate literals and to implement its constraints.

        Typical propagators add unit-resulting or conflicting constraints only.

        Args:
            assignment:
                The current assignment.
            control:
                The propagate control object for managing propagation.
            changes:
                A list of literals that have changed.
        """

    def undo(
        self,
        assignment: Assignment,
        changes: typing.Sequence[int | typing.SupportsIndex],
    ) -> None:
        """
        Undo previous assignments.

        This method is called to undo previous assignments.

        See also `PropagateInit.undo_mode`.

        Args:
            assignment:
                The current assignment.
            changes:
                The literals whose assignment is undone.
        """

class Trail:
    """
    Provides access to literals in the solver's trail.

    The trail represents the sequence of literals assigned during the solving
    process. It is structured by decision levels, where each level contains
    literals assigned due to implications from the decision literal at that level.

    The literals within each level are ordered by implication, reflecting the
    logical dependencies between them as determined by the solver's propagation and
    learning mechanisms. The decision literal for each level is placed at the
    beginning of its respective sequence.

    Implements `Sequence[int]` to access the solver literals in the trail.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __contains__(self, value: int | typing.SupportsIndex) -> bool:
        """
        Check whether the sequence contains the given value.
        """

    @typing.overload
    def __getitem__(self, index: int | typing.SupportsIndex) -> int:
        """
        Get the value at the given index.
        """

    @typing.overload
    def __getitem__(self, slice: slice) -> typing.Sequence[int]:
        """
        Slice the sequence.
        """

    def __iter__(self) -> collections.abc.Iterator[int]:
        """
        Get an iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.
        """

    def __reversed__(self) -> collections.abc.Iterator[int]:
        """
        Get a reverse iterator for the sequence.
        """

    def begin(self, level: int | typing.SupportsIndex) -> int:
        """
        Get the index of the first literal on the given level.

        This also corresponds to the decision literal for that level.

        Args:
            level: A decision level.
        Returns:
            The index of the decision literal.
        """

    def count(self, value: int | typing.SupportsIndex) -> int:
        """
        Count how often the given value occurs in the sequence.
        """

    def end(self, level: int | typing.SupportsIndex) -> int:
        """
        Get the index after the last literal on the given level.

        Args:
            level: A decision level.
        Returns:
            The index after the last literal.
        """

    def index(self, value: int | typing.SupportsIndex) -> int:
        """
        Get the index of the given value in the sequence.
        """

    def level(self, level: int | typing.SupportsIndex) -> typing.Sequence[int]:
        """
        Get the literals assigned at the given decision level.

        This sequence includes all literals assigned at this level in implication
        order, with the decision literal at the beginning of the sequence.

        Args:
            level: A decision level.
        Returns:
            A sequence of literals.
        """

class _TrailView:
    """
    Provides access to a subrange of literals in the solver's trail.

    Implements `Sequence[int]` to access the solver literals in the view.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __contains__(self, value: int | typing.SupportsIndex) -> bool:
        """
        Check whether the sequence contains the given value.
        """

    @typing.overload
    def __getitem__(self, index: int | typing.SupportsIndex) -> int:
        """
        Get the value at the given index.
        """

    @typing.overload
    def __getitem__(self, slice: slice) -> typing.Sequence[int]:
        """
        Slice the sequence.
        """

    def __iter__(self) -> collections.abc.Iterator[int]:
        """
        Get an iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.
        """

    def __reversed__(self) -> collections.abc.Iterator[int]:
        """
        Get a reverse iterator for the sequence.
        """

    def count(self, value: int | typing.SupportsIndex) -> int:
        """
        Count how often the given value occurs in the sequence.
        """

    def index(self, value: int | typing.SupportsIndex) -> int:
        """
        Get the index of the given value in the sequence.
        """
