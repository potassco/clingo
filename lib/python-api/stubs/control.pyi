"""
Module containing the Control class responsible for grounding and solving.

# Examples

The first example shows the most straightforward way to ground and solve a
small test program:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("1 { a; b }.")
>>> ctl.ground()
>>> with ctl.solve(on_model=print) as hnd:
...     hnd.get()
a
```

The second example shows how to call functions from within a program:

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Number
>>> from clingo.control import Control
>>>
>>> class Context:
...     def __init__(self, lib):
...       self.lib = lib
...     def inc(self, x):
...         return Number(self.lib, x.number + 1)
...     def seq(self, x, y):
...         return [x, y]
...
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string(\"\"\"
... p(@inc(10)).
... q(@seq(1,2)).
... \"\"\")
>>> ctl.ground(context=Context(lib))
>>> with ctl.solve(on_model=print) as hnd:
...     print(hnd.get())
p(11) q(1) q(2)
SAT
```
"""

from __future__ import annotations

import typing

import clingo.ast
import clingo.backend
import clingo.base
import clingo.config
import clingo.core
import clingo.propagate
import clingo.solve
import clingo.stats
import clingo.symbol

__all__ = ["Control", "ControlMode"]

class ControlMode:
    """
    Available control modes.

    Members:

      Parse : Parse only.

      Rewrite : Parse and rewrite.

      Ground : Parse, rewrite, and ground.

      Solve : Parse, rewrite, ground, and solve.
    """

    Ground: typing.ClassVar[ControlMode]  # value = <ControlMode.Ground: 2>
    Parse: typing.ClassVar[ControlMode]  # value = <ControlMode.Parse: 0>
    Rewrite: typing.ClassVar[ControlMode]  # value = <ControlMode.Rewrite: 1>
    Solve: typing.ClassVar[ControlMode]  # value = <ControlMode.Solve: 3>
    __members__: typing.ClassVar[
        dict[str, ControlMode]
    ]  # value = {'Parse': <ControlMode.Parse: 0>, 'Rewrite': <ControlMode.Rewrite: 1>, 'Ground': <ControlMode.Ground: 2>, 'Solve': <ControlMode.Solve: 3>}
    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __init__(self, value: int) -> None: ...
    def __int__(self) -> int: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __setstate__(self, state: int) -> None: ...
    def __str__(self) -> str: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

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

    def ground(
        self,
        parts: (
            typing.Sequence[tuple[str, typing.Sequence[clingo.symbol.Symbol]]] | None
        ) = None,
        context: typing.Any = None,
    ) -> None:
        """
        Ground the given program parts.

        Args:
            parts:
                A list of tuples of part names and their symbolic arguments.
            context:
                An optional object with functions to call during grounding.
        """

    def join(self, program: clingo.ast.Program) -> None:
        """
        Join with the given non-ground logic program.

        Args:
            program:
                A non-ground logic program.
        """

    def main(
        self,
        parts: (
            typing.Sequence[
                typing.Sequence[tuple[str, typing.Sequence[clingo.symbol.Symbol]]]
            ]
            | None
        ) = None,
    ) -> None:
        """
        Ground and solver a logic program.

        This function proceeds as clingo calling the main function from a script if
        there is any.

        If the optional parts member is given, each of its elements corresponds to one
        incremental solving step where the elements' parts are grounded.

        Args:
            parts:
                A sequence of part sequences to ground and solve.
        """

    def observe(
        self, observer: clingo.backend.Observer, preprocess: bool = True
    ) -> None:
        """
        Inspect the ground program of the current step.

        Args:
            observer: The program observer to inspect the program.
            preprocess: Whether the program should be preprocessed first (default: true).
        """

    def parse_files(self, files: typing.Sequence[str]) -> None:
        """
        Parses the logic programs in the given files

        Args:
            files:
                The files to parse.
        """

    def parse_string(self, program: str) -> None:
        """
        Parses a logic program given as a string.

        Args:
            program:
                The logic program as string.
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
        on_model: typing.Callable[[clingo.solve.Model], bool | None] | None = None,
        on_stats: (
            typing.Callable[[clingo.stats.Stats, clingo.stats.Stats], None] | None
        ) = None,
        yield_: bool = False,
        async_: bool = False,
    ) -> clingo.solve.SolveHandle:
        """
        Solve the current ground program.

        Args:
            assumptions:
                List of `tuple[clingo.symbol.Symbol, bool]` or program literals (see
                `clingo.base.Atom.literal`) that serve as assumptions for the solve
                call, e.g., solving under assumptions
                `[(clingo.symbol.Function(lib, "a"), True)]` only admits answer sets
                that contain atom `a`.
            on_model:
                Optional callback for intercepting models. A `clingo.solve.Model`
                object is passed to the callback. The search can be interruped from the
                model callback by returning `False`.
            on_unsat:
                Optional callback to intercept lower bounds during optimization.
            on_stats:
                Optional callback to update stats.
                The step and accumulated stats are passed as arguments.
            on_finish:
                Optional callback called once search has finished. A
                `clingo.solve.SolveResult` is passed to the callback.
            on_core:
                Optional callback called with the assumptions that made a problem
                unsatisfiable.
            yield_:
                The resulting `clingo.solve.SolveHandle` is iterable yielding
                `clingo.solve.Model` objects.
            async_:
                The solve call and the method `clingo.solve.SolveHandle.resume`
                of the returned handle are non-blocking.

        Returns:
            A solve handle to control the search.

        Note:
            If this function is used in embedded Python code, you might want to start
            clingo using the `--outf=3` option to disable all output from clingo.

            Asynchronous solving is only available if thread support was enabled.
            Furthermore, the `on_model` and `on_finish` callbacks are called from
            another thread. To ensure that the methods can be called, make sure to not
            use any functions that block Python's GIL indefinitely.

            This function as well as blocking functions on the
            `clingo.solve.SolveHandle` release the GIL but are not thread-safe.

        See Also:
            clingo.solve: For more examples how to use this method.
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
    def stats(self) -> dict:
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

    def __iter__(self) -> typing.Iterator[str]:
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

    def items(self) -> typing.Iterator[tuple[str, clingo.symbol.Symbol]]:
        """
        Get an iterator over the items in the map.
        """

    def keys(self) -> typing.Iterator[str]:
        """
        Get get an iterator over the keys in the map.
        """

    def values(self) -> typing.Iterator[clingo.symbol.Symbol]:
        """
        Get an iterator over the values in the map.
        """
