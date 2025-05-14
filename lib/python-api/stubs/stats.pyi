"""
Functions and classes related to solver stats.

Examples
--------
The following example shows how to add custom stats and access the stats:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> def on_stats(step, accu):
...     accu.update({"example": [21]})
...     accu.update({"example": [lambda x: (x or 0) + 21]})
...
>>> lib = Library()
>>> ctl = Control(lib, ['--stats'])
>>> ctl.parse_string("{a}.")
>>> ctl.ground()
>>> with ctl.solve(on_stats=on_stats) as hnd:
...     print(hnd.get())
SAT
>>> with ctl.solve(on_stats=on_stats) as hnd:
...     print(hnd.get())
SAT
>>> ctl.stats['user_step']
{ "example": [21.0] }
>>> ctl.stats['user_accu']
{ "example": [42.0] }
>>> ctl.stats['summary']['times']
{ "cpu": 0.000785999999999995,
  "sat": 7.867813110351562e-06,
  "solve": 2.288818359375e-05,
  "total": 0.0007848739624023438,
  "unsat": 0.0 }
```

Note that the control object is created passing options `--stats`; without this
option only basic stats are reported.
"""

from __future__ import annotations

import typing

__all__ = ["Stats", "StatsArray", "StatsMap", "StatsType"]

class StatsType:
    """
    The type of a stats object.

    Members:

      Map : Indicate a map of stats.

      Array : Indicate an array of stats.

      Value : Indicate a value of stats.
    """

    Array: typing.ClassVar[StatsType]  # value = <StatsType.Array: 1>
    Map: typing.ClassVar[StatsType]  # value = <StatsType.Map: 2>
    Value: typing.ClassVar[StatsType]  # value = <StatsType.Value: 0>
    __members__: typing.ClassVar[
        dict[str, StatsType]
    ]  # value = {'Map': <StatsType.Map: 2>, 'Array': <StatsType.Array: 1>, 'Value': <StatsType.Value: 0>}
    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __getstate__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __init__(self, value: typing.SupportsInt) -> None: ...
    def __int__(self) -> int: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __setstate__(self, state: typing.SupportsInt) -> None: ...
    def __str__(self) -> str: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class Stats:
    """
    Class representing solver stats.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def nestify(self) -> typing.Any:
        """
        Convert the statistics object into a nested structure consisting of sequencens,
        mappings with string keys, and floats.
        """

    def update(self, values: typing.Any) -> None:
        """
        Update the statistics with the given values.

        Note that values can be inserted and changed but they cannot be deleted nor can
        their type be changed.

        Args:
            values: A nested structure consisting of sequencens, mappings with string
                keys, floats, and functions. The latter can be used to update
                existing values. They receive the previous values as argument and must
                return an updated value. If there is no previous value, `None` is
                passed as argument.
        """

    @property
    def array(self) -> StatsArray:
        """
        Get an array of stats objects.
        """

    @property
    def map(self) -> StatsMap:
        """
        Get a map of stats objects.
        """

    @property
    def type(self) -> StatsType:
        """
        Get the type of the stats object.
        """

    @property
    def value(self) -> float:
        """
        Get/set the value of the stats object.
        """

    @value.setter
    def value(self, arg1: typing.SupportsFloat) -> None: ...

class StatsArray:
    """
    Class representing an array of stats.

    This class partially implements the mutable sequence protocol - elements of
    arrays can be modified but they cannot be deleted. Modifications are
    implemented via `Stats.update`.

    Most use cases should be implementable just using the update function of the
    top-level statistics object.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __getitem__(self, arg0: typing.SupportsInt) -> Stats:
        """
        Get the element at the given index.
        """

    def __len__(self) -> int:
        """
        Get the length of the array.
        """

    def __setitem__(self, arg0: typing.SupportsInt, arg1: typing.Any) -> None:
        """
        Set the element at the given index to the given value.
        """

    def append(self, value: typing.Any) -> None:
        """
        Append the given value to the array.

        Args:
                value: The value to append.
        """

class StatsMap:
    """
    Class representing a map of stats.

    This class partially implements the mutable mapping protocol - value of keys
    can be modified but they cannot be deleted. Modifications are implemented via
    `Stats.update`.

    Most use cases should be implementable just using the update function of the
    top-level statistics object.
    """

    @staticmethod
    def _pybind11_conduit_v1_(*args, **kwargs): ...
    def __getitem__(self, key: str) -> Stats:
        """
        Lookup the value with the given key.
        """

    def __len__(self) -> int:
        """
        Get the length of the map.
        """

    def __setitem__(self, key: str, value: typing.Any) -> None:
        """
        Set the value at the given key.
        """
