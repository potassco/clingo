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

from collections.abc import Callable, Generator
from enum import IntEnum
from types import TracebackType
from typing import Any, Protocol, Self, TypeVar, type_check_only

from typing_extensions import CapsuleType

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

class LogLevel(IntEnum):
    """
    The available log levels.
    """

    Debug = 1
    """
    The debug level.
    """
    Error = 8
    """
    The error level (least verbose).
    """
    Info = 2
    """
    The info level.
    """
    Trace = 0
    """
    The trace level (most verbose).
    """
    Warn = 7
    """
    The warning level.
    """

class MessageType(IntEnum):
    """
    Message categories emitted by the logger.
    """

    AtomUndefined = 4
    """
    An undefined atom in the program.
    """
    Debug = 1
    """
    A debug message.
    """
    Error = 8
    """
    An error message; a corresponding exception is raised later.
    """
    FileIncluded = 5
    """
    The same file included multiple times.
    """
    GlobalVariable = 6
    """
    A global variable in the tuple of an aggregate element.
    """
    Info = 2
    """
    An info message.
    """
    OperationUndefined = 3
    """
    An undefined operation in the program.
    """
    Trace = 0
    """
    A trace message.
    """
    Warn = 7
    """
    A warning message.
    """

class Library:
    """
    A library object that manages Clingo's core resources.

    This object is responsible for storing the logger, symbols, strings, and scripts.
    Functions and classes that need to create symbols require an instance of this class.

    This class implements the `ContextManager` interface.
    """

    def __init__(
        self,
        shared: bool = True,
        slotted: bool = True,
        fast_release: bool = False,
        log_level: LogLevel = LogLevel.Info,
        logger: Callable[[MessageType, str], None] | None = None,
        message_limit: int = 25,
    ) -> None:
        """
        Create a library object.

        Args:
            shared: Indicates whether symbols should be created in a thread-safe
                manner. Setting this to false might improve performance in
                single-threaded applications.
            slotted: Use a slotted allocator to store symbols. Setting this to true
                might improve performance.
            fast_release: Release stored symbols without checking for references. Leads
                to crashes if symbols are still referenced. Use with care.
            log_level: The log level.
            logger: A logger to emit/intercept messages.
            message_limit: The maximum number of messages to emit.
        """

    def __enter__(self) -> Self:
        """
        Return the library object.

        Returns:
            The library object.
        """

    def __exit__(
        self,
        type: type[BaseException] | None,
        value: BaseException | None,
        traceback: TracebackType | None,
        /,
    ) -> None:
        """
        Close the library object.

        Args:
            type: The exception type, if an exception occurred.
            value: The exception instance, if an exception occurred.
            traceback: The traceback, if an exception occurred.
        """

    def _capsule(self) -> CapsuleType:
        """
        Get a capsule holding the underlying C library object.
        """

class Position:
    """
    Represents a position in a source file.

    A `Position` object tracks the location of a symbol or construct
    within a source file, including its file name, line number, and column.
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

    def __eq__(self, value: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            value: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, value: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            value: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __hash__(self) -> int:
        """
        Compute a hash value for this position.

        Returns:
            The hash value.
        """

    def __ge__(self, value: Position, /) -> bool:
        """
        Compare with another position for greater than or equal.

        Args:
            value: The position to compare with.

        Returns:
            Whether this position is greater than or equal to the other one.
        """

    def __gt__(self, value: Position, /) -> bool:
        """
        Compare with another position for greater than.

        Args:
            value: The position to compare with.

        Returns:
            Whether this position is greater than the other one.
        """

    def __le__(self, value: Position, /) -> bool:
        """
        Compare with another position for less than or equal.

        Args:
            value: The position to compare with.

        Returns:
            Whether this position is less than or equal to the other one.
        """

    def __lt__(self, value: Position, /) -> bool:
        """
        Compare with another position for less than.

        Args:
            value: The position to compare with.

        Returns:
            Whether this position is less than the other one.
        """

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

    def __init__(self, begin: Position, end: Position) -> None:
        """
        Create a location object.

        Args:
            begin: The beginning of the location.
            end: The end of the location.
        """

    def __eq__(self, value: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            value: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, value: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            value: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __hash__(self) -> int:
        """
        Compute a hash value for this location.

        Returns:
            The hash value.
        """

    def __ge__(self, value: Location, /) -> bool:
        """
        Compare with another location for greater than or equal.

        Args:
            value: The location to compare with.

        Returns:
            Whether this location is greater than or equal to the other one.
        """

    def __gt__(self, value: Location, /) -> bool:
        """
        Compare with another location for greater than.

        Args:
            value: The location to compare with.

        Returns:
            Whether this location is greater than the other one.
        """

    def __le__(self, value: Location, /) -> bool:
        """
        Compare with another location for less than or equal.

        Args:
            value: The location to compare with.

        Returns:
            Whether this location is less than or equal to the other one.
        """

    def __lt__(self, value: Location, /) -> bool:
        """
        Compare with another location for less than.

        Args:
            value: The location to compare with.

        Returns:
            Whether this location is less than the other one.
        """

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

_T = TypeVar("_T")

@type_check_only
class Future(Protocol[_T]):
    """
    The interface of futures created by `EventLoop.create_future`.

    The interface is a subset of `asyncio.Future`.
    """

    def __await__(self) -> Generator[Any, None, _T]: ...
    def add_done_callback(self, fn: Callable[[Any], object], /) -> object: ...
    def cancelled(self) -> bool: ...
    def done(self) -> bool: ...
    def set_exception(self, exception: BaseException, /) -> None: ...
    def set_result(self, result: _T, /) -> None: ...

@type_check_only
class EventLoop(Protocol):
    """
    The interface of event loops passed to the asynchronous functions.

    The interface is a subset of `asyncio.AbstractEventLoop`. It is only used to
    create futures and to notify the loop from background threads, so that
    event loops of other frameworks can be adapted.

    See also: `clingo.control.Control.solve_async`,
    `clingo.control.Control.ground_async`
    """

    def create_future(self) -> Future[Any]: ...
    def call_soon_threadsafe(
        self, callback: Callable[..., object], *args: Any
    ) -> object: ...
