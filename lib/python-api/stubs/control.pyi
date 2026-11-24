"""
Module containing the Control class responsible for grounding and solving.

# Example

The example shows the most straightforward way to ground and solve a
small test program:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("1 { a; b }.")
>>> ctl.ground()
>>> print(ctl.solve(on_model=print))
a
SAT
```
"""

from collections.abc import (
    Callable,
    ItemsView,
    Iterator,
    KeysView,
    Mapping,
    Sequence,
    ValuesView,
)
from enum import IntEnum
from typing import Any, ClassVar, TypeVar, overload

from clingo.ast import Program
from clingo.backend import BackendManager, Observer
from clingo.base import Base
from clingo.config import Config
from clingo.core import EventLoop, Library
from clingo.ground import AsyncGroundHandle, GroundHandle, GroundResult
from clingo.propagate import Propagator
from clingo.solve import AsyncSolveHandle, Model, SolveHandle, SolveResult
from clingo.stats import Stats, StatsView
from clingo.symbol import Symbol

__all__: list[str] = ["ConstMap", "Control", "ControlMode"]

_T = TypeVar("_T")

class ControlMode(IntEnum):
    """
    Enumeration of control modes.
    """

    Ground = 2
    """
    Parse, rewrite, and ground the logic program.
    """
    Parse = 0
    """
    Only parse the logic program.
    """
    Rewrite = 1
    """
    Parse and rewrite the logic program.
    """
    Solve = 3
    """
    Parse, rewrite, ground, and solve the logic program.
    """

class ConstMap(Mapping[str, Symbol]):
    """
    The map of constants defined by `#const` directives.

    Implements a mapping from constant names to their values.
    """

    def __contains__(self, key: object, /) -> bool:
        """
        Check if the map contains the given key.

        Args:
            key: The key to check.

        Returns:
            Whether the key is present in the map.
        """

    def __eq__(self, other: object, /) -> bool:
        """
        Check whether the map is equal to the given object.

        The map is equal to another mapping with the same items.

        Args:
            other: The object to compare with.

        Returns:
            Whether the map is equal to the object.
        """
    __hash__: ClassVar[None]  # type: ignore[assignment]
    """
    The map is not hashable because it compares by value.
    """

    def __getitem__(self, key: str, /) -> Symbol:
        """
        Get the value for the given key.

        Args:
            key: The key to look up.

        Returns:
            The value for the given key.
        """

    def __iter__(self) -> Iterator[str]:
        """
        Get an iterator over the keys in the map.

        Returns:
            An iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number of elements in the map.

        Returns:
            The number of elements in the map.
        """

    @overload
    def get(self, key: str, default: None = None) -> Symbol | None:
        """
        Get the value for the given key or `None` if absent.

        Args:
            key: The key to look up.
            default: The value to return if the key is absent.

        Returns:
            The value for the given key or `None` if absent.
        """

    @overload
    def get(self, key: str, default: Symbol | _T) -> Symbol | _T:
        """
        Get the value for the given key or the default if absent.

        Args:
            key: The key to look up.
            default: The value to return if the key is absent.

        Returns:
            The value for the given key or the default if absent.
        """

    def get(self, key: str, default: _T | None = None) -> Symbol | _T | None:
        """
        Get the value for the given key or the default if absent.

        If the key is not present in the map, the default value is returned.
        If no default value is given, `None` is returned.

        Args:
            key: The key to look up.
            default: The value to return if the key is absent.

        Returns:
            The value for the given key or the default if absent.
        """

    def items(self) -> ItemsView[str, Symbol]:
        """
        Get a view on the items in the map.

        Returns:
            A view on the items in the map.
        """

    def keys(self) -> KeysView[str]:
        """
        Get a view on the keys in the map.

        Returns:
            A view on the keys in the map.
        """

    def values(self) -> ValuesView[Symbol]:
        """
        Get a view on the values in the map.

        Returns:
            A view on the values in the map.
        """

class Control:
    """
    A control object for grounding and solving.
    """

    def __init__(self, lib: Library, options: Sequence[str] = []) -> None:
        """
        Construct a control object.

        Args:
            lib: The library storing symbols and scripts.
            options: The command line options to initialize the control object.
        """

    def discard(self, minimize: bool = False, project: bool = False) -> None:
        """
        Discard statements of the selected types.

        Args:
            minimize: Discard all minimize and weak constraints.
            project: Discard all previously added project statements.
        """

    def ground(
        self,
        parts: Sequence[tuple[str, Sequence[Symbol]]] | None = None,
        context: Any = None,
    ) -> None:
        """
        Ground the given program parts.

        Non-ground logic programs must be added before calling this function.
        Programs can define named sections using `#program` directives. These
        sections can be selectively grounded by specifying their name and binding
        parameters to symbols.

        Args:
            parts: A sequence of tuples, each containing a section name and a
                sequence of symbols. The name identifies the program section to
                ground, and the symbols bind its parameters. If `None`, the parts
                set via `clingo.control.Control.parts` are grounded or, if there
                are none, the implicit base section without arguments.
            context: An optional object providing functions that can be called
                during grounding.
        """

    def ground_async(
        self,
        loop: EventLoop,
        parts: Sequence[tuple[str, Sequence[Symbol]]] | None = None,
        context: Any = None,
    ) -> AsyncGroundHandle:
        """
        Ground the given program parts in the background and return an awaitable handle.

        Awaiting the handle returns the ground result. See
        `clingo.control.Control.ground` for details on grounding program parts.

        Args:
            loop: The event loop of the awaiting task.
            parts: The program parts to ground.
            context: An optional object providing functions that can be called
                during grounding. They are called on the grounding thread.

        Returns:
            A handle to await grounding.

        Notes:
            Asynchronous grounding requires compiling clingo with thread
            support.
        """

    def interrupt(self) -> None:
        """
        Interrupt the active solve call.

        This function is thread-safe. Prefer using `clingo.solve.SolveHandle.cancel`
        if possible.
        """

    def join(self, program: Program) -> None:
        """
        Extend the logic program with the given non-ground program.

        After extending the logic program, the corresponding program parts are
        typically grounded with `clingo.control.Control.ground`.

        Args:
            program: The non-ground program to add.
        """

    def main(self) -> None:
        """
        Ground and solve a logic program based on the current control mode.

        This function serves as a high-level entry point for the default
        ground-and-solve process. It proceeds as follows:

        1. If a script defines a `main` function, execution is dispatched to it.
        2. Otherwise, in `ControlMode.Parse` and `ControlMode.Rewrite`, the parsed
           or rewritten program is printed.
        3. Otherwise, if the program includes `<incmode>`, incremental grounding and
           solving is performed.
        4. Otherwise, the program parts in `clingo.control.Control.parts` (or the
           base part if none are set) are grounded, followed by a single solve call
           in `ControlMode.Solve`.

        Before calling `main()`, the control object can be prepared by parsing
        programs, registering propagators, or performing other setup steps.
        """

    def observe(self, observer: Observer, preprocess: bool = True) -> None:
        """
        Inspect the ground program of the current step.

        Args:
            observer: The program observer to inspect the program.
            preprocess: Whether the program should be preprocessed first.
        """

    def parse_files(self, files: Sequence[str]) -> None:
        """
        Parse the logic programs in the given files.

        Args:
            files: The paths of the files to parse.
        """

    def parse_string(self, program: str) -> None:
        """
        Parse a logic program given as a string.

        Args:
            program: The logic program as a string.
        """

    def register_propagator(self, propagator: Propagator) -> None:
        """
        Register the given propagator for theory propagation.

        See the `clingo.propagate` module for an example.

        Args:
            propagator: The propagator to register.
        """

    def solve(
        self,
        assumptions: Sequence[tuple[Symbol, bool] | int] = [],
        on_model: Callable[[Model], bool | None] | None = None,
        on_unsat: Callable[[Sequence[int]], None] | None = None,
        on_stats: Callable[[Stats, Stats], None] | None = None,
        on_finish: Callable[[SolveResult], None] | None = None,
    ) -> SolveResult:
        """
        Solve the current ground program.

        This function is semantically equivalent to the following `start_solve`
        call:

        ```python
        with self.start_solve(assumptions, on_model, on_unsat, on_stats, on_finish) as hnd:
            return hnd.get()
        ```

        See `clingo.control.Control.start_solve` for a detailed description of the
        arguments.

        Args:
            assumptions: The assumptions that constrain the search.
            on_model: An optional callback to intercept models.
            on_unsat: An optional callback to intercept lower bounds during
                optimization.
            on_stats: An optional callback to extend the statistics.
            on_finish: An optional callback called once the search has finished.

        Returns:
            The result of the search.
        """

    def solve_async(
        self,
        loop: EventLoop,
        assumptions: Sequence[tuple[Symbol, bool] | int] = [],
        on_model: Callable[[Model], bool | None] | None = None,
        on_unsat: Callable[[Sequence[int]], None] | None = None,
        on_stats: Callable[[Stats, Stats], None] | None = None,
        yield_: bool = False,
    ) -> AsyncSolveHandle:
        """
        Solve the current ground program in the background and return an awaitable handle.

        Awaiting the handle returns the solve result. With `yield_=True`, the
        models can be iterated with `async for` before awaiting the result.
        Otherwise, models can be intercepted with the `on_model` callback.

        The callbacks are called on the solver thread. Their arguments are only
        valid during the call and they must not interact with the event loop.
        See `clingo.control.Control.start_solve` for a description of the
        callbacks.

        Args:
            loop: The event loop of the awaiting task.
            assumptions: The assumptions that constrain the search.
            on_model: An optional callback to intercept models.
            on_unsat: An optional callback to intercept lower bounds during
                optimization.
            on_stats: An optional callback to extend the statistics.
            yield_: Whether models can be iterated with `async for`.

        Returns:
            A handle to await the search.

        Notes:
            Asynchronous solving requires compiling clingo with thread support.

        See Also:
            `clingo.solve.AsyncSolveHandle`
        """

    def start_ground(
        self,
        parts: Sequence[tuple[str, Sequence[Symbol]]] | None = None,
        context: Any = None,
        on_finish: Callable[[GroundResult], None] | None = None,
    ) -> GroundHandle:
        """
        Start grounding the given program parts in the background.

        See `clingo.control.Control.ground` for details on grounding program
        parts.

        Args:
            parts: The program parts to ground.
            context: An optional object providing functions that can be called
                during grounding.
            on_finish: An optional callback called once grounding has finished.
                Exceptions raised by the callback are not propagated but reported
                via `sys.unraisablehook`.

        Returns:
            A handle to interact with the running grounding.
        """

    def start_solve(
        self,
        assumptions: Sequence[tuple[Symbol, bool] | int] = [],
        on_model: Callable[[Model], bool | None] | None = None,
        on_unsat: Callable[[Sequence[int]], None] | None = None,
        on_stats: Callable[[Stats, Stats], None] | None = None,
        on_finish: Callable[[SolveResult], None] | None = None,
        yield_: bool = False,
        async_: bool = False,
    ) -> SolveHandle:
        """
        Start solving the current ground program.

        This function runs the solver on the current ground program, optionally
        using assumptions, callbacks, or asynchronous execution. It returns a
        `clingo.solve.SolveHandle`, allowing interaction with the solving process.

        If asynchronous solving (`async_`) is enabled, the function returns
        immediately, and solving runs in the background. Otherwise, if yielding
        (`yield_`) is enabled, models are computed one at a time when they are
        requested via the handle. If neither is enabled, the function blocks until
        solving is complete.

        Args:
            assumptions: The assumptions that constrain the search. Each assumption
                is either a `tuple[clingo.symbol.Symbol, bool]` indicating an atom's
                truth value or a program literal (see `clingo.base.Atom.literal`).
                For example, using `[(clingo.symbol.Function(lib, "a"), True)]` only
                admits answer sets that contain atom `a`.
            on_model: An optional callback that receives a `clingo.solve.Model`
                object when a model is found. Returning `False` from the callback
                stops solving.
            on_unsat: An optional callback to intercept lower bounds during
                optimization.
            on_stats: An optional callback that receives statistics updates after
                each step. Two `clingo.stats.Stats` objects are passed: the
                step-specific and the accumulated stats.
            on_finish: An optional callback called once the search has finished.
                A `clingo.solve.SolveResult` is passed to the callback. Exceptions
                raised by the callback are not propagated but reported via
                `sys.unraisablehook`.
            yield_: If `True`, the returned `clingo.solve.SolveHandle` is iterable,
                yielding `clingo.solve.Model` objects during solving.
            async_: If `True`, solving runs asynchronously in a separate thread.
                Callbacks (`on_model`, `on_stats`, etc.) are then also called from a
                separate thread.

        Returns:
            A handle to control the search.

        Notes:
            Asynchronous solving requires compiling clingo with thread support.
            Blocking methods on `clingo.solve.SolveHandle` release the GIL but are
            not thread-safe.

        See Also:
            `clingo.solve`
        """

    def write_aspif(
        self,
        path: str | None = None,
        symbols: bool = False,
        append: bool = False,
        preamble: bool | None = None,
        preprocess: bool = True,
    ) -> None:
        """
        Write the current logic program in aspif format.

        If `path` is `None`, the program is written to clingo's internal output
        buffer, which is available via `clingo.control.Control.buffer`. Otherwise,
        it is written to the given file, which is created if it does not exist.

        If `append` is false, an existing file is overwritten. Output to the buffer
        is always appended to its current content.

        If `preamble` is `None`, the aspif preamble is written unless the program is
        appended to existing content, i.e., unless `append` is true and the file
        exists or the buffer is non-empty. In application mode, the buffer is
        printed to stdout and cleared after each major operation like grounding, so
        the preamble flag should be set explicitly in this case.

        Args:
            path: The optional path to write the program to.
            symbols: Whether to also write the symbols of the program.
            append: Whether to append to existing content.
            preamble: Whether to write the aspif preamble.
            preprocess: Whether to preprocess the program before writing.
        """

    @property
    def backend(self) -> BackendManager:
        """
        Get a backend manager to extend the ground program.
        """

    @property
    def base(self) -> Base:
        """
        Get the atom/term bases of the program.
        """

    @property
    def buffer(self) -> str:
        """
        Get the content of the output buffer.

        The buffer receives the output of `clingo.control.Control.write_aspif`
        without a path and the program printed by `clingo.control.Control.main` in
        `ControlMode.Parse` and `ControlMode.Rewrite`. It is only filled for
        standalone control objects; control objects of an application regularly
        flush it to stdout.
        """

    @property
    def config(self) -> Config:
        """
        Get the solver config.
        """

    @property
    def const_map(self) -> ConstMap:
        """
        Get the map of constants defined by `#const` directives.
        """

    @property
    def mode(self) -> ControlMode:
        """
        Get the control mode, which is set via the `--mode` option.
        """

    @property
    def parts(self) -> list[tuple[str, list[Symbol]]] | None:
        """
        Get or set the program parts to ground.

        The parts are grounded by `clingo.control.Control.ground` if no parts are
        passed explicitly. If `None`, no parts are set.
        """

    @parts.setter
    def parts(self, value: Sequence[tuple[str, Sequence[Symbol]]] | None) -> None: ...
    @property
    def profile(self) -> list[dict[str, Any]]:
        """
        Get the profiling information as a list of profile nodes.

        Profiling data is only collected if profiling is enabled via the `--profile`
        option. Otherwise, the list is empty.

        Each node is a dictionary. Internal nodes have the keys "type"
        (`"internal"`), "key", "nested", and "children". The key is the string
        representation of an expression in the logic program, nested indicates
        whether the node's times are already included in its parent node, and
        children is the list of child nodes. Leaf nodes have the keys "type"
        (`"leaf"`), "profile_type" (`"step"` for the last step or `"accu"` for all
        steps), "matches", "instances", "time_instantiate", and "time_propagate",
        where the times are given in nanoseconds.

        The result is directly convertible to JSON using Python's `json` module.
        """

    @property
    def stats(self) -> StatsView:
        """
        Get the solver stats.

        The stats describe the last solve call.
        """
