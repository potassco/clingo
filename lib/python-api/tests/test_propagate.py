"""
Unit tests for clingo.propagate module.
"""

from threading import Barrier
from typing import Optional, Sequence

import pytest
from clingo.control import Control
from clingo.core import Library
from clingo.propagate import (
    Assignment,
    CheckMode,
    PropagateControl,
    PropagateInit,
    Propagator,
    UndoMode,
    WeightConstraintType,
)
from clingo.symbol import Function
from util import MCB


class AIFFBPropagator(Propagator):
    """
    Simple propagator ensuring a iff b.
    """

    slit_a: int
    slit_b: int
    n_threads: int
    errors: list[str]
    fail_thread: int
    barrier: Optional[Barrier]
    weight_con: bool

    def __init__(self) -> None:
        super().__init__()
        self.slit_a = 0
        self.slit_b = 0
        self.n_threads = 1
        self.fail_thread = -1
        self.barrier = None
        self.weight_con = False

    def init(self, assignment: Assignment, init: PropagateInit) -> None:
        """
        Add watches for atoms `a` and `b`.
        """
        assert assignment

        def watch(p):
            plit = init.base[Function(init.library, p)].literal
            slit = init.solver_literal(plit)
            init.add_watch(slit)
            return slit

        if self.slit_a == 0:
            self.slit_a = watch("a")
            self.slit_b = watch("b")

        self.errors = []
        self.n_threads = init.number_of_threads
        self.barrier = (
            Barrier(init.number_of_threads) if self.fail_thread >= 0 else None
        )

    def check(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Check if watches are set correctly.
        """
        assert assignment
        thread_id = assignment.thread_id
        for p in [self.slit_a, self.slit_b]:
            if not control.has_watch(p):
                self.errors.append(f"solver {thread_id} misses watch {p}")

        if self.weight_con:
            control.add_weight_constraint(
                1,
                [(self.slit_a, 1), (self.slit_b, 1)],
                2,
                WeightConstraintType.Equivalence,
            )

        if self.barrier:
            self.barrier.wait()
            self.barrier = None
            if thread_id == self.fail_thread:
                raise ValueError(f"Forcing error on solver {thread_id}")

    def propagate(
        self,
        assignment: Assignment,
        control: PropagateControl,
        changes: Sequence[int],
    ) -> None:
        """
        Propagate solver literals `a` and `b`.
        """

        def propagate(p, q):
            # propagate a implies b
            if p in changes:
                assert assignment.is_true(p)
                control.add_clause([-p, q], tag=True)

        if not self.weight_con:
            propagate(self.slit_a, self.slit_b)
            propagate(self.slit_b, self.slit_a)


class AssertingPropagator(Propagator):
    """
    Test adding asserting clauses.
    """

    def __init__(self, lock=False):
        super().__init__()
        self._start_lit = 0
        self._end_lit = 0
        self._value_lit = 0
        self._lock = lock

    def init(self, assignment: Assignment, init: PropagateInit) -> None:
        """
        Test init.
        """
        assert assignment
        for atom in init.base[("start", 0)].values():
            self._start_lit = init.solver_literal(atom.literal)
        for atom in init.base[("end", 0)].values():
            self._end_lit = init.solver_literal(atom.literal)
        for atom in init.base[("value", 0)].values():
            self._value_lit = init.solver_literal(atom.literal)
        for lit in sorted([self._start_lit, self._end_lit, self._value_lit]):
            init.add_watch(lit)
            init.add_watch(-lit)

    def propagate(self, assignment: Assignment, control: PropagateControl, changes):
        """
        Test propagate.
        """
        assert changes
        if assignment.is_false(self._value_lit) and assignment.is_false(self._end_lit):
            nogood = [self._start_lit, -self._end_lit, -self._value_lit]
            dl = assignment.decision_level
            result = control.add_nogood(nogood, tag=False, lock=self._lock)
            assert assignment.decision_level == dl
            assert not result

    def decide(self, assignment: Assignment, fallback: int) -> int:
        """
        Test decide.
        """
        if assignment.is_free(self._end_lit):
            return -self._end_lit
        if assignment.is_free(self._value_lit):
            return -self._value_lit
        return fallback


class InitPropagator(Propagator):
    # pylint: disable=too-few-public-methods
    """
    Test functions in PropagateInit.
    """

    def __init__(self, lib: Library):
        super().__init__()
        self._lib = lib
        self._init = False

    def init(self, assignment: Assignment, init: PropagateInit):
        """
        Test init.
        """
        self._init = True
        a = init.base[Function(self._lib, "a")]
        b = init.base[Function(self._lib, "b")]
        c = init.base[Function(self._lib, "c")]
        lit_a = init.solver_literal(a.literal)
        lit_b = init.solver_literal(b.literal)
        lit_c = init.solver_literal(c.literal)
        lit = init.add_literal(False)
        # a <=> b
        init.add_clause([lit_a, -lit])
        init.add_clause([lit, -lit_a])
        init.add_clause([lit_b, -lit])
        init.add_clause([lit, -lit_b])
        # c <=> {a, b} >= 2
        init.add_weight_constraint(
            lit_c, [(lit_a, 1), (lit_b, 1)], 2, WeightConstraintType.Equivalence
        )
        init.add_minimize(lit_a, -1, 0)
        assert init.propagate()
        assert len(init.base) == 3
        # test assignment
        assert assignment.value(lit_a) is None
        assert not assignment.is_true(lit_a)
        assert not assignment.is_false(lit_a)
        assert not assignment.is_fixed(lit_a)
        assert assignment.decision_level == 0
        assert lit_a in assignment
        assert not assignment.has_conflict
        assert not assignment.is_total
        assert assignment.root_level == 0
        assert len(assignment) == 5
        assert len(list(assignment)) == 5

    def attach(self, assignment: Assignment, control: PropagateControl):
        """
        Test attach.
        """
        assert assignment
        assert control
        assert self._init


class AddLiteralPropagator(Propagator):
    # pylint: disable=too-few-public-methods
    """
    Test adding literals while solving.
    """

    def __init__(self):
        super().__init__()
        self._lit = 0

    def add_lit(self, control: PropagateControl):
        """
        Add a literal.
        """
        self._lit = control.add_literal()
        assert not control.has_watch(self._lit)
        control.add_watch(self._lit)
        assert control.has_watch(self._lit)
        control.remove_watch(self._lit)
        assert not control.has_watch(self._lit)

    def attach(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Attach
        """
        assert self._lit == 0
        assert assignment
        self.add_lit(control)
        self._lit = 1

    def check(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Test check.
        """
        assert assignment
        if self._lit == 1:
            self.add_lit(control)


class HeuristicPropagator(Propagator):
    """
    Test decide.
    """

    def __init__(self, lib: Library):
        super().__init__()
        self._lib = lib
        self._lit_a = 0
        self._lit_b = 0

    def init(self, assignment: Assignment, init: PropagateInit):
        """
        Test init.
        """
        assert assignment
        a = init.base[Function(self._lib, "a")]
        b = init.base[Function(self._lib, "b")]
        self._lit_a = init.solver_literal(a.literal)
        self._lit_b = init.solver_literal(b.literal)

    def decide(self, assignment: Assignment, fallback: int) -> int:
        """
        Test decide.
        """
        if assignment.is_free(self._lit_a):
            return self._lit_a
        if assignment.is_free(self._lit_b):
            return -self._lit_b
        return fallback


class PropagateControlPropagator(Propagator):
    """
    Test functions in PropagateControl.
    """

    _lit_a: int
    _lib: Library

    def __init__(self, lib: Library):
        super().__init__()
        self._lit_a = 0
        self._lib = lib

    def init(self, assignment: Assignment, init: PropagateInit):
        """
        Test initialization.
        """
        ass = assignment
        init.check_mode = CheckMode.Off
        assert init.check_mode == CheckMode.Off
        assert init.number_of_threads == 1
        assert len(ass) >= 1
        a = init.base[Function(self._lib, "a")]
        self._lit_a = init.solver_literal(a.literal)
        assert self._lit_a in ass
        assert -self._lit_a in ass
        init.add_watch(-self._lit_a)

    def propagate(
        self,
        assignment: Assignment,
        control: PropagateControl,
        changes: Sequence[int],
    ):
        """
        Test propagation.
        """
        trail = assignment.trail
        lvl = assignment.decision_level
        assert -self._lit_a in changes
        assert lvl >= 1
        assert assignment.level(self._lit_a) >= 1
        assert len(trail) >= 1
        assert len(list(trail)) >= 1
        assert trail[trail.begin(lvl)] == -self._lit_a
        assert list(trail[trail.begin(lvl) : trail.end(lvl)]) == [-self._lit_a]
        assert assignment.decision(lvl) == -self._lit_a
        assert assignment.thread_id == 0
        assert control.has_watch(-self._lit_a)
        assert control.propagate()
        assert not control.add_clause([self._lit_a])

    def undo(self, assignment: Assignment, changes: Sequence[int]):
        """
        Test undo.
        """
        assert assignment
        assert -self._lit_a in changes


class ModePropagator(Propagator):
    """
    Test check/undo mode.
    """

    def __init__(self):
        super().__init__()
        self.num_check = 0
        self.num_undo = 0
        self.level = [0]

    def init(self, assignment: Assignment, init: PropagateInit):
        """
        Test init.
        """
        assert assignment
        init.check_mode = CheckMode.Fixpoint
        init.undo_mode = UndoMode.Always

        assert init.check_mode == CheckMode.Fixpoint
        assert init.undo_mode == UndoMode.Always

    def check(self, assignment: Assignment, control: PropagateControl) -> None:
        """
        Test propagate.
        """
        assert control
        self.num_check += 1
        dl = assignment.decision_level
        assert self.level[-1] <= dl
        if self.level[-1] != dl:
            self.level.append(dl)

    def undo(self, assignment: Assignment, changes: Sequence[int]) -> None:
        """
        Test undo.
        """
        assert assignment
        assert not changes
        self.num_undo += 1
        assert len(self.level) >= 2
        self.level.pop()


class TestPropagate:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the propagate module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()
        self._ctl = Control(self._lib, ["0"])

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._ctl = None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    @property
    def ctl(self) -> Control:
        """
        Get the control object.
        """
        assert self._ctl is not None
        return self._ctl

    def test_init(self):
        """
        Test PropagateInit and Assignment.
        """
        self.ctl.config.solve.opt_mode = "optN"
        self.ctl.parse_string("{a; b; c}.")
        self.ctl.ground()
        self.ctl.register_propagator(InitPropagator(self.lib))
        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [["a", "b", "c"]]

    def test_control(self):
        """
        Test basic propagate control functionality.
        """
        self.ctl.parse_string("{a}.")
        self.ctl.ground()
        self.ctl.register_propagator(PropagateControlPropagator(self.lib))
        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [["a"]]

    def test_mode(self):
        """
        Test check and undo mode.
        """
        tpm = ModePropagator()
        self.ctl.register_propagator(tpm)
        self.ctl.parse_string("{a; b; c}.")
        self.ctl.ground()
        assert self.ctl.solve().satisfiable
        assert tpm.level == [0]
        assert tpm.num_check >= 16
        assert tpm.num_undo >= 8

    def test_add_literal(self):
        """
        Test adding literals while solving.
        """
        self.ctl.ground()
        self.ctl.register_propagator(AddLiteralPropagator())
        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [[], [], [], []]

    def test_heuristic(self):
        """
        Test decide.
        """
        self.ctl.config.solve.models = "1"
        self.ctl.parse_string("{a;b}.")
        self.ctl.ground()
        self.ctl.register_propagator(HeuristicPropagator(self.lib))
        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == [["a"]]

    @pytest.mark.parametrize("locked", [False, True])
    def test_asserting(self, locked):
        """
        Test adding asserting clauses.
        """
        prop = AssertingPropagator(locked)
        self.ctl.register_propagator(prop)
        self.ctl.parse_string("start. {value}. {end}.")
        self.ctl.ground()
        assert self.ctl.solve().satisfiable

    def test_aiffb(self):
        """
        Test the example a iff b propagator.
        """
        prop = AIFFBPropagator()
        self.ctl.register_propagator(prop)
        self.ctl.parse_string("1 { a; b }.")
        self.ctl.ground()

        for n in [1, 3]:
            self.ctl.config.solve.parallel_mode = n
            mcb = MCB()
            assert self.ctl.solve(on_model=mcb).satisfiable
            assert prop.n_threads == n, "init called with wrong number of threads"
            assert not prop.errors
            assert mcb.symbols == [["a", "b"]]

    def test_aiffb_wc(self):
        """
        Test the example a iff b propagator with weight constraints.
        """
        prop = AIFFBPropagator()
        self.ctl.register_propagator(prop)
        self.ctl.parse_string("1 { a; b }.")
        self.ctl.ground()
        prop.weight_con = True

        mcb = MCB()
        assert self.ctl.solve(on_model=mcb).satisfiable
        assert not prop.errors
        assert mcb.symbols == [["a", "b"]]

    def test_exception_propagation(self):
        """
        Test python exception propagation.
        """
        prop = AIFFBPropagator()
        self.ctl.register_propagator(prop)
        self.ctl.parse_string("1 { a; b }.")
        self.ctl.ground()
        for n in [3, 1]:
            self.ctl.config.solve.parallel_mode = n
            prop.fail_thread = n - 1
            mcb = MCB()

            with pytest.raises(ValueError, match=f".* solver {n - 1}"):
                assert self.ctl.solve(on_model=mcb).satisfiable
                assert prop.n_threads == n, "init called with wrong number of threads"
