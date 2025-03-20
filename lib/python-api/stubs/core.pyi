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

__all__ = ["Library", "Location", "MessageType", "Position", "version"]

def version() -> tuple[int, int, int]:
    """
    Clingo's version as a tuple (major, minor, revision).
    """

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
    Library objects are used to store the logger, symbols, strings, and scripts.

    Any function/or class that needs to create symbols takes this object as a
    parameter.

    Destroying the library object frees the logger, the symbols, and the scripts.

    This class implements the ContextManager interface.
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
            logger: A logger to emit/intercept messages.
            message_limit: The maximum number of messages to emit.
        """

class Location:
    """
    Location tracking object.
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

class Position:
    """
    Position object tracking locations in files.
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
            lib: The library to object storing symbols.
            file: The file name of the position.
            line: The line number of the postion.
            column: The column number of the postion.
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
