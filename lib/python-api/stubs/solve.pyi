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
    ...         print(mdl)
    ...         hnd.resume()
    ...     print(hnd.get())
    ...
    b
    a
    SAT
"""

from __future__ import annotations

import collections.abc
import enum
import typing

import clingo.base
import clingo.symbol

__all__ = ["Model", "ModelType", "SolveControl", "SolveHandle", "SolveResult"]

class ModelType(enum.IntEnum):
    """
    Enumeration of model types.
    """

    BraveConsequences: typing.ClassVar[
        ModelType
    ]  # value = <ModelType.BraveConsequences: 1>
    CautiousConsequences: typing.ClassVar[
        ModelType
    ]  # value = <ModelType.CautiousConsequences: 2>
    StableModel: typing.ClassVar[ModelType]  # value = <ModelType.StableModel: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Model:
    """
    A view on the solver's current solution.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __str__(self) -> str:
        """
        Get a string representation of the model.
        """

    def contains(self, atom: clingo.symbol.Symbol) -> bool:
        """
        Check if the model contains the given atom.

        Args:
            atom: The atom to look up.
        Returns:
            Whether the atom is contained.
        """

    def extend(self, symbols: typing.Sequence[clingo.symbol.Symbol]) -> None:
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
    ) -> typing.Sequence[clingo.symbol.Symbol]:
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
    def cost(self) -> typing.Sequence[int]:
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
    def priorities(self) -> typing.Sequence[int]:
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

class SolveControl:
    """
    A control object to add clauses while solving.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def add_clause(
        self, clause: typing.Sequence[tuple[clingo.symbol.Symbol, bool] | int]
    ) -> None:
        """
        Add a clause that applies to the current solving step during the search.

        Args:
          clause: The literals of the clause.
        """

    def add_nogood(
        self, nogood: typing.Sequence[tuple[clingo.symbol.Symbol, bool] | int]
    ) -> None:
        """
        Add a nogood that applies to the current solving step during the search.

        Args:
          nogood: The literals of the nogood.
        """

    @property
    def base(self) -> clingo.base.Base:
        """
        Get the atom/term bases of the program.
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

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
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

    def __iter__(self) -> collections.abc.Iterator[Model]:
        """
        Get an iterator over the models.
        """

    def cancel(self) -> None:
        """
        Cancel the running search.

        See also: `clingo.control.Control.interrupt`
        """

    def core(self) -> typing.Sequence[int]:
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

    def last(self) -> clingo.solve.Model | None:
        """
        Get the last computed model, if any.

        If the search is not completed yet or the problem is unsatisfiable, the
        function returns `None`.
        """

    def model(self) -> clingo.solve.Model | None:
        """
        Get the current model if there is any.
        """

    def resume(self) -> None:
        """
        Discards the last model and starts searching for the next one.

        If the search has been started asynchronously, this function starts the search
        in the background.
        """

    def wait(self, timeout: typing.SupportsFloat | None = None) -> bool:
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

class SolveResult:
    """
    A solve result captures information about a solve call.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
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
        Whether at least one model was found.
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
