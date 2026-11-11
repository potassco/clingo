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

import enum
import typing
from collections.abc import Iterator

__all__: list[str] = [
    "Stats",
    "StatsArray",
    "StatsArrayView",
    "StatsMap",
    "StatsMapView",
    "StatsType",
    "StatsView",
]

_T = typing.TypeVar("_T")

class StatsType(enum.IntEnum):
    """
    The type of a stats object.
    """

    Array = 1
    Map = 2
    Value = 0

class StatsView:
    """
    Class representing read-only solver stats.
    """

    def __contains__(self, key: str) -> bool:
        """
        Checks whether the given key is in the element, which must be a map.
        """

    def __getitem__(self, key: int | str, /) -> StatsView:
        """
        Look up the value with the given key.

        Values can be accessed by index if the element is an array or by name
        if the element is a map.
        """

    def __iter__(self) -> Iterator[str | StatsView]:
        """
        Return an iterator over this statistics object, which must be a map or an array.
        """

    def __len__(self) -> int:
        """
        Return the length of this element.
        """

    def nestify(self) -> object:
        """
        Convert the statistics object into a nested structure consisting of sequences,
        mappings with string keys, and floats.
        """

    @property
    def array(self) -> StatsArrayView:
        """
        Return an array of stats objects.
        """

    @property
    def map(self) -> StatsMapView:
        """
        Get a map of stats objects.
        """

    @property
    def type(self) -> StatsType:
        """
        Return the type of the stats object.
        """

    @property
    def value(self) -> float:
        """
        Return the value of the stats object.
        """

class Stats(StatsView):
    """
    Class representing solver stats.
    """

    def __getitem__(self, key: int | str, /) -> Stats:
        """
        Look up the value with the given key.

        Values can be accessed by index if the element is an array or by name
        if the element is a map.
        """

    def __iter__(self) -> Iterator[str | Stats]:
        """
        Return an iterator over this statistics object, which must be a map or an array.
        """

    def update(self, values: object, /) -> None:
        """
        Update the statistics with the given values.

        Note that values can be inserted and changed but they cannot be deleted nor can
        their type be changed.

        Args:
            values: A nested structure consisting of sequences, mappings with
                string keys, floats, and functions. The latter can be used to
                update existing values. They receive the previous values as
                argument and must return an updated value. If there is no
                previous value, `None` is passed as argument.
        """

    @property
    def array(self) -> StatsArray:
        """
        Return an array of stats objects.
        """

    @property
    def map(self) -> StatsMap:
        """
        Get a map of stats objects.
        """

    @property
    def value(self) -> float:
        """
        Return the value of the stats object.
        """

    @value.setter
    def value(self, value: float) -> None:
        """
        Set the value of the stats object.
        """

class StatsArrayView:
    """
    Class representing a read-only array of stats.

    This provides a read-only sequence interface.
    """

    def __getitem__(self, index: int) -> StatsView:
        """
        Return the element at the given index.
        """

    def __iter__(self) -> Iterator[StatsView]:
        """
        Return an iterator over the elements of the array.
        """

    def __len__(self) -> int:
        """
        Return the length of the array.
        """

class StatsArray(StatsArrayView):
    """
    Class representing an array of stats.

    This class provides a mutable sequence interface - array elements can be
    modified but they cannot be deleted. Modifications are implemented via
    `clingo.stats.Stats.update`.

    Most use cases should be implementable just using the update function of the
    top-level statistics object.
    """

    def __getitem__(self, index: int) -> Stats:
        """
        Return the element at the given index.
        """

    def __iter__(self) -> Iterator[Stats]:
        """
        Return an iterator over the elements of the array.
        """

    def __setitem__(self, index: int, value: object) -> None:
        """
        Set the element at the given index to the given value.
        """

    def append(self, value: object) -> None:
        """
        Append the given value to the array.

        Args:
            value: The value to append.
        """

    def extend(self, values: object, /) -> None:
        """
        Append all values from the given iterable to the array.
        """

class StatsMapView:
    """
    Class representing a read-only map of stats.

    This class provides a read-only mapping interface.
    """

    def __getitem__(self, key: str) -> StatsView:
        """
        Look up the value with the given key.
        """

    def __iter__(self) -> Iterator[str]:
        """
        Return an iterator over the keys of the map.
        """

    def __len__(self) -> int:
        """
        Return the length of the map.
        """

    def __contains__(self, key: str) -> bool:
        """
        Return whether the map contains the given key.
        """

    @typing.overload
    def get(self, key: str) -> StatsView | None:
        """
        Return the value for `key` if present, otherwise return `None`.

        Args:
            key: The key to look up.

        Returns:
            The value associated with the key or `None` if the key is not
                found.
        """

    @typing.overload
    def get(self, key: str, default: _T) -> StatsView | _T:
        """
        Return the value for `key` if present, otherwise return `default`.

        Args:
            key: The key to look up.
            default: The default value to return if the key is not found.

        Returns:
            The value associated with the key or the default value if the key
                is not found.
        """

    def get(self, key: str, default: _T | None = None) -> StatsView | _T | None:
        """
        Return the value for `key` if present, otherwise return `default`.

        Args:
            key: The key to look up.
            default: The default value to return if the key is not found.

        Returns:
            The value associated with the key or the default value if the key
                is not found.
        """

    def items(self) -> Iterator[tuple[str, StatsView]]:
        """
        Return an iterator over the items of the map.
        """

    def keys(self) -> Iterator[str]:
        """
        Return an iterator over the keys of the map.
        """

    def values(self) -> Iterator[StatsView]:
        """
        Return an iterator over the values of the map.
        """

class StatsMap(StatsMapView):
    """
    Class representing a map of stats.

    This class provides a mutable mapping interface - values for keys can be
    modified but they cannot be deleted. Modifications are implemented via
    `clingo.stats.Stats.update`.

    Most use cases should be implementable just using the update function of
    the top-level statistics object.
    """

    def __getitem__(self, key: str) -> Stats:
        """
        Look up the value with the given key.
        """

    def __setitem__(self, key: str, value: object) -> None:
        """
        Set the value at the given key.
        """

    @typing.overload
    def get(self, key: str, default: None = None) -> Stats | None:
        """
        Return the value for `key` if present, otherwise return `None`.

        Args:
            key: The key to look up.

        Returns:
            The value associated with the key or `None` if the key is not
                found.
        """

    @typing.overload
    def get(self, key: str, default: _T) -> Stats | _T:
        """
        Return the value for `key` if present, otherwise return `default`.

        Args:
            key: The key to look up.
            default: The default value to return if the key is not found.

        Returns:
            The value associated with the key or the default value if the key
                is not found.
        """

    def get(self, key: str, default: _T | None = None) -> StatsView | _T | None:
        """
        Return the value for `key` if present, otherwise return `default`.

        Args:
            key: The key to look up.
            default: The default value to return if the key is not found.

        Returns:
            The value associated with the key or the default value if the key
                is not found
        """

    def setdefault(self, key: str, default: object) -> Stats:
        """
        Insert key with default if not already in the map.

        Args:
            key: The key to look up.
            default: The default value to insert.

        Returns:
            The newly inserted value or the existing value.
        """

    def items(self) -> Iterator[tuple[str, Stats]]:
        """
        Return an iterator over the items of the map.
        """

    def values(self) -> Iterator[Stats]:
        """
        Return an iterator over the values of the map.
        """
