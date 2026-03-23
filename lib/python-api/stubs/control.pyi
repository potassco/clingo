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

from __future__ import annotations

import collections.abc
import enum
import typing

import clingo.ast
import clingo.backend
import clingo.base
import clingo.config
import clingo.core
import clingo.ground
import clingo.propagate
import clingo.solve
import clingo.stats
import clingo.symbol

__all__: list[str] = ["Control", "ControlMode"]

class ControlMode(enum.IntEnum):
    """
    Available control modes.
    """

    Ground: typing.ClassVar[ControlMode]  # value = <ControlMode.Ground: 2>
    Parse: typing.ClassVar[ControlMode]  # value = <ControlMode.Parse: 0>
    Rewrite: typing.ClassVar[ControlMode]  # value = <ControlMode.Rewrite: 1>
    Solve: typing.ClassVar[ControlMode]  # value = <ControlMode.Solve: 3>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Control:
    """
    A control object for grounding and solving.
    """

    def __init__(
        self, lib: clingo.core.Library, options: typing.Sequence[str] = []
    ) -> None:
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
            minimize: Discard all minimize and weak constraints (default: False).
            project: Discard all previously added project  statements (default: False).
        """

    def ground(
        self,
        parts: (
            typing.Sequence[tuple[str, typing.Sequence[clingo.symbol.Symbol]]] | None
        ) = None,
        context: typing.Any = None,
    ) -> None:
        """
        Ground the given program parts.

        Non-ground logic programs must be added before calling this function.  Programs
        can define named sections using `#program.` directives.  These sections can be
        selectively grounded by specifying their name and binding parameters to
        symbols.

        Args:
            parts:
                        A sequence of tuples, each containing a section name and a sequence of
                        symbols. The name identifies the program section to ground, and the
                        symbols bind its parameters.  If `None`, the implicit base section
                        without arguments is grounded.
            context:
                        An optional object providing functions that can be called during
                        grounding.
        """

    def interrupt(self) -> None:
        """
        Interrupt the active solve call.

        This function is thread-safe. Prefer using `clingo.solve.SolveHandle.cancel` if
        possible.
        """

    def join(self, program: clingo.ast.Program) -> None:
        """
        Join with the given non-ground logic program.

        Args:
            program:
                A non-ground logic program.
        """

    def main(self) -> None:
        """
        Ground and solve a logic program based on the current control mode.

        This function serves as a high-level entry point for the default
        ground-and-solve process. It considers the current `ControlMode` and can
        dispatch execution to a script's `main` function if defined.

        If solving, the function proceeds as follows:

        1. Each set of program parts in `parts` is grounded sequentially.
        2. After grounding each set, solving is performed immediately.

        Before calling `main()`, the control object can be prepared by parsing
        programs, registering propagators, or performing other setup steps.
        """

    def observe(
        self, observer: clingo.backend.Observer, preprocess: bool = True
    ) -> None:
        """
        Inspect the ground program of the current step.

        Args:
            observer: The program observer to inspect the program.
                preprocess:
                        Whether the program should be preprocessed first (default: True).
        """

    def parse_files(self, files: typing.Sequence[str]) -> None:
        """
        Parses the logic programs in the given files

        Args:
            files:
                A list of file paths to parse.
        """

    def parse_string(self, program: str) -> None:
        """
        Parses a logic program given as a string.

        Args:
            program:
                The logic program as a string.
        """

    def register_propagator(self, propagator: clingo.propagate.Propagator) -> None:
        """
        Register the given propagator for theory propagation.

        See the `clingo.propagate` module for an example.

        Args:
            propagator:
                The propagator.
        """

    def solve(
        self,
        assumptions: typing.Sequence[tuple[clingo.symbol.Symbol, bool] | int] = [],
        on_model: (
            collections.abc.Callable[[clingo.solve.Model], bool | None] | None
        ) = None,
        on_unsat: collections.abc.Callable[[typing.Sequence[int]], None] | None = None,
        on_stats: (
            collections.abc.Callable[[clingo.stats.Stats, clingo.stats.Stats], None]
            | None
        ) = None,
        on_finish: (
            collections.abc.Callable[[clingo.solve.SolveResult], None] | None
        ) = None,
    ) -> clingo.solve.SolveResult:
        """
        Solve the current ground program.

        This function is semantically equivalent to the following `start_solve` call:

        ```python
        with self.start_solve(assumptions, on_model, on_unsat, on_stats, on_finish) as hnd:
            return hnd.get()
        ```

        Args:
            assumptions:
                        A list of assumptions.
            on_model:
                Optional callback to intercept models.
            on_unsat:
                Optional callback to intercept lower bounds during optimization.
            on_stats:
                Optional callback extend statistics.
            on_finish:
                        Optional callback called once search has finished.
        Returns:
            A `clingo.solve.SolveResult` representing the result of the search.
        """

    def start_ground(
        self,
        parts: (
            typing.Sequence[tuple[str, typing.Sequence[clingo.symbol.Symbol]]] | None
        ) = None,
        context: typing.Any = None,
        on_finish: (
            collections.abc.Callable[[clingo.ground.GroundResult], None] | None
        ) = None,
    ) -> clingo.ground.GroundHandle:
        """
        Ground the given program parts.

        Starts grounding in the background and returns a `clingo.ground.GroundHandle`
        to the running grounding. See `Control.ground` for details on grounding program
        parts.

        Args:
            parts:
                        A sequence of parts to ground.
            context:
                        An optional object providing functions that can be called during
                        grounding.
            on_finish:
                An optional callback called once grounding has finished.
        """

    def start_solve(
        self,
        assumptions: typing.Sequence[tuple[clingo.symbol.Symbol, bool] | int] = [],
        on_model: (
            collections.abc.Callable[[clingo.solve.Model], bool | None] | None
        ) = None,
        on_unsat: collections.abc.Callable[[typing.Sequence[int]], None] | None = None,
        on_stats: (
            collections.abc.Callable[[clingo.stats.Stats, clingo.stats.Stats], None]
            | None
        ) = None,
        on_finish: (
            collections.abc.Callable[[clingo.solve.SolveResult], None] | None
        ) = None,
        yield_: bool = False,
        async_: bool = False,
    ) -> clingo.solve.SolveHandle:
        """
        Solve the current ground program.

        This function runs the solver on the current ground program, optionally  using
        assumptions, callbacks, or asynchronous execution. It returns a  `SolveHandle`,
        allowing interaction with the solving process.

        If asynchronous solving (`async_`) is enabled, the function returns
        immediately, and solving runs in the background. Otherwise, the function
        blocks until solving is complete.

        Args:
            assumptions:
                        A list of assumptions that constrain this search. Each assumption is
                        either a `tuple[clingo.symbol.Symbol, bool]` indicating an atom's truth
                        value or a program literal (see `clingo.base.Atom.literal`). For
                        example, using `[(clingo.symbol.Function(lib, "a"), True)]` only admits
                        answer sets that contain atom `a`.
            on_model:
                Optional callback that receives a `clingo.solve.Model` object when
                a model is found. Returning `False` from the callback stops solving.
            on_unsat:
                Optional callback to intercept lower bounds during optimization.
            on_stats:
                Optional callback that receives statistics updates after each step.
                Two `clingo.stats.Stats` objects are passed: step-specific and
                accumulated stats.
            on_finish:
                        Optional callback called once search has finished. A
                        `clingo.solve.SolveResult` is passed to the callback.
            yield_:
                        If `True`, the returned `clingo.solve.SolveHandle` is iterable,
                        yielding  `clingo.solve.Model` objects during solving.
            async_:
                If `True`, solving runs asynchronously in a separate thread.
                Note: Callbacks (`on_model`, `on_stats`, etc.) will also be called
                from a separate thread.
        Returns:
            A `clingo.solve.SolveHandle` to control the search.

        Notes:
                Asynchronous solving requires compiling clingo with thread support.
                Blocking methods on `SolveHandle` release the GIL but are not thread-safe.

        See Also:
            clingo.solve: Contains examples on using this function.
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
        Write the current logic programs to the given file.

        If append is true, a file will be created if none exists yet. If preamble is
        None, then the aspif preamble is written for newly created files and omitted
        for existing files.

        If path is None, clingo's internal output buffer is used instead of writing to
        a file. The buffer content can be accessed via `Control.buffer`. Note that the
        buffer is flushed to stdout in application mode. Explicitly set the preamble
        flag to control when to write the preamble as it is always written if it is
        None in this mode.

        Args:
            path:
                The optional path to write the program to.
            append:
                Whether to append to an existing file.
            preamble:
                Whether to write the aspif preamble.
            preprocess:
                Whether to preprocess the program before writing.
        """

    @property
    def backend(self) -> clingo.backend.BackendManager:
        """
        Get a backend manager to extend the ground program.
        """

    @property
    def base(self) -> clingo.base.Base:
        """
        Get the atom/term bases of the program.
        """

    @property
    def buffer(self) -> str:
        """
        The content of the output buffer.
        """

    @property
    def config(self) -> clingo.config.Config:
        """
        Get the solver config.
        """

    @property
    def const_map(self) -> typing.Mapping[str, clingo.symbol.Symbol]:
        """
        Get the map of constants defined by `#const` directives.
        """

    @property
    def mode(self) -> ControlMode:
        """
        Get the application mode.
        """

    @property
    def parts(
        self,
    ) -> typing.Sequence[tuple[str, typing.Sequence[clingo.symbol.Symbol]]] | None:
        """
        Get/set the program parts to ground.
        """

    @parts.setter
    def parts(
        self,
        arg1: typing.Sequence[tuple[str, typing.Sequence[clingo.symbol.Symbol]]] | None,
    ) -> None: ...
    @property
    def profile(self) -> list:
        """
        Get the profiling information as a list of profile nodes.

        Each node is a dictionary with keys such as "type", "key", "depth", "nested",
        "children", etc. Returns a list of top-level profile nodes representing the
        profiling tree.

        The result is directly convertible to JSON using Python's `json` module.
        """

    @property
    def stats(self) -> clingo.stats.StatsView:
        """
        Get the solver stats.
        """

class _ConstMap:
    """
    The map from constants defined by #const directives.
    """

    def __contains__(self, key: str) -> bool:
        """
        Check if the map contains the given key.
        """

    def __getitem__(self, key: str) -> clingo.symbol.Symbol:
        """
        Get the value for the given key.
        """

    def __iter__(self) -> collections.abc.Iterator[str]:
        """
        Get an iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number elements in the map.
        """

    def get(
        self, key: str, default: clingo.symbol.Symbol | None = None
    ) -> clingo.symbol.Symbol | None:
        """
        Get the value for the given key or the default if absent.
        """

    def items(self) -> collections.abc.Iterator[tuple[str, clingo.symbol.Symbol]]:
        """
        Get an iterator over the items in the map.
        """

    def keys(self) -> collections.abc.Iterator[str]:
        """
        Get an iterator over the keys in the map.
        """

    def values(self) -> collections.abc.Iterator[clingo.symbol.Symbol]:
        """
        Get an iterator over the values in the map.
        """
