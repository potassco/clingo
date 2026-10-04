"""
Core functionality used throughout the clingo package.

Examples
--------

```python
>>> from clingo.core import version
>>> version()
(6, 0, 0)
```
"""

from __future__ import annotations

import collections.abc
import enum
import types
import typing

__all__: list[str] = [
    "Library",
    "Location",
    "LogLevel",
    "MessageType",
    "Position",
    "version",
]

def version() -> tuple[int, int, int]:
    """
    Get Clingo's version.

    Returns:
        A tuple (major, minor, revision) representing the Clingo version.
    """

class LogLevel(enum.IntEnum):
    """
    The available log levels.
    """

    Debug = 1
    Error = 8
    Info = 2
    Trace = 0
    Warn = 7

class MessageType(enum.IntEnum):
    """
    Message categories emitted by the logger.
    """

    AtomUndefined = 4
    Debug = 1
    Error = 8
    FileIncluded = 5
    GlobalVariable = 6
    Info = 2
    OperationUndefined = 3
    Trace = 0
    Warn = 7

class Library:
    """
    A library object that manages Clingo's core resources.

    This object is responsible for storing the logger, symbols, strings, and scripts.
    Functions and classes that need to create symbols require an instance of this class.

    This class implements the `ContextManager` interface.
    """

    def __enter__(self) -> Library:
        """
        Return self.
        """

    def __exit__(self, arg0: typing.Any, arg1: typing.Any, arg2: typing.Any) -> bool:
        """
        Close the library object.
        """

    def __init__(
        self,
        shared: bool = True,
        slotted: bool = True,
        log_level: LogLevel = LogLevel.Info,
        logger: collections.abc.Callable[[MessageType, str], None] | None = None,
        message_limit: int = 25,
    ) -> None:
        """
        Create a library object.

        Args:
            slotted: Use a slotted allocator to store symbols. Setting this to true
                might improve performance.
            shared: Indicates whether symbols should be created in a thread-safe
                manner. Setting this to false might improve performance in
                single-threaded applications.
            log_level: The log level.
            logger: A logger to emit/intercept messages.
            message_limit: The maximum number of messages to emit.
        """

    def _capsule(self) -> types.CapsuleType:
        """
        Get a capsule holding the underlying C library object.
        """

class Position:
    """
    Represents a position in a source file.

    A `Position` object tracks the location of a symbol or construct
    within a source file, including its file name, line number, and column.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(self, lib: Library, file: str, line: int, column: int) -> None:
        """
        Create a position object.

        Args:
            lib: The library object managing symbols.
            file: The file name where the position is located.
            line: The line number in the file.
            column: The column number in the line.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...
    @property
    def column(self) -> int:
        """
        The column number.
        """

    @property
    def file(self) -> str:
        """
        The file name.
        """

    @property
    def line(self) -> int:
        """
        The line number.
        """

class Location:
    """
    Represents a range of positions in a source file.

    The `Location` object tracks the start and end positions of a region in the
    file. It is used for error reporting and debugging, providing information about
    the source of the program elements.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __init__(self, begin: Position, end: Position) -> None:
        """
        Create a location object.

        Args:
            begin: The beginning of the location.
            end: The end of the location.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...
    @property
    def begin(self) -> Position:
        """
        The beginning of the location.
        """

    @property
    def end(self) -> Position:
        """
        The end of the location.
        """
