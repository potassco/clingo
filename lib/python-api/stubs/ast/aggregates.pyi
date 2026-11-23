"""
AST nodes for aggregates.
"""

from collections.abc import Sequence
from enum import IntEnum
from typing import final

from clingo.ast import AST
from clingo.ast.literals import Literal
from clingo.core import Library, Location

__all__: list[str] = [
    "AggregateFunction",
    "SetAggregateElement",
]

class AggregateFunction(IntEnum):
    """
    Enumeration of aggregate functions.
    """

    Count = 0
    """
    The `#count` function.
    """
    Sum = 1
    """
    The `#sum` function.
    """
    Sump = 2
    """
    The `#sum+` function.
    """
    Min = 3
    """
    The `#min` function.
    """
    Max = 4
    """
    The `#max` function.
    """

@final
class SetAggregateElement(AST):
    """
    An element of a set aggregate.

    For example, `p(X): q(X)` in `{ p(X): q(X) }`.
    """

    __match_args__ = ("location", "literal", "condition")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        literal: Literal,
        condition: Sequence[Literal],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            literal: The literal of the element.
            condition: The condition of the element.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        literal: Literal = ...,
        condition: Sequence[Literal] = ...,
    ) -> SetAggregateElement:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            literal: The literal of the element.
            condition: The condition of the element.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the element.
        """

    @property
    def literal(self) -> Literal:
        """
        The literal of the element.
        """

    @property
    def condition(self) -> Sequence[Literal]:
        """
        The condition of the element.
        """
