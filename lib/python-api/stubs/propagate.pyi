"""
Functions and classes to implement custom propagators.

# Example

```python
>>> from typing import Sequence
>>> from clingo.control import Control
>>> from clingo.core import Library
>>> from clingo.propagate import PropagateControl, PropagateInit, Propagator
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
...     def init(self, init: PropagateInit) -> None:
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
...     def propagate(self, control: PropagateControl, changes: Sequence[int]) -> None:
...         # if `a` is true imply `b`
...         if self.slit_a in changes:
...             assert control.assignment.is_true(self.slit_a)
...             control.add_clause([-self.slit_a, self.slit_b])
...         # if `b` is true imply `a`
...         if self.slit_b in changes:
...             assert control.assignment.is_true(self.slit_b)
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

class CheckMode:
    """
    Enumeration of check modes.

    Members:

      Off : Do not call `Propagator.check()` at all.

      Fixpoint : Call `Propagator.check()` on propagation fixpoints.

      Total : Call `Propagator.check()` on total assignments.

      Both : Call `Propagator.check()` on propagation fixpoints and total assignments.
    """

    Both: typing.ClassVar[CheckMode]  # value = <CheckMode.Both: 3>
    Fixpoint: typing.ClassVar[CheckMode]  # value = <CheckMode.Fixpoint: 2>
    Off: typing.ClassVar[CheckMode]  # value = <CheckMode.Off: 0>
    Total: typing.ClassVar[CheckMode]  # value = <CheckMode.Total: 1>
    __members__: typing.ClassVar[
        dict[str, CheckMode]
    ]  # value = {'Off': <CheckMode.Off: 0>, 'Fixpoint': <CheckMode.Fixpoint: 2>, 'Total': <CheckMode.Total: 1>, 'Both': <CheckMode.Both: 3>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __init__(self, value: typing.SupportsInt) -> None: ...
    def __int__(self) -> int: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __setstate__(self, state: typing.SupportsInt) -> None: ...
    def __str__(self) -> str: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class UndoMode:
    """
    Enumeration of undo modes.

    Members:

      Default : Call `Propagator.undo()` for decision levels with non-emty changes.

      Always : Additionally call `Propagator.undo()` when check has been called.
    """

    Always: typing.ClassVar[UndoMode]  # value = <UndoMode.Always: 1>
    Default: typing.ClassVar[UndoMode]  # value = <UndoMode.Default: 0>
    __members__: typing.ClassVar[
        dict[str, UndoMode]
    ]  # value = {'Default': <UndoMode.Default: 0>, 'Always': <UndoMode.Always: 1>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __init__(self, value: typing.SupportsInt) -> None: ...
    def __int__(self) -> int: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __setstate__(self, state: typing.SupportsInt) -> None: ...
    def __str__(self) -> str: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class WeightConstraintType:
    """
    Enumeration of weight constraint types.

    Members:

      Equivalence : The weight constraint is equal to its literal.

      LeftImplication : The weight constraint implies its literal.

      RightImplication : The literal implies the weight constraint.
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
    __members__: typing.ClassVar[
        dict[str, WeightConstraintType]
    ]  # value = {'Equivalence': <WeightConstraintType.Equivalence: 0>, 'LeftImplication': <WeightConstraintType.LeftImplication: -1>, 'RightImplication': <WeightConstraintType.RightImplication: 1>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __init__(self, value: typing.SupportsInt) -> None: ...
    def __int__(self) -> int: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __setstate__(self, state: typing.SupportsInt) -> None: ...
    def __str__(self) -> str: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

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
    def __contains__(self, value: typing.SupportsInt) -> bool:
        """
        Get a reverse iterator for the sequence.
        """

    def __getitem__(self, index: typing.SupportsInt) -> int:
        """
        Get the value at the given index.
        """

    def __iter__(self) -> typing.Iterator[int]:
        """
        Get an iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.
        """

    def __reversed__(self) -> typing.Iterator[int]:
        """
        Get a reverse iterator for the sequence.
        """

    def count(self, value: typing.SupportsInt) -> int:
        """
        Count how often the given value occurs in the sequence.
        """

    def decision(self, level: typing.SupportsInt) -> int:
        """
        Returns the decision literal of the given level.

        Each level has exactly one decision literal, which implies other literals on
        the same level.

        Args:
            level: The decision level.
        Returns:
            The decision literal.
        """

    def index(self, value: typing.SupportsInt) -> int:
        """
        Get the index of the given value in the sequence.
        """

    def is_false(self, literal: typing.SupportsInt) -> bool:
        """
        Check if the given literal is false.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is false.
        """

    def is_fixed(self, literal: typing.SupportsInt) -> bool:
        """
        Checks if the truth value of the literal is fixed.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is fixed.
        """

    def is_free(self, literal: typing.SupportsInt) -> bool:
        """
        Check if the given literal is free.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is free.
        """

    def is_true(self, literal: typing.SupportsInt) -> bool:
        """
        Check if the given literal is true.

        Args:
            literal: The solver literal.
        Returns:
            Whether the literal is true.
        """

    def level(self, literal: typing.SupportsInt) -> int:
        """
        Returns the decision level of the given literal.

        The decision level indicates when the literal was assigned or implied during
        the solving process.

        Args:
            literal: The solver literal.
        Returns:
            The decision level of the literal.
        """

    def value(self, literal: typing.SupportsInt) -> bool | None:
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
    def trail(self) -> Trail:
        """
        Get the trail of literals.
        """

class PropagateControl:
    """
    Class for controlling propagation.

    This class provides methods for adding clauses, literals, and nogoods, as well
    as managing watches and performing propagation.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def add_clause(
        self,
        literals: typing.Sequence[typing.SupportsInt],
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

    def add_literal(self) -> int:
        """
        Add a literal to the solver.

        This literal is only added to the associated solving thread and deleted after
        the solve call.

        Returns:
            A fresh solver literal.
        """

    def add_nogood(
        self,
        literals: typing.Sequence[typing.SupportsInt],
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

    def add_watch(self, literal: typing.SupportsInt) -> None:
        """
        Add a watch for the given solver literal.

        Args:
            literal: The literal to watch.
        """

    def has_watch(self, literal: typing.SupportsInt) -> bool:
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
        `Propagator.propagate()` or `Propagator.check()` call.

        Returns:
            True if propagation was successful, False otherwise.
        """

    def remove_watch(self, literal: typing.SupportsInt) -> None:
        """
        Remove the watch for the given literal.

        This function has no effect if the literal is not watched.

        Args:
            literal: The literal to remove the watch for.
        """

    @property
    def assignment(self) -> Assignment:
        """
        The current assignment.
        """

    @property
    def thread_id(self) -> int:
        """
        The id of the current solving thread.
        """

class PropagateInit:
    """
    Class for initializing a propagator.

    This class provides methods for setting up propagation, including adding
    clauses, literals, watches, and constraints.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def add_clause(self, literals: typing.Sequence[typing.SupportsInt]) -> bool:
        """
        Add a clause to the solver.

        If the clause could not be added to the solver, there is a top-level conflict
        and the underlying problem is unsatisfiable.

        Args:
            literals:
                A sequence of solver literals representing the clause.

        Returns:
            Whether the clause could be added without conflict.
        """

    def add_literal(self, freeze: bool = True) -> int:
        """
        Add a new literal to the solver.

        See also `freeze_literal()`.

        Args:
            freeze:
                Whether to freeze the literal.

        Returns:
            The newly added solver literal.
        """

    def add_minimize(
        self,
        literal: typing.SupportsInt,
        weight: typing.SupportsInt,
        priority: typing.SupportsInt = 0,
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

    def add_watch(
        self, literal: typing.SupportsInt, thread_id: typing.SupportsInt | None = None
    ) -> None:
        """
        Add a watch for the given solver literal.

        Args:
            literal:
                The literal to watch.
            thread_id:
                The id of the thread to add the watch to. If None, adds to all threads.
        """

    def add_weight_constraint(
        self,
        literal: typing.SupportsInt,
        literals: typing.Sequence[tuple[int, int]],
        bound: typing.SupportsInt,
        type: WeightConstraintType = WeightConstraintType.Equivalence,
        compare_equal: bool = False,
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
            compare_equal:
                Whether to use equality comparison.

        Returns:
            Whether the weight constraint could be added without conflict.
        """

    def freeze_literal(self, literal: typing.SupportsInt) -> None:
        """
        Freeze the given literal.

        Frozen literals are exempt from simplification. This is important for literals
        whose truth values a propagator has to track.

        Args:
            literal:
                The literal to freeze.
        """

    def propagate(self) -> bool:
        """
        Perform initial propagation.

        See the `add_clause()` for how to intepret the return value.

        Returns:
            True if propagation was successful, False otherwise.
        """

    def remove_watch(
        self, literal: typing.SupportsInt, thread_id: typing.SupportsInt | None = None
    ) -> None:
        """
        Remove the watch for the given literal.

        Args:
            literal:
                The literal to remove the watch for.
            thread_id:
                The id of the thread to remove the watch from. If None, removes from
                all threads.
        """

    def solver_literal(self, literal: typing.SupportsInt) -> int:
        """
        Map the given program literal to a solver literal.

        Args:
            literal:
                The program literal to map.

        Returns:
            The corresponding solver literal.
        """

    @property
    def assignment(self) -> Assignment:
        """
        The current assignment.
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
    def check(self, control: PropagateControl) -> None:
        """
        Check if the current assignment is valid.

        This method is called on propagation fixpoints or total assignments (see
        `PropagateInit.check_mode`). A propagator should add clauses to implement its
        constraints here.

        Args:
            control:
                The propagate control object for managing propagation.
        """

    def decide(
        self,
        thread_id: typing.SupportsInt,
        assignment: Assignment,
        fallback: typing.SupportsInt,
    ) -> int:
        """
        Make a decision on the next literal to assign.

        This method is called to decide on a literal to be assigned next.

        Args:
            thread_id:
                The id of the current solver thread.
            assignment:
                The current assignment.
            fallback:
                The literal choosen by the solver's heuristic.

        Returns:
            The literal to assign or 0 if no decision was made.
        """

    def init(self, init: PropagateInit) -> None:
        """
        Initialize the propagator.

        This method is called once before each solving step. It is used to map
        relevant program literals to solver literals, add watches for solver
        literals, and initialize the propagator's internal state.

        Args:
            init:
                The propagate init object for initializing the propagator.
        """

    def propagate(
        self, control: PropagateControl, changes: typing.Sequence[typing.SupportsInt]
    ) -> None:
        """
        Propagate given a set of changes.

        This method is called during propagation with a non-empty list watched literals
        that have been assigned truth values. A propagator should add clauses to
        propagate literals and to implement its constraints.

        Typical propagators add unit-resulting or conflicting constraints only.

        Args:
            control:
                The propagate control object for managing propagation.
            changes:
                A list of literals that have changed.
        """

    def undo(
        self,
        thread_id: typing.SupportsInt,
        assignment: Assignment,
        changes: typing.Sequence[typing.SupportsInt],
    ) -> None:
        """
        Undo previous assignments.

        This method is called to undo previous assignments.

        See also `PropagateInit.undo_mode`.

        Args:
            thread_id:
                The id of the current solver thread.
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
    def __contains__(self, value: typing.SupportsInt) -> bool:
        """
        Get a reverse iterator for the sequence.
        """

    @typing.overload
    def __getitem__(self, index: typing.SupportsInt) -> int:
        """
        Get the value at the given index.
        """

    @typing.overload
    def __getitem__(self, slice: slice) -> typing.Sequence[int]:
        """
        Slice the sequence.
        """

    def __iter__(self) -> typing.Iterator[int]:
        """
        Get an iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.
        """

    def __reversed__(self) -> typing.Iterator[int]:
        """
        Get a reverse iterator for the sequence.
        """

    def begin(self, level: typing.SupportsInt) -> int:
        """
        Get the index of the first literal on the given level.

        This also corresponds to the decision literal for that level.

        Args:
            level: A decision level.
        Returns:
            The index of the decision literal.
        """

    def count(self, value: typing.SupportsInt) -> int:
        """
        Count how often the given value occurs in the sequence.
        """

    def end(self, level: typing.SupportsInt) -> int:
        """
        Get the index after the last literal on the given level.

        Args:
            level: A decision level.
        Returns:
            The index after the last literal.
        """

    def index(self, value: typing.SupportsInt) -> int:
        """
        Get the index of the given value in the sequence.
        """

    def level(self, level: typing.SupportsInt) -> typing.Sequence[int]:
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
    def __contains__(self, value: typing.SupportsInt) -> bool:
        """
        Get a reverse iterator for the sequence.
        """

    @typing.overload
    def __getitem__(self, index: typing.SupportsInt) -> int:
        """
        Get the value at the given index.
        """

    @typing.overload
    def __getitem__(self, slice: slice) -> typing.Sequence[int]:
        """
        Slice the sequence.
        """

    def __iter__(self) -> typing.Iterator[int]:
        """
        Get an iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.
        """

    def __reversed__(self) -> typing.Iterator[int]:
        """
        Get a reverse iterator for the sequence.
        """

    def count(self, value: typing.SupportsInt) -> int:
        """
        Count how often the given value occurs in the sequence.
        """

    def index(self, value: typing.SupportsInt) -> int:
        """
        Get the index of the given value in the sequence.
        """
