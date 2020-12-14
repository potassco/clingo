'''
This modules contains functions and classes related to propagation.
'''

from typing import Iterable, Iterator, Optional, Sequence, Tuple
from enum import Enum
from abc import ABCMeta

from ._internal import _cb_error_handler, _c_call, _ffi, _handle_error, _lib
from .util import Slice, SlicedSequence
from .symbolic_atoms import SymbolicAtoms
from .theory_atoms import TheoryAtom


class Trail(Sequence[int]):
    '''
    Object to access literals assigned by the solver in chronological order.

    Literals in the trail are ordered by decision levels, where the first literal
    with a larger level than the previous literals is a decision; the following
    literals with same level are implied by this decision literal. Each decision
    level up to and including the current decision level has a valid offset in the
    trail.
    '''
    def __init__(self, rep):
        self._rep = rep

    def __len__(self):
        return _c_call('uint32_t', _lib.clingo_assignment_trail_size, self._rep)

    def __getitem__(self, slc):
        if isinstance(slc, slice):
            return SlicedSequence(self, Slice(slc))
        if slc < 0:
            slc += len(self)
        if slc < 0 or slc >= len(self):
            raise IndexError('invalid index')
        return _c_call('clingo_literal_t', _lib.clingo_assignment_trail_at, self._rep, slc)

    def __iter__(self):
        for i in range(len(self)):
            yield _c_call('clingo_literal_t', _lib.clingo_assignment_trail_at, self._rep, i)

    def begin(self, level: int) -> int:
        '''
        Returns the offset of the decision literal with the given decision level in the
        trail.

        Parameters
        ----------
        level : int
            The decision level.

        Returns
        -------
        int
        '''
        return _c_call('uint32_t', _lib.clingo_assignment_trail_begin, self._rep, level)

    def end(self, level: int) -> int:
        '''
        Returns the offset following the last literal with the given decision literal
        in the trail.

        Parameters
        ----------
        level : int
            The decision level.

        Returns
        -------
        int
        '''
        return _c_call('uint32_t', _lib.clingo_assignment_trail_end, self._rep, level)

class Assignment(Sequence[int]):
    '''
    Object to inspect the (parital) assignment of an associated solver.

    Assigns truth values to solver literals.  Each solver literal is either true,
    false, or undefined, represented by the Python constants `True`, `False`, or
    `None`, respectively.

    This class implements `Sequence[int]` to access the (positive)
    literals in the assignment.
    '''
    def __init__(self, rep):
        self._rep = rep

    def __len__(self):
        return _lib.clingo_assignment_size(self._rep)

    def __getitem__(self, slc):
        if isinstance(slc, slice):
            return SlicedSequence(self, Slice(slc))
        if slc < 0:
            slc += len(self)
        if slc < 0 or slc >= len(self):
            raise IndexError('invalid index')
        return _c_call('clingo_literal_t', _lib.clingo_assignment_at, self._rep, slc)

    def __iter__(self):
        for i in range(len(self)):
            yield _c_call('clingo_literal_t', _lib.clingo_assignment_at, self._rep, i)

    def decision(self, level: int) -> int:
        '''
        Return the decision literal of the given level.

        Parameters
        ----------
        level : int
            The decision level.

        Returns
        -------
        int
        '''
        return _c_call('clingo_literal_t', _lib.clingo_assignment_decision, self._rep, level)

    def has_literal(self, literal : int) -> bool:
        '''
        Determine if the given literal is valid in this solver.

        Parameters
        ----------
        literal : int
            The solver literal.

        Returns
        -------
        bool
        '''
        return _lib.clingo_assignment_has_literal(self._rep, literal)

    def is_false(self, literal: int) -> bool:
        '''
        Determine if the literal is false.

        Parameters
        ----------
        literal : int
            The solver literal.

        Returns
        -------
        bool
        '''
        return _c_call('bool', _lib.clingo_assignment_is_false, self._rep, literal)

    def is_fixed(self, literal: int) -> bool:
        '''
        Determine if the literal is assigned on the top level.

        Parameters
        ----------
        literal : int
            The solver literal.

        Returns
        -------
        bool
        '''
        return _c_call('bool', _lib.clingo_assignment_is_fixed, self._rep, literal)

    def is_true(self, literal: int) -> bool:
        '''
        Determine if the literal is true.

        Parameters
        ----------
        literal : int
            The solver literal.

        Returns
        -------
        bool
        '''
        return _c_call('bool', _lib.clingo_assignment_is_true, self._rep, literal)

    def is_free(self, literal: int) -> bool:
        '''
        Determine if the literal is free.

        Parameters
        ----------
        literal : int
            The solver literal.

        Returns
        -------
        bool
        '''
        value = _c_call('clingo_truth_value_t', _lib.clingo_assignment_truth_value, self._rep, literal)
        return value == _lib.clingo_truth_value_free

    def level(self, literal: int) -> int:
        '''
        The decision level of the given literal.

        Parameters
        ----------
        literal : int
            The solver literal.

        Returns
        -------
        int

        Notes
        -----
        Note that the returned value is only meaningful if the literal is assigned -
        i.e., `value(lit) is not None`.
        '''
        return _c_call('uint32_t', _lib.clingo_assignment_level, self._rep, literal)

    def value(self, literal) -> Optional[bool]:
        '''
        Get the truth value of the given literal or `None` if it has none.

        Parameters
        ----------
        literal : int
            The solver literal.

        Returns
        -------
        Optional[bool]
        '''
        value = _c_call('clingo_truth_value_t', _lib.clingo_assignment_truth_value, self._rep, literal)
        if value == _lib.clingo_truth_value_true:
            return True
        if value == _lib.clingo_truth_value_false:
            return False
        assert value == _lib.clingo_truth_value_free
        return None

    @property
    def decision_level(self) -> int:
        '''
        The current decision level.
        '''
        return _lib.clingo_assignment_decision_level(self._rep)

    @property
    def has_conflict(self) -> bool:
        '''
        True if the assignment is conflicting.
        '''
        return _lib.clingo_assignment_has_conflict(self._rep)

    @property
    def is_total(self) -> bool:
        '''
        Whether the assignment is total.
        '''
        return _lib.clingo_assignment_is_total(self._rep)

    @property
    def root_level(self) -> int:
        '''
        The current root level.
        '''
        return _lib.clingo_assignment_root_level(self._rep)

    @property
    def trail(self) -> Trail:
        '''
        The trail of assigned literals.
        '''
        return Trail(self._rep)

class PropagatorCheckMode(Enum):
    '''
    Enumeration of supported check modes for propagators.

    Note that total checks are subject to the lock when a model is found. This
    means that information from previously found models can be used to discard
    assignments in check calls.

    Attributes
    ----------
    Off : PropagatorCheckMode
        Do not call `Propagator.check` at all.
    Total : PropagatorCheckMode
        Call `Propagator.check` on total assignments.
    Fixpoint : PropagatorCheckMode
        Call `Propagator.check` on propagation fixpoints.
    Both : PropagatorCheckMode
        Call `Propagator.check` on propagation fixpoints and total assignments.
    '''
    Both = _lib.clingo_propagator_check_mode_both
    Fixpoint = _lib.clingo_propagator_check_mode_fixpoint
    Off = _lib.clingo_propagator_check_mode_none
    Total = _lib.clingo_propagator_check_mode_total

class PropagateInit:
    '''
    Object that is used to initialize a propagator before each solving step.

    See Also
    --------
    Control.register_propagator
    '''
    def __init__(self, rep):
        self._rep = rep

    def add_clause(self, clause: Sequence[int]) -> bool:
        '''
        Statically adds the given clause to the problem.

        Parameters
        ----------
        clause : Sequence[int]
            The clause over solver literals to add.

        Returns
        -------
        bool
            Returns false if the program becomes unsatisfiable.

        Notes
        -----
        If this function returns false, initialization should be stopped and no further
        functions of the `PropagateInit` and related objects should be called.
        '''
        return _c_call('bool', _lib.clingo_propagate_init_add_clause, self._rep, clause, len(clause))

    def add_literal(self, freeze: bool=True) -> int:
        '''
        Statically adds a literal to the solver.

        To be able to use the variable in clauses during propagation or add watches to
        it, it has to be frozen. Otherwise, it might be removed during preprocessing.

        Parameters
        ----------
        freeze : bool=True
            Whether to freeze the variable.

        Returns
        -------
        int
            Returns the added literal.

        Notes
        -----
        If literals are added to the solver, subsequent calls to `add_clause` and
        `propagate` are expensive. It is best to add literals in batches.
        '''
        return _c_call('clingo_literal_t', _lib.clingo_propagate_init_add_literal, self._rep, freeze)

    def add_minimize(self, literal: int, weight: int, priority: int=0) -> None:
        '''
        Extends the solver's minimize constraint with the given weighted literal.

        Parameters
        ----------
        literal : int
            The literal to add.
        weight : int
            The weight of the literal.
        priority : int=0
            The priority of the literal.
        '''
        _handle_error(_lib.clingo_propagate_init_add_minimize(self._rep, literal, weight, priority))

    def add_watch(self, literal: int, thread_id: Optional[int]=None) -> None:
        '''
        Add a watch for the solver literal in the given phase.

        Parameters
        ----------
        literal : int
            The solver literal to watch.
        thread_id : Optional[int]
            The id of the thread to watch the literal. If the is `None` then all active
            threads will watch the literal.

        Returns
        -------
        None
        '''
        if thread_id is None:
            _handle_error(_lib.clingo_propagate_init_add_watch(self._rep, literal))
        else:
            _handle_error(_lib.clingo_propagate_init_add_watch_to_thread(self._rep, literal, thread_id))

    def add_weight_constraint(self, literal: int, literals: Sequence[Tuple[int,int]],
                              bound: int, type_: int=0, compare_equal: bool=False) -> bool:
        '''
        Statically adds a constraint of form

            literal <=> { l=w | (l, w) in literals } >= bound

        to the solver.

        - If `type_ < 0`, then `<=>` is a left implication.
        - If `type_ > 0`, then `<=>` is a right implication.
        - Otherwise, `<=>` is an equivalence.

        Parameters
        ----------
        literal : int
            The literal associated with the constraint.
        literals : Sequence[Tuple[int,int]]
            The weighted literals of the constrain.
        bound : int
            The bound of the constraint.
        type_ : int
            Add a weight constraint of the given type_.
        compare_equal : bool=False
            A Boolean indicating whether to compare equal or less than equal.

        Returns
        -------
        bool
            Returns false if the program becomes unsatisfiable.

        Notes
        -----
        If this function returns false, initialization should be stopped and no further
        functions of the `PropagateInit` and related objects should be called.
        '''
        return _c_call('bool', _lib.clingo_propagate_init_add_weight_constraint,
                       self._rep, literal, literals, len(literals), bound, type_, compare_equal)

    def propagate(self) -> bool:
        '''
        Propagates consequences of the underlying problem excluding registered propagators.

        Returns
        -------
        bool
            Returns false if the program becomes unsatisfiable.

        Notes
        -----
        This function has no effect if SAT-preprocessing is enabled.

        If this function returns false, initialization should be stopped and no further
        functions of the `PropagateInit` and related objects should be called.
        '''
        return _c_call('bool', _lib.clingo_propagate_init_propagate, self._rep)

    def solver_literal(self, literal: int) -> int:
        '''
        Maps the given program literal or condition id to its solver literal.

        Parameters
        ----------
        literal : int
            A program literal or condition id.

        Returns
        -------
        int
            A solver literal.
        '''
        return _c_call('clingo_literal_t', _lib.clingo_propagate_init_solver_literal, self._rep, literal)

    @property
    def assignment(self) -> Assignment:
        '''
        `Assignment` object capturing the top level assignment.
        '''
        return Assignment(_lib.clingo_propagate_init_assignment(self._rep))

    @property
    def check_mode(self) -> PropagatorCheckMode:
        '''
        `PropagatorCheckMode` controlling when to call `Propagator.check`.
        '''
        return PropagatorCheckMode(_lib.clingo_propagate_init_get_check_mode(self._rep))

    @check_mode.setter
    def check_mode(self, mode: PropagatorCheckMode) -> None:
        _lib.clingo_propagate_init_set_check_mode(self._rep, mode.value)

    @property
    def number_of_threads(self) -> int:
        '''
        The number of solver threads used in the corresponding solve call.
        '''
        return _lib.clingo_propagate_init_number_of_threads(self._rep)

    @property
    def symbolic_atoms(self) -> SymbolicAtoms:
        '''
        The symbolic atoms captured by a `SymbolicAtoms` object.
        '''
        atoms = _c_call('clingo_symbolic_atoms_t*', _lib.clingo_propagate_init_symbolic_atoms, self._rep)
        return SymbolicAtoms(atoms)

    @property
    def theory_atoms(self) -> Iterator[TheoryAtom]:
        '''
        An iterator over all theory atoms.
        '''
        atoms = _c_call('clingo_theory_atoms_t*', _lib.clingo_propagate_init_theory_atoms, self._rep)
        size = _c_call('size_t', _lib.clingo_theory_atoms_size, atoms)

        for idx in range(size):
            yield TheoryAtom(atoms, idx)

class PropagateControl(metaclass=ABCMeta):
    '''
    This object can be used to add clauses and to propagate them.

    See Also
    --------
    Control.register_propagator
    '''
    def __init__(self, rep):
        self._rep = rep

    def add_clause(self, clause: Sequence[int], tag: bool=False, lock: bool=False) -> bool:
        '''
        Add the given clause to the solver.

        Parameters
        ----------
        clause : Sequence[int]
            List of solver literals forming the clause.
        tag : bool=False
            If true, the clause applies only in the current solving step.
        lock : bool=False
            If true, exclude clause from the solver's regular clause deletion policy.

        Returns
        -------
        bool
            This method returns false if the current propagation must be stopped.
        '''
        type_ = 0
        if tag:
            type_ |= _lib.clingo_clause_type_volatile
        if lock:
            type_ |= _lib.clingo_clause_type_static
        return _c_call('bool', _lib.clingo_propagate_control_add_clause, self._rep, clause, len(clause), type_)

    def add_literal(self) -> int:
        '''
        Adds a new positive volatile literal to the underlying solver thread.

        The literal is only valid within the current solving step and solver thread.
        All volatile literals and clauses involving a volatile literal are deleted
        after the current search.

        Returns
        -------
        int
            The added solver literal.
        '''
        return _c_call('clingo_literal_t', _lib.clingo_propagate_control_add_literal, self._rep)

    def add_nogood(self, clause: Iterable[int], tag: bool=False, lock: bool=False) -> bool:
        '''
        Equivalent to `self.add_clause([-lit for lit in clause], tag, lock)`.
        '''
        return self.add_clause([-lit for lit in clause], tag, lock)

    def add_watch(self, literal: int) -> None:
        '''
        Add a watch for the solver literal in the given phase.

        Parameters
        ----------
        literal : int
            The target solver literal.

        Returns
        -------
        None

        Notes
        -----
        Unlike `PropagateInit.add_watch` this does not add a watch to all solver
        threads but just the current one.
        '''
        _handle_error(_lib.clingo_propagate_control_add_watch(self._rep, literal))

    def has_watch(self, literal: int) -> bool:
        '''
        Check whether a literal is watched in the current solver thread.

        Parameters
        ----------
        literal : int
            The target solver literal.

        Returns
        -------
        bool
            Whether the literal is watched.
        '''
        return _lib.clingo_propagate_control_has_watch(self._rep, literal)

    def propagate(self) -> bool:
        '''
        Propagate literals implied by added clauses.

        Returns
        -------
        bool
            This method returns false if the current propagation must be stopped.
        '''
        return _c_call('bool', _lib.clingo_propagate_control_propagate, self._rep)

    def remove_watch(self, literal: int) -> None:
        '''
        Removes the watch (if any) for the given solver literal.

        Parameters
        ----------
        literal : int
            The target solver literal.

        Returns
        -------
        None
        '''
        _lib.clingo_propagate_control_remove_watch(self._rep, literal)

    @property
    def assignment(self) -> Assignment:
        '''
        `Assignment` object capturing the partial assignment of the current solver thread.
        '''
        return Assignment(_lib.clingo_propagate_control_assignment(self._rep))

    @property
    def thread_id(self) -> int:
        '''
        The numeric id of the current solver thread.
        '''
        return _lib.clingo_propagate_control_thread_id(self._rep)

class Propagator(metaclass=ABCMeta):
    '''
    Propagator interface for custom constraints.
    '''
    def init(self, init: PropagateInit) -> None:
        '''
        This function is called once before each solving step.

        It is used to map relevant program literals to solver literals, add
        watches for solver literals, and initialize the data structures used
        during propagation.

        Parameters
        ----------
        init : PropagateInit
            Object to initialize the propagator.

        Returns
        -------
        None

        Notes
        -----
        This is the last point to access theory atoms.  Once the search has
        started, they are no longer accessible.
        '''

    def propagate(self, control: PropagateControl, changes: Sequence[int]) -> None:
        '''
        Can be used to propagate solver literals given a partial assignment.

        Parameters
        ----------
        control : PropagateControl
            Object to control propagation.
        changes : Sequence[int]
            List of watched solver literals assigned to true.

        Returns
        -------
        None

        Notes
        -----
        Called during propagation with a non-empty list of watched solver
        literals that have been assigned to true since the last call to either
        propagate, undo, (or the start of the search) - the change set. Only
        watched solver literals are contained in the change set. Each literal
        in the change set is true w.r.t. the current Assignment.
        `PropagateControl.add_clause` can be used to add clauses. If a clause
        is unit resulting, it can be propagated using
        `PropagateControl.propagate`. If either of the two methods returns
        False, the propagate function must return immediately.

            c = ...
            if not control.add_clause(c) or not control.propagate(c):
                return

        Note that this function can be called from different solving threads.
        Each thread has its own assignment and id, which can be obtained using
        `PropagateControl.thread_id`.
        '''

    def undo(self, thread_id: int, assignment: Assignment,
             changes: Sequence[int]) -> None:
        '''
        Called whenever a solver with the given id undos assignments to watched
        solver literals.

        Parameters
        ----------
        thread_id : int
            The solver thread id.
        assignment : Assignment
            Object for inspecting the partial assignment of the solver.
        changes : Sequence[int]
            The list of watched solver literals whose assignment is undone.

        Returns
        -------
        None

        Notes
        -----
        This function is meant to update assignment dependent state in a
        propagator but not to modify the current state of the solver.
        Furthermore, errors raised in the function lead to program termination.
        '''

    def check(self, control: PropagateControl) -> None:
        '''
        This function is similar to propagate but is called without a change
        set on propagation fixpoints.

        When exactly this function is called, can be configured using the @ref
        PropagateInit.check_mode property.

        Parameters
        ----------
        control : PropagateControl
            Object to control propagation.

        Returns
        -------
        None

        Notes
        -----
        This function is called even if no watches have been added.
        '''

    def decide(self, thread_id: int, assignment: Assignment, fallback: int) -> int:
        '''
        This function allows a propagator to implement domain-specific
        heuristics.

        It is called whenever propagation reaches a fixed point.

        Parameters
        ----------
        thread_id : int
            The solver thread id.
        assignment : Assignment
            Object for inspecting the partial assignment of the solver.
        fallback : int
            The literal choosen by the solver's heuristic.

        Returns
        -------
        int
            Тhe next solver literal to make true.

        Notes
        -----
        This function should return a free solver literal that is to be
        assigned true. In case multiple propagators are registered, this
        function can return 0 to let a propagator registered later make a
        decision. If all propagators return 0, then the fallback literal is
        used.
        '''

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_propagator_init(init, data):
    propagator = _ffi.from_handle(data).data
    propagator.init(PropagateInit(init))
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_propagator_propagate(control, changes, size, data):
    propagator = _ffi.from_handle(data).data
    py_changes = [changes[i] for i in range(size)]
    propagator.propagate(PropagateControl(control), py_changes)
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_propagator_undo(control, changes, size, data):
    propagator = _ffi.from_handle(data).data
    py_changes = [changes[i] for i in range(size)]
    ctl = PropagateControl(control)
    propagator.undo(ctl.thread_id, ctl.assignment, py_changes)

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_propagator_check(control, data):
    propagator = _ffi.from_handle(data).data
    propagator.check(PropagateControl(control))
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_propagator_decide(thread_id, assignment, fallback, data, decision):
    propagator = _ffi.from_handle(data).data
    decision[0] = propagator.decide(thread_id, Assignment(assignment), fallback)
    return True
