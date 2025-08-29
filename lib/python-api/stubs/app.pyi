"""
Module to create custom clingo-based applications.

This module provides application-level functionality for working with clingo,
including an application class, options definitions, and helper routines.

# Examples

The following example shows how to run clingo without customization:

```python
import sys
from clingo.app import clingo_main
from clingo.core import Library
from clingo.script import enable_python

with Library() as lib:
    enable_python(lib)
    clingo_main(lib, sys.argv[1:])
```

The next example shows how to write a simple clingo-based application that adds
an option to print atoms in models in order:

```python
from typing import Callable, Sequence
import sys
from clingo.app import App, AppOptions, Flag, clingo_main
from clingo.core import Library
from clingo.control import Control
from clingo.solve import Model
from clingo.symbol import Symbol

Parts = Sequence[Sequence[tuple[str, Sequence[Symbol]]]]

class MyApp(App):
    def __init__(self) -> None:
        super().__init__("my-app", "1.0.0")
        self._order = Flag()

    def print_model(self, model: Model, default_printer: Callable[[], None]) -> None:
        if self._order.value:
            print(" ".join(str(sym) for sym in sorted(model.symbols(shown=True))))
        else:
            default_printer()

    def register_options(self, options: AppOptions) -> None:
        options.add_flag(
            "MyApp", "order", "Print atoms in models in order.", self._order
        )

    def main(self, control: Control, files: Sequence[str], parts: Parts) -> None:
        control.parse_files(files)
        control.main(parts)

with Library() as lib:
    sys.exit(clingo_main(lib, sys.argv[1:], MyApp()))
```
"""

from __future__ import annotations

import typing

import clingo.control
import clingo.core
import clingo.solve

__all__ = ["App", "AppOptions", "Flag", "clingo_main"]

def _pyclingo() -> int: ...
def clingo_main(
    lib: clingo.core.Library,
    arguments: typing.Sequence[str],
    app: App | None = None,
    raise_errors: bool = False,
) -> int:
    """
    Entry point for running the Clingo application.

    This function initializes necessary components, processes input arguments, and
    then executes the main functionality of the Clingo application. It can
    optionally use a provided App instance to customize this behavior.

    The flag `raise_errors` might help for debugging purposes to obtain traces
    where errors originated from.

    Args:
        lib:
                The Clingo core library interface.
        arguments:
                A list of command-line arguments.
        app:
                An optional App instance containing application-specific logic.
        raise_errors:
            Whether to raise errors instead of just reporting them.

    Returns:
        An integer exit code.
    """

class App:
    """
    Interface to implement a custom Clingo-based application.

    This class encapsulates the main execution flow of a Clingo-based application.
    It provides methods for executing the program, printing models, registering
    application options, and validating the configuration.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __init__(
        self, program_name: str | None = None, version: str | None = None
    ) -> None:
        """
        Initializes the application with a program name and its version.

        If no name is given, `"clingo"` is used. If no version is given, the current
        clingo version is used.

        Args:
            program_name:
                The name of the application.
            version:
                The version string of the application.
        """

    def main(
        self, control: clingo.control.Control, files: typing.Sequence[str]
    ) -> None:
        """
        Run the main execution flow of the application.

        This method is invoked after Clingo's control object has been configured. It
        processes input files, executes the control flow, and handles output.

        Args:
            control:
                The Clingo control object for managing grounding and solving.
            files:
                A list of filenames representing the input logic programs.
        """

    def print_model(
        self, model: clingo.solve.Model, default_printer: typing.Callable[[], None]
    ) -> None:
        """
        Print the given model in a custom format.

        The default printer can be used to print the model as clingo would. A possible
        use case would be to use the printer and then print additional information
        below the model.

        Args:
                model:
                        The current model held by the solver.
                default_printer:
                        A callable that prints the model in default format.
        """

    def register_options(self, options: AppOptions) -> None:
        """
        Register command-line options for the application.

        Args:
                options:
                        An instance of AppOptions to add new options.
        """

    def validate_options(self) -> None:
        """
        Validate the options passed to the application.

        Once the application options have been set, this method confirms that they are
        valid. If an error is detected, a ValueError should be raised.
        """

class AppOptions:
    """
    Manager for application options and their definitions.

    Provides interface to add/configures various option types:
    - argument options,
    - flag options, and
    - multi-value options.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def add(
        self,
        group: str,
        option: str,
        description: str,
        parser: typing.Callable[[str], None],
        multi: bool = False,
        argument: str | None = None,
    ) -> None:
        """
        Adds an option with a custom parser.

        An option's group name acts like a section header; all options with the same
        group name are displayed under it in the help output. The option name is the
        identifier following the two dashes on the command line and it's value is
        parsed by the given parser.

        Args:
            group:
                The option group or category.
            option:
                The option name (after --).
            description:
                A brief description of the option.
            parser:
                A callable to process the string input for this option.
            multi:
                Whether the option can accept multiple values (default is False).
            argument:
                An optional string indicating the argument type or format.
        """

    def add_flag(self, group: str, option: str, description: str, flag: Flag) -> None:
        """
        Add a Boolean flag option.

        Similar to `add_option` but used for Boolean options that can be toggled on or
        off.

        Args:
            group:
                The option group or category.
            option:
                The option name or flag identifier.
            description:
                A brief description of the flag option.
            flag:
                A Flag object that holds the value of the flag.
        """

class Flag:
    """
    Boolean flag with value management.

    Represents command-line toggle options.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __init__(self, value: bool = False) -> None:
        """
        Initializes the flag with the provided value.

        Args:
            value:
                The initial boolean value of the flag (default is False).
        """

    @property
    def value(self) -> bool:
        """
        Get/set the value of the flag.
        """

    @value.setter
    def value(self, arg0: bool) -> None: ...
