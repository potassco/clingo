"""
AST nodes for literals.
"""

from collections.abc import Sequence
from enum import IntEnum
from typing import TypeAlias, final

from clingo.ast import AST
from clingo.ast.terms import Term
from clingo.core import Library, Location

__all__: list[str] = [
    "LeftGuard",
    "Literal",
    "LiteralBoolean",
    "LiteralComparison",
    "LiteralSymbolic",
    "Relation",
    "RightGuard",
    "Sign",
]

class Sign(IntEnum):
    """
    Enumeration of signs of literals.
    """

    NoSign = 0
    """
    No sign.
    """
    Single = 1
    """
    Default negation `not`.
    """
    Double = 2
    """
    Double default negation `not not`.
    """

class Relation(IntEnum):
    """
    Enumeration of comparison relations.
    """

    Equal = 0
    """
    The relation `=`.
    """
    NotEqual = 1
    """
    The relation `!=`.
    """
    Less = 2
    """
    The relation `<`.
    """
    LessEqual = 3
    """
    The relation `<=`.
    """
    Greater = 4
    """
    The relation `>`.
    """
    GreaterEqual = 5
    """
    The relation `>=`.
    """

@final
class LeftGuard(AST):
    """
    A guard on the left-hand side of an aggregate.

    For example, `1 <` in `1 < #count { X: p(X) }`.
    """

    __match_args__ = ("term", "relation")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, term: Term, relation: Relation) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            term: The term of the guard.
            relation: The relation of the guard.
        """

    def update(
        self, lib: Library, *, term: Term = ..., relation: Relation = ...
    ) -> LeftGuard:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            term: The term of the guard.
            relation: The relation of the guard.

        Returns:
            The updated node.
        """

    @property
    def term(self) -> Term:
        """
        The term of the guard.
        """

    @property
    def relation(self) -> Relation:
        """
        The relation of the guard.
        """

@final
class RightGuard(AST):
    """
    A guard on the right-hand side of a comparison or aggregate.

    For example, `< 3` in `X < 3`.
    """

    __match_args__ = ("relation", "term")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, relation: Relation, term: Term) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            relation: The relation of the guard.
            term: The term of the guard.
        """

    def update(
        self, lib: Library, *, relation: Relation = ..., term: Term = ...
    ) -> RightGuard:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            relation: The relation of the guard.
            term: The term of the guard.

        Returns:
            The updated node.
        """

    @property
    def relation(self) -> Relation:
        """
        The relation of the guard.
        """

    @property
    def term(self) -> Term:
        """
        The term of the guard.
        """

@final
class LiteralBoolean(AST):
    """
    A literal representing a Boolean constant.

    For example, `#true` or `not #false`.
    """

    __match_args__ = ("location", "sign", "value")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, sign: Sign, value: bool
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            sign: The sign of the literal.
            value: The value of the constant.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        sign: Sign = ...,
        value: bool = ...,
    ) -> LiteralBoolean:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            sign: The sign of the literal.
            value: The value of the constant.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the literal.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

    @property
    def value(self) -> bool:
        """
        The value of the constant.
        """

@final
class LiteralComparison(AST):
    """
    A literal representing a (chain of) comparison(s).

    For example, `X < Y` or `1 <= X < 10`.
    """

    __match_args__ = ("location", "sign", "left", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        sign: Sign,
        left: Term,
        right: Sequence[RightGuard],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            sign: The sign of the literal.
            left: The leftmost term of the comparison.
            right: The guards following the leftmost term.

                The sequence contains at least one guard.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        sign: Sign = ...,
        left: Term = ...,
        right: Sequence[RightGuard] = ...,
    ) -> LiteralComparison:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            sign: The sign of the literal.
            left: The leftmost term of the comparison.
            right: The guards following the leftmost term.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the literal.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

    @property
    def left(self) -> Term:
        """
        The leftmost term of the comparison.
        """

    @property
    def right(self) -> Sequence[RightGuard]:
        """
        The guards following the leftmost term.

        The sequence contains at least one guard.
        """

@final
class LiteralSymbolic(AST):
    """
    A literal representing an atom.

    For example, `p(X)` or `not -q`.
    """

    __match_args__ = ("location", "sign", "atom")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, sign: Sign, atom: Term
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            sign: The sign of the literal.
            atom: The term representing the atom.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        sign: Sign = ...,
        atom: Term = ...,
    ) -> LiteralSymbolic:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            sign: The sign of the literal.
            atom: The term representing the atom.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the literal.
        """

    @property
    def sign(self) -> Sign:
        """
        The sign of the literal.
        """

    @property
    def atom(self) -> Term:
        """
        The term representing the atom.
        """

Literal: TypeAlias = LiteralBoolean | LiteralComparison | LiteralSymbolic
"""
The available literals.
"""
