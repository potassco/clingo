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
>>> print(ctl.solve(on_model=print))
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

from enum import IntEnum
from types import TracebackType
from typing import Self

__all__: list[str] = ["GroundHandle", "GroundResult"]

class GroundResult(IntEnum):
    """
    Enumeration of ground result types.
    """

    Interrupted = 2
    """
    Indicates that the grounding was interrupted.
    """
    Ok = 0
    """
    Indicates that the grounding finished successfully.
    """
    Unsatisfiable = 1
    """
    Indicates that grounding finished and the program is unsatisfiable.
    """

class GroundHandle:
    """
    An object to interact with a running grounding.

    A GroundHandle is a context manager and must be used with Python's with
    statement.

    Blocking functions in this object release the GIL. They are not thread-safe
    though.

    See also: `clingo.control.Control.ground`, `clingo.control.Control.start_ground`
    """

    def __enter__(self) -> Self:
        """
        Return the handle.

        Grounding has already been started when the handle is created.

        Returns:
            The handle itself.
        """

    def __exit__(
        self,
        type: type[BaseException] | None,
        value: BaseException | None,
        traceback: TracebackType | None,
        /,
    ) -> None:
        """
        Stop grounding and close the handle.

        Args:
            type: The exception type, if an exception occurred.
            value: The exception instance, if an exception occurred.
            traceback: The traceback object, if an exception occurred.
        """

    def cancel(self) -> None:
        """
        Cancel the running grounding.

        See also: `clingo.control.Control.interrupt`
        """

    def close(self) -> None:
        """
        Stop grounding and close the handle.
        """

    def get(self) -> GroundResult:
        """
        Get the ground result.

        This is always the last function to be called on a handle to ensure
        that grounding is properly terminated. It might be preceded by a call
        to cancel to stop grounding.

        Returns:
            The ground result.
        """

    def wait(self, timeout: float | None = None) -> bool:
        """
        Wait for the ground call to finish or for the given timeout to expire.

        If a timeout is provided, the function blocks for the given duration or
        until grounding is finished. A positive timeout blocks for that amount
        of time. A negative timeout blocks until grounding is finished. And a
        zero timeout allows polling for a result.

        Args:
            timeout: The maximum time to block in seconds.

        Returns:
            Whether the ground call has finished.
        """
