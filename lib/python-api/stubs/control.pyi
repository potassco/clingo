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

__all__ = ["Control"]

class Control:
    """
    A control object for grounding and solving.
    """

    def __init__(self, lib: clingo.core.Library, options: list[str] = []) -> None:
        """
        Construct a control object.

        Args:
          lib: The library storing symbols and scripts.
          options: The command line options to initialize the control object.
        """

    def ground(
        self,
        parts: list[tuple[str, list[clingo.symbol.Symbol]]] | None = None,
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

    def main(self) -> None:
        """
        Ground and solver a logic program.

        This function proceeds as clingo calling the main function from a script if
        there is any.
        """

    def observe(self, observer: clingo.backend.Observer) -> None:
        """
        Inspect the ground program of the current step.

        Args:
            observer: The program observer to inspect the program.
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
        assumptions: list[tuple[clingo.symbol.Symbol, bool] | int] = [],
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
        The content of the output bufer.
        """

    @property
    def config(self) -> clingo.config.Config:
        """
        Get the solver config.
        """

    @property
    def stats(self) -> dict:
        """
        Get the solver stats.
        """
