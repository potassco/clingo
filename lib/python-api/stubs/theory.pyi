"""
This module allows for using theories implemented in C from Python.
"""

from collections.abc import Callable, Iterator, Sequence
from typing import TypeAlias

from clingo.app import AppOptions
from clingo.ast import Statement
from clingo.control import Control
from clingo.core import Library
from clingo.solve import Model
from clingo.stats import Stats
from clingo.symbol import Symbol

__all__: list[str] = ["Theory", "TheoryAssignment"]

ValueType: TypeAlias = Symbol | int | float
"""
The value type of a theory value.

It can be a symbol, an integer or a float.
"""

class TheoryAssignment:
    """
    Assignment of theory values.
    """

    def __iter__(
        self,
    ) -> Iterator[tuple[Symbol, ValueType]]:
        """
        Get an iterator over the symbol value pairs in the assignment.

        Returns:
            An iterator over the symbol value pairs.
        """

    def __getitem__(self, index: int | Symbol, /) -> ValueType:
        """
        Get the value at the given index in the assignment.

        Args:
            index: The index of the value

        Returns:
            The value at the given index.
        """

    def get(self, symbol: Symbol, /) -> int | None:
        """
        Get the value index of the symbol in the assignment.

        The resulting index can be used to retrieve the value using
        `__getitem__`.

        Args:
            symbol: The symbol to lookup.

        Returns:
            The index or None if unnassigned.
        """

class Theory:
    """
    Object to call functions from a C-library implementing a custom theory.
    """

    def __init__(self, library: Library, create: object, /) -> None:
        """
        Construct a theory object.

        Args:
            library: Library object to store symbols in.
            create:
                A capsule object holding a function pointer to initialize the theory.
        """

    def assignment(self, thread_id: int, /) -> TheoryAssignment:
        """
        Get the symbols and values currently assigned by the theory

        It depends on the theory when this function can be called. Generally, it can be
        called after `clingo.theory.Theory.on_model` while the solver is still holding
        its current model.

        Args:
            thread_id: The id of the thread to query.

        Returns:
            An iterable over symbol value pairs.
        """

    def on_model(self, model: Model, /) -> None:
        """
        Notify the theory about the given model.

        Some theories extend the model here and set their internal assignments.
        This function should be called in the on_model callback of a control's
        solve function.

        Args:
            model: The current model.
        """

    def on_stats(self, step: Stats, accu: Stats, /) -> None:
        """
        Let the theory update statistics.

        Some theories extend the statistics here.

        Args:
            step: The per step statistics.
            accu: The accumulated statistics.
        """

    def prepare(self, control: Control, /) -> None:
        """
        Prepare the theory for solving.

        Args:
            control: The control object used for solving.
        """

    def register(self, control: Control, /) -> None:
        """
        Register the theory with the given control object.

        This function should be called once on the control object before grounding and
        solving starts.

        Args:
            control: The control object to register the theory with.
        """

    def register_options(self, options: AppOptions, /) -> None:
        """
        Register theory related options.

        Args:
            options: The application options.

        See also: `clingo.app.App.register_options`
        """

    def rewrite(
        self, statement: Statement, callback: Callable[[Statement], None], /
    ) -> None:
        """
        Rewrite the given statement and pass the result to the callback.

        Some theories require rewriting prior to adding a non-ground program to a
        control object.

        Args:
            statement: The statement to rewrite.
            callback: The callback receiving rewritten statements.
        """

    def rewrite_files(
        self, lib: Library, control: Control, files: Sequence[str], /
    ) -> None:
        """
        Rewrite the program in the given files and add it to the control.

        Args:
            lib: The library to store symbols.
            control: The target control.
            files: The files to parse.
        """

    def rewrite_string(self, lib: Library, control: Control, program: str, /) -> None:
        """
        Rewrite the given program adding it to the control object.

        Args:
            lib: The library to store symbols.
            control: The target control.
            program: The program to parse.
        """

    def validate_options(self) -> None:
        """
        Validate the registered options.

        See also: `clingo.app.App.validate_options`
        """

    def value(self, thread_id: int, symbol: Symbol, /) -> ValueType | None:
        """
        Get the value of the symbol in the assignment of the given thread.

        It depends on the theory when this function can be called. Generally, it can be
        called after `clingo.theory.Theory.on_model` while the solver is still holding
        its current model.

        Args:
            thread_id: The id of the thread to query.
            symbol: The symbol to lookup.

        Returns:
            The value or None if unassigned.
        """

    @property
    def commit_hash(self) -> str:
        """
        The commit hash of the theory.
        """

    @property
    def has_assignment(self) -> bool:
        """
        Check whether the theory supports symbol value assignments.
        """

    @property
    def name(self) -> str:
        """
        The name of the theory.
        """

    @property
    def version(self) -> tuple[int, int, int]:
        """
        The version of the theory (major, minor, revision).
        """
