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

import typing

__all__ = ["Library", "Location", "LogLevel", "MessageType", "Position", "version"]

def version() -> tuple[int, int, int]:
    """
    Get Clingo's version.

    Returns:
        A tuple (major, minor, revision) representing the Clingo version.
    """

class LogLevel:
    """
    The available log levels.

    Members:

      Trace : Report trace messages (includes debug level).

      Debug : Report debug messages (includes info level).

      Info : Report info messages (includes warning level).

      Warn : Report warning messages (includes error level).

      Error : Report error messages.
    """

    Debug: typing.ClassVar[LogLevel]  # value = <LogLevel.Debug: 1>
    Error: typing.ClassVar[LogLevel]  # value = <LogLevel.Error: 8>
    Info: typing.ClassVar[LogLevel]  # value = <LogLevel.Info: 2>
    Trace: typing.ClassVar[LogLevel]  # value = <LogLevel.Trace: 0>
    Warn: typing.ClassVar[LogLevel]  # value = <LogLevel.Warn: 7>
    __members__: typing.ClassVar[
        dict[str, LogLevel]
    ]  # value = {'Trace': <LogLevel.Trace: 0>, 'Debug': <LogLevel.Debug: 1>, 'Info': <LogLevel.Info: 2>, 'Warn': <LogLevel.Warn: 7>, 'Error': <LogLevel.Error: 8>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
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

class MessageType:
    """
    Message categories emitted by the logger.

    Members:

      Trace : A trace message.

      Debug : A debug message.

      Info : A generic info message.

      OperationUndefined : An info message about an undefined operation.

      AtomUndefined : An info message about an undefined atom.

      FileIncluded : An info message about an already included file.

      GlobalVariable : An info message about a global variable in the tuple of an aggregate.

      Warn : A warning message.

      Error : An error message.
    """

    AtomUndefined: typing.ClassVar[
        MessageType
    ]  # value = <MessageType.AtomUndefined: 4>
    Debug: typing.ClassVar[MessageType]  # value = <MessageType.Debug: 1>
    Error: typing.ClassVar[MessageType]  # value = <MessageType.Error: 8>
    FileIncluded: typing.ClassVar[MessageType]  # value = <MessageType.FileIncluded: 5>
    GlobalVariable: typing.ClassVar[
        MessageType
    ]  # value = <MessageType.GlobalVariable: 6>
    Info: typing.ClassVar[MessageType]  # value = <MessageType.Info: 2>
    OperationUndefined: typing.ClassVar[
        MessageType
    ]  # value = <MessageType.OperationUndefined: 3>
    Trace: typing.ClassVar[MessageType]  # value = <MessageType.Trace: 0>
    Warn: typing.ClassVar[MessageType]  # value = <MessageType.Warn: 7>
    __members__: typing.ClassVar[
        dict[str, MessageType]
    ]  # value = {'Trace': <MessageType.Trace: 0>, 'Debug': <MessageType.Debug: 1>, 'Info': <MessageType.Info: 2>, 'OperationUndefined': <MessageType.OperationUndefined: 3>, 'AtomUndefined': <MessageType.AtomUndefined: 4>, 'FileIncluded': <MessageType.FileIncluded: 5>, 'GlobalVariable': <MessageType.GlobalVariable: 6>, 'Warn': <MessageType.Warn: 7>, 'Error': <MessageType.Error: 8>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
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

class Library:
    """
    A library object that manages Clingo's core resources.

    This object is responsible for storing the logger, symbols, strings, and scripts.
    Functions and classes that need to create symbols require an instance of this class.

    This class implements the `ContextManager` interface.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
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
        logger: typing.Callable[[MessageType, str], None] | None = None,
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

class Location:
    """
    Represents a range of positions in a source file.

    The `Location` object tracks the start and end positions of a region in the
    file. It is used for error reporting and debugging, providing information about
    the source of the program elements.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
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

class Position:
    """
    Represents a position in a source file.

    A `Position` object tracks the location of a symbol or construct
    within a source file, including its file name, line number, and column.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
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
