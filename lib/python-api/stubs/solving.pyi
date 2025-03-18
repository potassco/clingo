"""
Functions and classes related to solving.

Examples
--------

The examples below show various ways to intercept models. The asynchronous
variants leave room for additional computation before calling blocking functions
`like SolveHandle.get` or `SolveHandle.model`.

The following example shows how to intercept models with a callback:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(on_model=print) as hnd:
    ...     print(hnd.get())
    ...
    a
    b
    SAT

The following example shows how to yield models:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(yield_=True) as hnd:
    ...     for m in hnd:
    ...         print(m)
    ...     print(hnd.get())
    ...
    a
    b
    SAT

The following example shows how to solve asynchronously:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(on_model=print, async_=True) as hnd:
    ...     print(hnd.get())
    ...
    a
    b
    SAT

This example shows how to solve both iteratively and asynchronously:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(yield_=True, async_=True) as hnd:
    ...     while mdl := hnd.model():
    ...         hnd.resume()
    ...     print(hnd.get())
    ...
    b
    a
    SAT
"""

from __future__ import annotations

import typing

import clingo.symbol

__all__ = ["Model", "SolveHandle", "SolveResult"]

class Model:
    """
    A view on the solver's current solution.
    """

    def __str__(self) -> str:
        """
        Get a string representation of the model.
        """

    def symbols(
        self,
        shown: bool = False,
        atoms: bool = False,
        terms: bool = False,
        theory: bool = False,
    ) -> list[clingo.symbol.Symbol]:
        """
        Get the symbols in the model.

        Args:
          shown: Include shown atoms and terms.
          atoms: Include all true atoms including hidden ones.
          terms: Include shown terms.
          theory: Include terms added by external theories.
        """

class SolveHandle:
    """
    An object to interact with a running search.

    It can be used to control solving, like, retrieving models or cancelling a
    search.

    A SolveHandle is a context manager and must be used with Python's with
    statement.

    Blocking functions in this object release the GIL. They are not thread-safe
    though.

    See also: `clingo.control.Control.solve`
    """

    def __enter__(self) -> SolveHandle:
        """
        Start the search.
        """

    def __exit__(
        self, arg0: type | None, arg1: typing.Any | None, arg2: typing.Any | None
    ) -> None:
        """
        Stop the search closing the handle.
        """

    def __iter__(self) -> typing.Iterator[Model]:
        """
        Get an iterator over the models.
        """

    def cancel(self) -> None:
        """
        Cancel the running search.

        See also: `clingo.control.Control.interrupt`
        """

    def core(self) -> list[int]:
        """
        Get the subset of assumptions that made the problem unsatisfiable.
        """

    def get(self) -> SolveResult:
        """
        Get the solve result.

        This is always the last function that should be called on a handle to ensure
        that the search is properly terminated. It might be preceded by a call to
        cancel to stop a running search.
        """

    def last(self) -> Model | None:
        """
        Get the last computed model if there is any.

        If the search is not completed yet or the problem is unsatisfiable, the
        function returns `None`.
        """

    def model(self) -> Model | None:
        """
        Get the current model if there is any.
        """

    def resume(self) -> None:
        """
        Discards the last model and starts searching for the next one.

        If the search has been started asynchronously, this function starts the search
        in the background.
        """

    def wait(self, timeout: float | None = None) -> bool:
        """
        Wait for solve call to finish or the next result with an optional timeout.

        If a timeout is given, the behavior of the function changes depending on the
        sign of the timeout. If a postive timeout is given, the function blocks for the
        given amount time or until a result is ready. If the timeout is negative, the
        function will block until a result is ready, which also corresponds to the
        behavior of the function if no timeout is given. A timeout of zero can be used
        to poll if a result is ready.

        Args:
          timeout:
            If a timeout is given, the function blocks for at most timeout seconds.

        Returns:
          Indicates whether the solve call has finished or the next result is ready.
        """

class SolveResult:
    """
    A solve result captures information about a solve call.
    """

    def __str__(self) -> str:
        """
        Get a string representation of the solve result.
        """

    @property
    def exhausted(self) -> bool:
        """
        Whether all models have been enumerated.
        """

    @property
    def interrupted(self) -> bool:
        """
        Whether the search was interrupted.
        """

    @property
    def satisfiable(self) -> bool:
        """
        Whether there was at least one model.
        """

    @property
    def unknown(self) -> bool:
        """
        Whether the satisfiablity could be determined.
        """

    @property
    def unsatisfiable(self) -> bool:
        """
        Whether there was no model.
        """
