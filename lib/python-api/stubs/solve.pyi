"""
Functions and classes related to solving.

Examples
--------

The examples below show various ways to intercept models. The asynchronous
variants leave room for additional computation before calling blocking
functions like `clingo.solve.SolveHandle.get` or
`clingo.solve.SolveHandle.model`.

The following example shows how to intercept models with a callback:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> print(ctl.solve(on_model=print))
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
    >>> with ctl.start_solve(yield_=True) as hnd:
    ...     for mdl in hnd:
    ...         print(mdl)
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
    >>> with ctl.start_solve(on_model=print, async_=True) as hnd:
    ...     # other work: poll with wait or use on_finish
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
    >>> with ctl.start_solve(yield_=True, async_=True) as hnd:
    ...     # other work: poll with wait or use on_finish
    ...     while mdl := hnd.model():
    ...         print(mdl)
    ...         hnd.resume()
    ...         # other work: poll with wait or use on_finish
    ...     print(hnd.get())
    ...
    b
    a
    SAT

The following example shows how to solve with asyncio:

    >>> import asyncio
    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> async def main():
    ...     lib = Library()
    ...     ctl = Control(lib, ["0"])
    ...     ctl.parse_string("1 { a; b } 1.")
    ...     loop = asyncio.get_running_loop()
    ...     await ctl.ground_async(loop)
    ...     async with ctl.solve_async(loop, yield_=True) as hnd:
    ...         async for mdl in hnd:
    ...             print(mdl)
    ...         print(await hnd)
    ...
    >>> asyncio.run(main())
    b
    a
    SAT
"""

from collections.abc import Generator, Iterator, Sequence
from enum import IntEnum
from types import TracebackType
from typing import Any, Self

from clingo.base import Base
from clingo.symbol import Symbol

__all__: list[str] = [
    "AsyncSolveHandle",
    "Model",
    "ModelType",
    "SolveControl",
    "SolveHandle",
    "SolveResult",
]

class ModelType(IntEnum):
    """
    Enumeration of model types.
    """

    BraveConsequences = 1
    CautiousConsequences = 2
    StableModel = 0

class SolveControl:
    """
    A control object to add clauses while solving.
    """

    def add_clause(self, clause: Sequence[tuple[Symbol, bool] | int]) -> None:
        """
        Add a clause that applies to the current solving step during the search.

        Args:
          clause: The literals of the clause.
        """

    def add_nogood(self, nogood: Sequence[tuple[Symbol, bool] | int]) -> None:
        """
        Add a nogood that applies to the current solving step during the search.

        Args:
          nogood: The literals of the nogood.
        """

    @property
    def base(self) -> Base:
        """
        Get the atom/term bases of the program.
        """

class Model:
    """
    A view on the solver's current solution.

    Converting a model to a string yields its shown symbols separated by
    commas.
    """

    def contains(self, atom: Symbol) -> bool:
        """
        Check if the model contains the given atom.

        Args:
            atom: The atom to look up.
        Returns:
            Whether the atom is contained.
        """

    def extend(self, symbols: Sequence[Symbol]) -> None:
        """
        Extend a model with the given symbols.

        This only has an effect if there is an underlying clingo application, which
        will print the added symbols.

        Args:
            symbols: The symbols to add to the model.
        """

    def is_consequence(self, literal: int) -> bool | None:
        """
        Check if the given program literal is a consequence.

        The function returns `True`, `False`, or `None` if the literal is a
        consequence, not a consequence, or it is not yet known whether it is a
        consequence, respectively.

        While enumerating cautious or brave consequences, there is partial information
        about which literals are consequences. The current state of a literal can be
        requested using this function. If this function is used during normal model
        enumeration, it simply returns whether the literal is true or false in the
        current model.

        Args:
            literal: The given program literal.

        Returns:
            Whether the given program literal is a consequence.
        """

    def is_true(self, literal: int) -> bool:
        """
        Check if the given program literal is true.

        Args:
            literal: The given program literal.

        Returns:
            Whether the given program literal is true.
        """

    def symbols(
        self,
        shown: bool = False,
        atoms: bool = False,
        terms: bool = False,
        theory: bool = False,
    ) -> Sequence[Symbol]:
        """
        Get the symbols in the model.

        Args:
            shown: Include shown atoms and terms.
            atoms: Include all true atoms, including hidden ones.
            terms: Include shown terms.
            theory: Include terms added by external theories.

        Returns:
            A sequence of symbols present in the model.
        """

    @property
    def control(self) -> SolveControl:
        """
        Get the associated solve control object.
        """

    @property
    def cost(self) -> Sequence[int]:
        """
        Return a sequence of integer cost values of the model.
        """

    @property
    def number(self) -> int:
        """
        Get the running number of a model.
        """

    @property
    def optimality_proven(self) -> bool:
        """
        Whether the optimality of the model has been proven.
        """

    @property
    def priorities(self) -> Sequence[int]:
        """
        Get the associated priorities of the cost values.
        """

    @property
    def thread_id(self) -> int:
        """
        Get the thread/solver id the model was found in.
        """

    @property
    def type(self) -> ModelType:
        """
        Get the type of a model.
        """

class SolveResult:
    """
    A solve result captures information about a solve call.

    Converting a result to a string yields `SAT`, `UNSAT`, or `UNKNOWN`.
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
        Whether at least one model was found.
        """

    @property
    def unknown(self) -> bool:
        """
        Whether it is unknown if there is a model.
        """

    @property
    def unsatisfiable(self) -> bool:
        """
        Whether there was no model.
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

    See also: `clingo.control.Control.solve`, `clingo.control.Control.start_solve`
    """

    def __enter__(self) -> Self:
        """
        Returns self.

        The search has already been started when the handle is created.

        Returns:
            Returns the handle itself.
        """

    def __exit__(
        self,
        type: type[BaseException] | None,
        value: BaseException | None,
        traceback: TracebackType | None,
        /,
    ) -> None:
        """
        Stop the search closing the handle.

        Args:
            type: The exception type, if an exception occurred.
            value: The exception instance, if an exception occurred.
            traceback: The traceback object, if an exception occurred.
        """

    def __iter__(self) -> Iterator[Model]:
        """
        Get an iterator over the models.
        """

    def close(self) -> None:
        """
        Stop the search closing the handle.

        See also: `clingo.solve.SolveHandle.__exit__`
        """

    def cancel(self) -> None:
        """
        Cancel the running search.

        See also: `clingo.control.Control.interrupt`
        """

    def interrupt(self) -> None:
        """
        Request the search to stop without waiting for it.

        Use `clingo.solve.SolveHandle.wait` or `clingo.solve.SolveHandle.get`
        to wait until the search has stopped. The result is then interrupted
        unless the search finished before. Like `clingo.solve.SolveHandle.resume`,
        this invalidates the current model.
        """

    def core(self) -> Sequence[int]:
        """
        Get the subset of assumptions that made the problem unsatisfiable.
        """

    def get(self) -> SolveResult:
        """
        Get the solve result.

        This is always the last function to be called on a handle to ensure that the
        search is properly terminated. It might be preceded by a call to cancel to stop
        the search.
        """

    def last(self) -> Model | None:
        """
        Get the last computed model, if any.

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
        Wait for the solve call to finish or the next result with an optional timeout.

        If a timeout is provided, the function blocks for the given duration or until a
        result is ready. A positive timeout blocks for that amount of time. A negative
        timeout blocks until a result is available, and a zero timeout allows polling
        for a result.

        Args:
            timeout: The maximum time to block in seconds.

        Returns:
            Whether the solve call has finished or the next result is ready.
        """

class AsyncSolveHandle:
    """
    An object to await a search running in the background.

    Awaiting the handle returns the solve result. If the search has been
    started with `yield_=True`, the models can be iterated with `async for`
    before awaiting the result. A model is only valid until the next iteration
    step. Awaiting the result before all models have been iterated continues
    the search passing the remaining models to the callbacks only.

    The handle is an asynchronous context manager. Leaving the context
    cancels a search whose result has not been awaited and closes the handle.
    Afterward, the last model is no longer available.

    The callbacks passed to `clingo.control.Control.solve_async` are called on
    the solver thread. Their arguments are only valid during the call and they
    must not interact with the event loop.

    Cancelling an awaiting task cancels the search. The result can still be
    awaited afterward.

    Cancelling the search does not wait for it to stop. Awaiting the result
    afterward waits until it stopped. Until then, callbacks might still be
    called and the control object must not be used.

    See also: `clingo.control.Control.solve_async`
    """

    def __await__(self) -> Generator[Any, None, SolveResult]:
        """
        Wait for the search to finish and return the solve result.
        """

    def __aiter__(self) -> Self:
        """
        Return the handle to iterate over the models.

        This is only possible if the search has been started with
        `yield_=True`.
        """

    async def __anext__(self) -> Model:
        """
        Wait for the next model.
        """

    async def __aenter__(self) -> Self:
        """
        Return the handle.

        The search has already been started when the handle is created.
        """

    async def __aexit__(
        self,
        type: type[BaseException] | None,
        value: BaseException | None,
        traceback: TracebackType | None,
        /,
    ) -> None:
        """
        Stop the search and close the handle.

        Args:
            type: The exception type, if an exception occurred.
            value: The exception instance, if an exception occurred.
            traceback: The traceback object, if an exception occurred.
        """

    def cancel(self) -> None:
        """
        Cancel the running search.

        No further models are handed out and awaiting the handle afterward
        returns the result once the search stopped. The result is interrupted
        unless the search finished before. The function does not wait for the
        search to stop; the control object must not be used before it stopped.
        """

    def core(self) -> Sequence[int]:
        """
        Get the subset of assumptions that made the problem unsatisfiable.

        The core is only available after the result has been awaited.
        """

    def last(self) -> Model | None:
        """
        Get the last computed model, if any.

        If the result has not been awaited yet or the problem is unsatisfiable,
        the function returns `None`.
        """
