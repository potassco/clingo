from typing import Any, Protocol, TypeVar
from abc import abstractmethod


C = TypeVar("C", bound="Comparable")


class Comparable(Protocol):
    @abstractmethod
    def __eq__(self, other: Any) -> bool: ...

    @abstractmethod
    def __lt__(self: C, other: C) -> bool: ...

    def __gt__(self: C, other: C) -> bool: ...

    def __le__(self: C, other: C) -> bool: ...

    def __ge__(self: C, other: C) -> bool: ...
