"""
Functions and classes to implement custom propagators.

# Example

```python
>>> from collections.abc import Sequence
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
>>> print(ctl.solve(on_model=print))
a b
SAT
```
"""

from collections.abc import Iterator, Sequence
from enum import IntEnum
from typing import overload

from clingo.base import Base
from clingo.core import Library

__all__: list[str] = [
    "Assignment",
    "CheckMode",
    "PropagateControl",
    "PropagateInit",
    "Propagator",
    "Trail",
    "TrailView",
    "UndoMode",
    "WeightConstraintType",
]

class WeightConstraintType(IntEnum):
    """
    Enumeration of weight constraint types.
    """

    Equivalence = 0
    """
    The literal is equivalent to the weight constraint.
    """
    LeftImplication = -1
    """
    The weight constraint implies the literal.
    """
    RightImplication = 1
    """
    The literal implies the weight constraint.
    """

class CheckMode(IntEnum):
    """
    Enumeration of check modes.
    """

    Both = 3
    """
    Call `clingo.propagate.Propagator.check` on propagation fixpoints and total
    assignments.
    """
    Fixpoint = 2
    """
    Call `clingo.propagate.Propagator.check` on propagation fixpoints.
    """
    Off = 0
    """
    Do not call `clingo.propagate.Propagator.check`.
    """
    Total = 1
    """
    Call `clingo.propagate.Propagator.check` on total assignments.

    Total checks are subject to the lock when a model is found, i.e., information
    from previously found models can be used to discard assignments in check calls.
    """

class UndoMode(IntEnum):
    """
    Enumeration of undo modes.
    """

    Always = 1
    """
    Also call `clingo.propagate.Propagator.undo` when check has been called.
    """
    Default = 0
    """
    Call `clingo.propagate.Propagator.undo` for non-empty changes only.
    """

class TrailView:
    """
    Provides access to a subrange of literals in the solver's trail.

    This provides a read-only sequence interface to access the solver literals in
    the view. The view is only valid during the propagator callback in which it was
    obtained.
    """

    def __contains__(self, value: int, /) -> bool:
        """
        Check whether the view contains the given literal.

        Args:
            value: The literal to look up.

        Returns:
            Whether the literal is contained.
        """

    @overload
    def __getitem__(self, index: int, /) -> int:
        """
        Get the literal at the given index.

        Args:
            index: The index of the literal.

        Returns:
            The literal at the given index.
        """

    @overload
    def __getitem__(self, index: slice, /) -> TrailView:
        """
        Get a view on the literals in the given slice.

        Args:
            index: The slice of literals.

        Returns:
            A view on the literals in the given slice.
        """

    def __getitem__(self, index: int | slice, /) -> int | TrailView:
        """
        Get the literal at the given index or a view on the given slice.

        Args:
            index: The index of the literal or the slice of literals.

        Returns:
            The literal at the given index or a view on the selected literals.
        """

    def __iter__(self) -> Iterator[int]:
        """
        Get an iterator over the literals in the view.

        Returns:
            An iterator over the literals in the view.
        """

    def __len__(self) -> int:
        """
        Get the number of literals in the view.

        Returns:
            The number of literals in the view.
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

    This provides a read-only sequence interface to access the solver literals in
    the trail. The trail is only valid during the propagator callback in which it
    was obtained.
    """

    def __contains__(self, value: int, /) -> bool:
        """
        Check whether the trail contains the given literal.

        Args:
            value: The literal to look up.

        Returns:
            Whether the literal is contained.
        """

    @overload
    def __getitem__(self, index: int, /) -> int:
        """
        Get the literal at the given index.

        Args:
            index: The index of the literal.

        Returns:
            The literal at the given index.
        """

    @overload
    def __getitem__(self, index: slice, /) -> TrailView:
        """
        Get a view on the literals in the given slice.

        Args:
            index: The slice of literals.

        Returns:
            A view on the literals in the given slice.
        """

    def __getitem__(self, index: int | slice, /) -> int | TrailView:
        """
        Get the literal at the given index or a view on the given slice.

        Args:
            index: The index of the literal or the slice of literals.

        Returns:
            The literal at the given index or a view on the selected literals.
        """

    def __iter__(self) -> Iterator[int]:
        """
        Get an iterator over the literals in the trail.

        Returns:
            An iterator over the literals in the trail.
        """

    def __len__(self) -> int:
        """
        Get the number of literals in the trail.

        Returns:
            The number of literals in the trail.
        """

    def begin(self, level: int) -> int:
        """
        Get the index of the first literal on the given level.

        This also corresponds to the decision literal for that level.

        Args:
            level: A decision level.

        Returns:
            The index of the decision literal.
        """

    def end(self, level: int) -> int:
        """
        Get the index after the last literal on the given level.

        Args:
            level: A decision level.

        Returns:
            The index after the last literal.
        """

    def level(self, level: int) -> TrailView:
        """
        Get the literals assigned at the given decision level.

        The view includes all literals assigned at this level in implication order,
        with the decision literal at the beginning.

        Args:
            level: A decision level.

        Returns:
            A view on the literals of the level.
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
    - The current decision level is the highest level at which literals are
      assigned.
    - The root level is the lowest decision level that can be backtracked to.

    This provides a read-only sequence interface to access the positive solver
    literals, i.e., one entry per solver variable, independent of their truth
    values. The assignment is only valid during the propagator callback in which it
    was obtained.
    """

    def __contains__(self, value: int, /) -> bool:
        """
        Check whether the given solver literal is part of the assignment.

        Both positive and negative literals are accepted. The check does not depend
        on the truth value of the literal.

        Args:
            value: The literal to look up.

        Returns:
            Whether the literal is part of the assignment.
        """

    @overload
    def __getitem__(self, index: int, /) -> int:
        """
        Get the literal at the given index.

        Args:
            index: The index of the literal.

        Returns:
            The literal at the given index.
        """

    @overload
    def __getitem__(self, index: slice, /) -> list[int]:
        """
        Get a list of the literals in the given slice.

        Args:
            index: The slice of literals.

        Returns:
            A list of the literals in the given slice.
        """

    def __getitem__(self, index: int | slice, /) -> int | list[int]:
        """
        Get the literal at the given index or the literals in the given slice.

        Args:
            index: The index of the literal or the slice of literals.

        Returns:
            The literal at the given index or a list of the selected literals.
        """

    def __iter__(self) -> Iterator[int]:
        """
        Get an iterator over the literals in the assignment.

        Returns:
            An iterator over the literals in the assignment.
        """

    def __len__(self) -> int:
        """
        Get the number of positive solver literals in the assignment.

        Returns:
            The number of positive solver literals in the assignment.
        """

    def decision(self, level: int) -> int:
        """
        Get the decision literal of the given level.

        Each level has exactly one decision literal, which implies other literals on
        the same level.

        Args:
            level: The decision level.

        Returns:
            The decision literal.
        """

    def is_false(self, literal: int) -> bool:
        """
        Check if the given literal is false.

        Args:
            literal: The solver literal.

        Returns:
            Whether the literal is false.
        """

    def is_fixed(self, literal: int) -> bool:
        """
        Check if the truth value of the given literal is fixed.

        Args:
            literal: The solver literal.

        Returns:
            Whether the literal is fixed.
        """

    def is_free(self, literal: int) -> bool:
        """
        Check if the given literal is free.

        Args:
            literal: The solver literal.

        Returns:
            Whether the literal is free.
        """

    def is_true(self, literal: int) -> bool:
        """
        Check if the given literal is true.

        Args:
            literal: The solver literal.

        Returns:
            Whether the literal is true.
        """

    def level(self, literal: int) -> int:
        """
        Get the decision level of the given literal.

        The decision level indicates when the literal was assigned or implied during
        the solving process.

        Args:
            literal: The solver literal.

        Returns:
            The decision level of the literal.
        """

    def value(self, literal: int) -> bool | None:
        """
        Get the truth value of the given literal.

        Args:
            literal: The solver literal.

        Returns:
            The truth value of the literal or `None` if it is unassigned.
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
        Check if all literals in the assignment are assigned.
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
        Get the trail of assigned literals.
        """

class PropagateControl:
    """
    Class for controlling propagators.

    This class provides methods for adding clauses, literals, and nogoods, as well
    as managing watches and performing propagation.

    If a method adding constraints or `clingo.propagate.PropagateControl.propagate`
    returns `False`, no further methods must be called on the control object or the
    assignment, and the propagator must return from the current callback
    immediately.

    A control object is only valid during the propagator callback in which it was
    obtained.
    """

    def add_clause(
        self, literals: Sequence[int], tag: bool = False, lock: bool = False
    ) -> bool:
        """
        Add a clause to the solver.

        Tagged clauses are deleted after the current solve call finishes while locked
        clauses are exempt from the solver's clause deletion strategy.

        See `clingo.propagate.PropagateControl.propagate` for how to handle the case
        that the function returns false.

        Args:
            literals: The solver literals of the clause.
            tag: Whether to tag the clause.
            lock: Whether to lock the clause.

        Returns:
            Whether the clause could be integrated without conflict.
        """

    def add_literal(self, freeze: bool = True) -> int:
        """
        Add a literal to the solver.

        If the function is called in the context of a particular solver thread, the
        literal is only added to that thread and deleted after the active solve call
        together with all clauses involving it.

        Args:
            freeze: Whether to freeze the literal.

        Returns:
            A fresh solver literal.
        """

    def add_nogood(
        self, literals: Sequence[int], tag: bool = False, lock: bool = False
    ) -> bool:
        """
        Add a nogood to the solver.

        This is a shortcut for `add_clause([-literal for literal in literals], tag, lock)`.

        Args:
            literals: The solver literals of the nogood.
            tag: Whether to tag the nogood.
            lock: Whether to lock the nogood.

        Returns:
            Whether the nogood could be integrated without conflict.
        """

    def add_watch(self, literal: int) -> None:
        """
        Add a watch for the given solver literal.

        If called on a `clingo.propagate.PropagateInit` object, the watch is added
        to all current and future solver threads. Otherwise, it is only added to
        the active solver thread.

        Args:
            literal: The literal to watch.
        """

    def add_weight_constraint(
        self,
        literal: int,
        literals: Sequence[tuple[int, int]],
        bound: int,
        type: WeightConstraintType = WeightConstraintType.Equivalence,
    ) -> bool:
        """
        Add a weight constraint to the solver.

        The constraint has the form `literal <=> sum >= bound`, where `sum` is the
        sum of the weights of the true literals in `literals`. Depending on `type`,
        the connective `<=>` can also be a left or right implication.

        See `clingo.propagate.PropagateControl.add_clause` for how to interpret the
        return value.

        Args:
            literal: The literal associated with the constraint.
            literals: The weighted literals of the constraint as (literal, weight)
                pairs.
            bound: The bound of the weight constraint.
            type: The type of the weight constraint.

        Returns:
            Whether the weight constraint could be integrated without conflict.
        """

    def has_watch(self, literal: int) -> bool:
        """
        Check if a watch exists for the given solver literal.

        The check refers to the current context, i.e., the active solver thread or,
        for a `clingo.propagate.PropagateInit` object, the initialization.

        Args:
            literal: The literal to check.

        Returns:
            Whether a watch exists for the literal.
        """

    def propagate(self) -> bool:
        """
        Perform propagation in the solver.

        If this function returns `False`, the propagator must add no further
        clauses or literals and immediately return from the corresponding
        `clingo.propagate.Propagator.init`, `clingo.propagate.Propagator.propagate`,
        or `clingo.propagate.Propagator.check` call.

        Returns:
            Whether propagation finished without conflict.
        """

    def remove_watch(self, literal: int) -> None:
        """
        Remove the watch for the given literal.

        This function has no effect if the literal is not watched. If called on a
        `clingo.propagate.PropagateInit` object, the watch is removed from all
        current and future solver threads. Otherwise, it is only removed from the
        active solver thread.

        Args:
            literal: The literal to remove the watch for.
        """

class PropagateInit(PropagateControl):
    """
    Class for initializing a propagator.

    This class extends `clingo.propagate.PropagateControl` and additionally provides
    methods for freezing and looking up literals, adding constraints, and
    configuring the propagator's behavior.

    Methods inherited from `clingo.propagate.PropagateControl` affect all current
    and future solver threads. Watched literals are frozen automatically.

    An init object is only valid during the call to
    `clingo.propagate.Propagator.init` in which it was obtained.
    """

    def add_minimize(self, literal: int, weight: int, priority: int = 0) -> None:
        """
        Add a weighted literal to minimize to the solver.

        This corresponds to a weak constraint of form `:~ literal. [weight@priority]`.

        Args:
            literal: The literal to minimize.
            weight: The weight of the literal.
            priority: The priority of the literal.
        """

    def freeze_literal(self, literal: int) -> None:
        """
        Freeze the given literal.

        Literals that are not frozen are subject to simplification and might be
        removed in a preprocessing step after propagator initialization. A
        propagator should freeze all literals over which it might add clauses during
        propagation. Watched literals are frozen automatically, and it does not
        matter which phase of a literal is frozen.

        Args:
            literal: The literal to freeze.
        """

    def solver_literal(self, literal: int | None) -> int:
        """
        Map the given program literal or condition id to a solver literal.

        Condition ids are the literals associated with the conditions of theory
        elements. If the literal is `None`, the true solver literal is returned.

        Args:
            literal: The program literal or condition id to map.

        Returns:
            The corresponding solver literal.
        """

    @property
    def base(self) -> Base:
        """
        Get the base object to inspect the grounder's base.
        """

    @property
    def check_mode(self) -> CheckMode:
        """
        Get or set the check mode of the propagator.
        """

    @check_mode.setter
    def check_mode(self, value: CheckMode) -> None: ...
    @property
    def library(self) -> Library:
        """
        Get the library object managing symbols.
        """

    @property
    def number_of_threads(self) -> int:
        """
        Get the number of solver threads.
        """

    @property
    def undo_mode(self) -> UndoMode:
        """
        Get or set the undo mode of the propagator.
        """

    @undo_mode.setter
    def undo_mode(self, value: UndoMode) -> None: ...

class Propagator:
    """
    Interface for implementing propagators.

    This class defines methods that can be overridden to create custom propagators
    for use with the solver.

    Only the methods overridden in the class of a propagator are called by the
    solver. Methods that are not overridden are not registered with the solver at
    all, which avoids the overhead of calling into Python. This matters, for
    example, for `clingo.propagate.Propagator.decide`, which is called very often
    during search. The overridden methods are determined when the propagator is
    registered; methods assigned to an instance are ignored.

    The default implementations, which can be called via `super()`, do nothing
    except `clingo.propagate.Propagator.decide`, which returns the fallback literal.
    """

    def __init__(self) -> None:
        """
        Construct a propagator.
        """

    def attach(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Attach the propagator to a solver thread.

        This function is called once for each solver thread before solving of the
        current step is started. It can be used to add thread-specific watches and
        literals, or to initialize thread-specific data structures.

        Literals and watches added in this function are only valid within the solver
        thread. Furthermore, added literals and clauses involving them are deleted
        after the current search.

        Args:
            assignment: The assignment of the solver thread.
            control: The propagate control object of the solver thread.
        """

    def check(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Check if the current assignment is valid.

        This method is called on propagation fixpoints or total assignments (see
        `clingo.propagate.PropagateInit.check_mode`). A propagator should add clauses
        to implement its constraints here. The method is called even if no watches
        have been added.

        Args:
            assignment: The current assignment.
            control: The propagate control object for managing propagation.
        """

    def decide(self, assignment: Assignment, fallback: int) -> int:
        """
        Make a decision on the next literal to assign.

        This method is called whenever propagation reaches a fixpoint and should
        return a free solver literal that is to be assigned true. If multiple
        propagators are registered, returning 0 lets a propagator registered later
        make the decision. If all propagators return 0, the fallback literal is
        used.

        Args:
            assignment: The current assignment.
            fallback: The literal chosen by the solver's heuristic.

        Returns:
            The literal to make true or 0 to leave the decision to others.
        """

    def init(self, assignment: Assignment, init: PropagateInit) -> None:
        """
        Initialize the propagator.

        This method is called once before each solving step. It is used to map
        relevant program literals to solver literals, add watches for solver
        literals, and initialize the propagator's internal state.

        This is the last point to access symbolic and theory atoms via
        `clingo.propagate.PropagateInit.base`. Once the search has started, they are
        no longer accessible.

        Args:
            assignment: The top-level assignment.
            init: The propagate init object for initializing the propagator.
        """

    def propagate(
        self,
        assignment: Assignment,
        control: PropagateControl,
        changes: Sequence[int],
    ) -> None:
        """
        Propagate the given set of changes.

        This method is called during propagation with a non-empty list of watched
        literals that have become true since the last call to
        `clingo.propagate.Propagator.propagate` or `clingo.propagate.Propagator.undo`,
        or since the start of the search. A propagator should add clauses to
        propagate literals and to implement its constraints. If adding a clause or
        propagating returns `False`, the method must return immediately.

        Typical propagators add unit-resulting or conflicting constraints only.

        The method can be called from different solver threads. Each thread has its
        own assignment and thread id.

        Args:
            assignment: The current assignment.
            control: The propagate control object for managing propagation.
            changes: The watched literals that have become true.
        """

    def undo(self, assignment: Assignment, changes: Sequence[int]) -> None:
        """
        Undo previous assignments.

        This method is called when the solver backtracks to undo the assignment of
        watched literals. It is meant to update assignment-dependent state in the
        propagator.

        See also `clingo.propagate.PropagateInit.undo_mode`.

        Args:
            assignment: The current assignment.
            changes: The watched literals whose assignment is undone.
        """
