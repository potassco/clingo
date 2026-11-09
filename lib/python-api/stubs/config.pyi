"""
Functions and classes related to configuration.

Examples
--------
The following example shows how inspect the configuration and modify it to
enumerate all models:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.config.attributes
['tester', 'solve', 'asp', 'solver', 'configuration', 'share',
 'learn_explicit', 'sat_prepro', 'stats', 'parse_ext', 'parse_maxsat']
>>> ctl.config.solve.attributes
['solve_limit', 'parallel_mode', 'global_restarts', 'distribute',
 'integrate', 'enum_mode', 'project', 'models', 'opt_mode', 'opt_stop']
>>> str(ctl.config.solve)
\"\"\"\\
solve_limit: "umax,umax"
parallel_mode: "1,compete"
global_restarts: "no"
distribute: "no,conflict,global,4,4194303"
integrate: "gp,1024,all"
enum_mode: "auto"
project: "no"
models: "-1"
opt_mode: "-1,opt"
opt_stop: "-1,opt,no"\\
\"\"\"
>>> ctl.config.solve.models.description
"Compute at most <n> models (0 for all)"
>>> ctl.config.solve.models = 0
>>> ctl.parse_string("1 {a; b}.")
>>> ctl.ground()
>>> print(ctl.solve(on_model=print))
b
a
a b
SAT
```

The next example shows how to extend the configuration with a custom entry:

```python
from clingo.core import Library
from clingo.control import Control


class CustomConfig:
    value: str | None
    array: list[str | None]

    def __init__(self):
        self.value = None
        self.array = []

    def set_val(self, value: str):
        self.value = value

    def get_val(self):
        return self.value

    def get_arr_len(self):
        return len(self.array)

    def set_arr_val(self, value, index=None):
        if index is None:
            index = 0
        while len(self.array) <= index:
            self.array.append(None)
        self.array[index] = value

    def get_arr_val(self, index=None):
        if index is None:
            index = 0
        return self.array[index]


lib = Library()
ctl = Control(lib)
cfg = ctl.config
ctm = CustomConfig()

cfg.add_entry("custom", "simple example config")
cfg.add_entry("custom.val", "value", ctm.get_val, ctm.set_val)
cfg.add_entry("custom.arr[]", "array", size=ctm.get_arr_len)
cfg.add_entry(
    "custom.arr[].val", "value in array", get=ctm.get_arr_val, set=ctm.set_arr_val
)

cfg.custom.val = "a"
cfg.custom.arr[0].val = "b"
cfg.custom.arr[1].val = "c"

print(cfg.custom)
```

Running the above code produces the following output:
```
arr:
  - val: "b"
  - val: "c"
val: "a"
```
"""

import typing
from collections.abc import Callable, Sequence

__all__: list[str] = ["Config"]

from typing import Protocol

class ArrayGetter(Protocol):
    """
    Getter for array configuration entries.
    """
    def __call__(self, index: int | None = None, /) -> str | None:
        """
        Get the value at the given index.

        Handling the case that index is None is left to the user. Possibilities
        include returning the first element or a default one.
        """

class ArraySetter(Protocol):
    """
    Setter for array configuration entries.
    """
    def __call__(self, value: str, index: int | None = None, /) -> None:
        """
        Set the value at the given index.

        Handling the case that index is None is left to the user. Possibilities
        include setting the first element or a default one.
        """

class Config:
    """
    Allows for changing the configuration of the underlying solver.

    Options are organized hierarchically. To get or change the value of an option
    (identified by `is_value`) use:

    ```python
    config.group.subgroup.option = "value"        # variant 1 (short)
    config.group.subgroup.option.value = "value"  # variant 2
    value = config.group.subgroup.option.value
    ```

    There are also sequences of option groups (identified by `is_sequence`):

    ```python
    config.group.subgroup[0].option = "value1"
    config.group.subgroup[1].option = "value2"
    ```

    Use the `attributes` member to list subgroups of an option group. Meta options
    with key `configuration` set multiple related options when assigned. Use
    `description` for more information about an option or option group.

    Notes:

    - The first element of a sequence can be accessed directly without index 0.
    - Config objects have a YAML-like string representation for inspection.
    - In string representations of sequences, attributes (for index 0) are only
      added if the sequence is empty.
    - Option values are always strings; assigned values are automatically converted
      to strings.
    """

    def __getattr__(self, name: str) -> Config:
        """
        Get the configuration entry with the given name.
        """

    def __getitem__(self, index: int) -> Config:
        """
        Get the index-th element of a sequence.
        """

    def __len__(self) -> int:
        """
        Get the length of a sequence config.
        """

    def __setattr__(self, name: str, value: object) -> None:
        """
        Set the value with the given name.

        Values are automatically converted to strings.
        """

    @typing.overload
    def add_entry(
        self,
        name: str,
        description: str,
    ) -> None:
        """
        Add a custom configuration entry.

        This overload is used for entries that group other entries and do not
        have a value.

        Notes:
        - Entries can have at most one array parent entry.
        - Custom entries can be added under the root key. Existing solver
          configuration entries cannot be extened.

        Args:
            name: Name of the new entry.
            description: Description of the new entry.
        """

    @typing.overload
    def add_entry(
        self,
        name: str,
        description: str,
        get: Callable[[], str | None] | None = None,
        set: Callable[[str], None] | None = None,
    ) -> None:
        """
        Add a custom configuration entry.

        This overload is used for entries that have a value and are not part of
        a sequence. At least one get or set callback must be provided.

        Args:
            name: Name of the new entry.
            description: Description of the new entry.
            get: Callable to get the value.
            set: Callable to set the value.
        """

    @typing.overload
    def add_entry(
        self,
        name: str,
        description: str,
        get: ArrayGetter | None = None,
        set: ArraySetter | None = None,
        size: Callable[[], int] | None = None,
    ) -> None:
        """
        Add a custom configuration entry.

        This overload is used for entries that are part of a sequence. At least
        one get, set, or size callbacks must be provided.

        It is up to the user to handle array insertion. Possible options
        include increasing the size of an array upon assignment of values or by
        setting a special size field that controls the size of the array.

        Args:
            name: Name of the new entry.
            description: Description of the new entry.
            get: Callable to get the value.
            set: Callable to set the value.
            size: Callable to get the size.
        """

    @typing.overload
    def add_entry(
        self,
        name: str,
        description: str,
        get: Callable[[], str | None] | None = None,
        set: Callable[[str], None] | None = None,
        size: Callable[[], int] | None = None,
    ) -> None:
        """
        Add a custom configuration entry.

        This overload is used for entries that are part of a sequence but set
        meta values that apply to all array entries. At least one get, set, or
        size callbacks must be provided.

        This could for example be used to get/set the array size, specify
        default values, etc.

        Args:
            name: Name of the new entry.
            description: Description of the new entry.
            get: Callable to get the value.
            set: Callable to set the value.
            size: Callable to get the size.
        """

    @property
    def attributes(self) -> Sequence[str]:
        """
        Get the attribute names of nested configurations.
        """

    @property
    def description(self) -> str:
        """
        Get the description of a configuration entry.
        """

    @property
    def is_sequence(self) -> bool:
        """
        Whether the configuration is a sequence.
        """

    @property
    def is_value(self) -> bool:
        """
        Whether the configuration entry is a value.
        """

    @property
    def value(self) -> str | None:
        """
        Get the string value of the configuration entry.
        """

    @value.setter
    def value(self, value: object) -> None:
        """
        Set the value of the configuration entry.

        Values are automatically converted to strings.
        """
