"""
This module allows for using theories implemented in C from Python.
"""

from __future__ import annotations

import typing

import clingo.app
import clingo.control
import clingo.solve
import clingo.symbol

__all__ = ["Theory"]

class Theory:
    """
    Object to call functions from a C-library implementing a custom theory.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __init__(self, theory_pointer: typing.Any) -> None:
        """
        Construct a theory object from the given pointer.

        Args:
            theory_pointer: A capsule object holding a clingo_theory_t pointer.
        """

    def assignment(
        self, thread_id: int
    ) -> typing.Iterator[
        tuple[clingo.symbol.Symbol, clingo.symbol.Symbol | int | float]
    ]:
        """
        Get the symbols and values currently assigned by the theory

        It depends on the theory when this function can be called. Generally, it can be
        called after `on_model` while the solver is still holding its current model.

        Args:
            thread_id: The id of the thread to query.

        Returns:
            An interable over symbol value pairs.
        """

    def configure(self, name: str, value: str) -> None:
        """
        Configure the theory using its name/value interface.

        It depends on the theory which keys are supported and when this function can be
        called.

        Args:
            key: The name of the option.
            value: The value of the option.
        """

    def on_model(self, model: clingo.solve.Model) -> None:
        """
        Notify the theory about the given model.

        Some theories extend the model here are set their internal assignments. This
        function should be called in the on_model callback of a control's solve
        function.

        Args:
            model: The current model.
        """

    def register(self, control: clingo.control.Control) -> None:
        """
        Register the theory with the given control object.

        This function should be called once on the control object before grounding and
        solving starts.

        Args:
            control: The control object to register the theory with.
        """

    def register_options(self, options: clingo.app.AppOptions) -> None:
        """
        Register theory related options.

        Args:
            options: The application options.

        See also: `clingo.app.App.register_options`
        """

    def rewrite(
        self,
        statement: (
            clingo.ast.StatementRule
            | clingo.ast.StatementTheory
            | clingo.ast.StatementOptimize
            | clingo.ast.StatementWeakConstraint
            | clingo.ast.StatementShow
            | clingo.ast.StatementShowNothing
            | clingo.ast.StatementShowSignature
            | clingo.ast.StatementProject
            | clingo.ast.StatementProjectSignature
            | clingo.ast.StatementDefined
            | clingo.ast.StatementExternal
            | clingo.ast.StatementEdge
            | clingo.ast.StatementHeuristic
            | clingo.ast.StatementScript
            | clingo.ast.StatementInclude
            | clingo.ast.StatementProgram
            | clingo.ast.StatementConst
            | clingo.ast.StatementComment
        ),
        callback: typing.Callable[
            [
                clingo.ast.StatementRule
                | clingo.ast.StatementTheory
                | clingo.ast.StatementOptimize
                | clingo.ast.StatementWeakConstraint
                | clingo.ast.StatementShow
                | clingo.ast.StatementShowNothing
                | clingo.ast.StatementShowSignature
                | clingo.ast.StatementProject
                | clingo.ast.StatementProjectSignature
                | clingo.ast.StatementDefined
                | clingo.ast.StatementExternal
                | clingo.ast.StatementEdge
                | clingo.ast.StatementHeuristic
                | clingo.ast.StatementScript
                | clingo.ast.StatementInclude
                | clingo.ast.StatementProgram
                | clingo.ast.StatementConst
                | clingo.ast.StatementComment
            ],
            None,
        ],
    ) -> None:
        """
        Rewrite the given statement and pass the result to the callback.

        Some theories require rewriting prior to adding a non-ground program to a
        control object.

        Args:
            statement: The statement to rewrite.
            callback: The callback receiving rewritten statements.
        """

    def validate_options(self) -> None:
        """
        Check the registered options.

        See also: `clingo.app.App.validate_options`
        """

    def value(
        self, thread_id: int, symbol: clingo.symbol.Symbol
    ) -> clingo.symbol.Symbol | int | float | None:
        """
        Get the value of the symbol in the assignment of the given thread.

        It depends on the theory when this function can be called. Generally, it can be
        called after `on_model` while the solver is still holding its current model.

        Args:
            thread_id: The id of the thread to query.
            symbol: The symbol to lookup.

        Returns:
            The value or None if unnassigned.
        """

    @property
    def has_assignment(self) -> bool:
        """
        Check whether the theory supports symbol value assigments.
        """

    @property
    def name(self) -> str:
        """
        Get the name of the theory.
        """

    @property
    def version(self) -> tuple[int, int, int]:
        """
        Get the version of the theory (major, minor, revision).
        """

class _TheoryAssignment:
    """
    Internal class.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __iter__(
        self,
    ) -> typing.Iterator[
        tuple[clingo.symbol.Symbol, clingo.symbol.Symbol | int | float]
    ]:
        """
        Return self.
        """

    def __next__(
        self,
    ) -> tuple[clingo.symbol.Symbol, clingo.symbol.Symbol | int | float]:
        """
        Get the next symbol value pair.
        """
