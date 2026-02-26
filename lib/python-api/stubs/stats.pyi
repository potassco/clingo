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
...     step.update({"example": [21]})
...     accu.update({"example": [lambda x: (x or 0) + 21]})
...
>>> lib = Library()
>>> ctl = Control(lib, ['--stats'])
>>> ctl.parse_string("{a}.")
>>> ctl.ground()
>>> print(ctl.solve(on_stats=on_stats))
SAT
>>> print(ctl.solve(on_stats=on_stats))
SAT
>>> ctl.stats['user_step'].nestify()
{ "example": [21.0] }
>>> ctl.stats['user_accu'].nestify()
{ "example": [42.0] }
>>> ctl.stats['summary']['times'].nestify()
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

import enum
import typing

__all__: list[str] = [
    "Stats",
    "StatsArray",
    "StatsArrayView",
    "StatsMap",
    "StatsMapView",
    "StatsType",
    "StatsView",
]

class StatsType(enum.IntEnum):
    """
    The type of a stats object.
    """

    Array: typing.ClassVar[StatsType]  # value = <StatsType.Array: 1>
    Map: typing.ClassVar[StatsType]  # value = <StatsType.Map: 2>
    Value: typing.ClassVar[StatsType]  # value = <StatsType.Value: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Stats(StatsView):
    """
    Class representing solver stats.
    """

    @typing.overload
    def __getitem__(self, arg0: int) -> Stats:
        """
        Get the element at the given index.
        """

    @typing.overload
    def __getitem__(self, key: str) -> Stats:
        """
        Lookup the value with the given key.
        """

    def __iter__(self) -> typing.Iterator[str | Stats]:
        """
        Get an iterator over this statistics object, which must be a map or an array.
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
    def value(self) -> float:
        """
        Get/set the value of the stats object.
        """

    @value.setter
    def value(self, arg1: typing.SupportsFloat) -> None: ...

class StatsArray(StatsArrayView):
    """
    Class representing an array of stats.

    This class partially implements the mutable sequence protocol - elements of
    arrays can be modified but they cannot be deleted. Modifications are
    implemented via `Stats.update`.

    Most use cases should be implementable just using the update function of the
    top-level statistics object.
    """

    def __getitem__(self, arg0: int) -> Stats:
        """
        Get the element at the given index.
        """

    def __iter__(self) -> typing.Iterator[Stats]:
        """
        Get an iterator over the elements of the array.
        """

    def __setitem__(self, arg0: int, arg1: typing.Any) -> None:
        """
        Set the element at the given index to the given value.
        """

    def append(self, value: typing.Any) -> None:
        """
        Append the given value to the array.

        Args:
                value: The value to append.
        """

class StatsArrayView:
    """
    Class representing a read-only array of stats.

    This class partially implements the mutable sequence protocol.
    """

    def __getitem__(self, arg0: int) -> StatsView:
        """
        Get the element at the given index.
        """

    def __iter__(self) -> typing.Iterator[StatsView]:
        """
        Get an iterator over the elements of the array.
        """

    def __len__(self) -> int:
        """
        Get the length of the array.
        """

    def __str__(self) -> str:
        """
        A readable representation to inspect the array.
        """

class StatsMap(StatsMapView):
    """
    Class representing a map of stats.

    This class partially implements the mutable mapping protocol - value of keys
    can be modified but they cannot be deleted. Modifications are implemented via
    `Stats.update`.

    Most use cases should be implementable just using the update function of the
    top-level statistics object.
    """

    def __getitem__(self, key: str) -> Stats:
        """
        Lookup the value with the given key.
        """

    def __setitem__(self, key: str, value: typing.Any) -> None:
        """
        Set the value at the given key.
        """

    def items(self) -> typing.Iterator[tuple[str, Stats]]:
        """
        Get an iterator over the items of the map.
        """

    def values(self) -> typing.Iterator[Stats]:
        """
        Get an iterator over the values of the map.
        """

class StatsMapView:
    """
    Class representing a read-only map of stats.

    This class partially implements the mutable mapping protocol.
    """

    def __getitem__(self, key: str) -> StatsView:
        """
        Lookup the value with the given key.
        """

    def __iter__(self) -> typing.Iterator[str]:
        """
        Get an iterator over the keys of the map.
        """

    def __len__(self) -> int:
        """
        Get the length of the map.
        """

    def __str__(self) -> str:
        """
        A readable representation to inspect the map.
        """

    def items(self) -> typing.Iterator[tuple[str, StatsView]]:
        """
        Get an iterator over the items of the map.
        """

    def keys(self) -> typing.Iterator[str]:
        """
        Get an iterator over the keys of the map.
        """

    def values(self) -> typing.Iterator[StatsView]:
        """
        Get an iterator over the values of the map.
        """

class StatsView:
    """
    Class representing read-only solver stats.
    """

    def __contains__(self, arg0: str) -> bool:
        """
        Checks whether the given key is in the element, which must be a map.
        """

    @typing.overload
    def __getitem__(self, arg0: int) -> StatsView:
        """
        Get the element at the given index.
        """

    @typing.overload
    def __getitem__(self, key: str) -> StatsView:
        """
        Lookup the value with the given key.
        """

    def __iter__(self) -> typing.Iterator[str | StatsView]:
        """
        Get an iterator over this statistics object, which must be a map or an array.
        """

    def __len__(self) -> int:
        """
        Get the length of this element.
        """

    def __str__(self) -> str:
        """
        A readable representation to inspect the statistics.
        """

    def nestify(self) -> typing.Any:
        """
        Convert the statistics object into a nested structure consisting of sequencens,
        mappings with string keys, and floats.
        """

    @property
    def array(self) -> StatsArrayView:
        """
        Get an array of stats objects.
        """

    @property
    def map(self) -> StatsMapView:
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
        Get the value of the stats object.
        """
