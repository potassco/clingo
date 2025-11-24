"""
Functions and classes related to grounding.

# Examples

The example shows how call external functions during grounding:

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Number
>>> from clingo.control import Control
>>>
>>> class Context:
...     def __init__(self, lib):
...       self.lib = lib
...     def inc(self, x):
...         return Number(self.lib, x.number + 1)
...     def seq(self, x, y):
...         return [x, y]
...
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string(\"\"\"
... p(@inc(10)).
... q(@seq(1,2)).
... \"\"\")
>>> ctl.ground(context=Context(lib))
>>> with ctl.solve(on_model=print) as hnd:
...     print(hnd.get())
p(11) q(1) q(2)
SAT
```

The example below shows how to start grounding in the background and wait for
grounding to finish:

```python
>>> from clingo.control import Control
>>> from clingo.core import Library
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("{a}.")
>>> with ctl.start_ground() as hnd:
...     w = {True: "finished", False: "running"}
...     print(f"start_ground status: {w[hnd.wait(0)]}")
...     print(f"start_ground result: {hnd.get()!r}")
...     print(f"start_ground status: {w[hnd.wait(0)]}")
...
start_ground status: running
start_ground result: <GroundResult.Ok: 0>
start_ground status: finished
```
"""

from __future__ import annotations

import enum
import typing

__all__ = ["GroundHandle", "GroundResult"]

class GroundResult(enum.IntEnum):
    """
    Enumeration of ground result types.
    """

    Interrupted: typing.ClassVar[GroundResult]  # value = <GroundResult.Interrupted: 2>
    Ok: typing.ClassVar[GroundResult]  # value = <GroundResult.Ok: 0>
    Unsatisfiable: typing.ClassVar[
        GroundResult
    ]  # value = <GroundResult.Unsatisfiable: 1>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class GroundHandle:
    """
    An object to interact with a running search.

    It can be used to control solving, like, retrieving models or cancelling a
    search.

    A GroundHandle is a context manager and must be used with Python's with
    statement.

    Blocking functions in this object release the GIL. They are not thread-safe
    though.

    See also: `clingo.control.Control.ground`
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __enter__(self) -> GroundHandle:
        """
        Start the search.
        """

    def __exit__(
        self, arg0: type | None, arg1: typing.Any | None, arg2: typing.Any | None
    ) -> None:
        """
        Stop grounding and close the handle.
        """

    def cancel(self) -> None:
        """
        Cancel the running search.

        See also: `clingo.control.Control.interrupt`
        """

    def get(self) -> GroundResult:
        """
        Get the ground result.

        This is always the last function to be called on a handle to ensure that the
        search is properly terminated. It might be preceded by a call to cancel to stop
        the search.
        """

    def wait(
        self, timeout: typing.SupportsFloat | typing.SupportsIndex | None = None
    ) -> bool:
        """
        Wait for the ground call to finish or the next result with an optional timeout.

        If a timeout is provided, the function blocks for the given duration or until a
        result is ready. A positive timeout blocks for that amount of time. A negative
        timeout blocks until a result is available, and a zero timeout allows polling
        for a result.

        Args:
            timeout: The maximum time to block in seconds.

        Returns:
            Whether the ground call has finished or the next result is ready.
        """
