"""
AST nodes for head literals.
"""

from collections.abc import Sequence
from typing import TypeAlias, final

from clingo.ast import AST
from clingo.ast.aggregates import AggregateFunction, SetAggregateElement
from clingo.ast.literals import LeftGuard, Literal, RightGuard
from clingo.ast.terms import Term
from clingo.ast.theory import TheoryAtomElement, TheoryRightGuard
from clingo.core import Library, Location

@final
class HeadSimpleLiteral(AST):
    """
    A literal in a rule head.

    For example, `p(X)`.
    """

    __match_args__ = ("literal",)
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, literal: Literal) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            literal: The literal.
        """

    def update(self, lib: Library, *, literal: Literal = ...) -> HeadSimpleLiteral:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            literal: The literal.

        Returns:
            The updated node.
        """

    @property
    def literal(self) -> Literal:
        """
        The literal.
        """

@final
class HeadAggregateElement(AST):
    """
    An element of a head aggregate.

    For example, `X: p(X): q(X)` in `#count { X: p(X): q(X) } = 1`.
    """

    __match_args__ = ("location", "tuple", "literal", "condition")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        tuple: Sequence[Term],
        literal: Literal,
        condition: Sequence[Literal],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The terms of the element.
            literal: The literal of the element.
            condition: The condition of the element.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        tuple: Sequence[Term] = ...,
        literal: Literal = ...,
        condition: Sequence[Literal] = ...,
    ) -> HeadAggregateElement:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The terms of the element.
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
    def tuple(self) -> Sequence[Term]:
        """
        The terms of the element.
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

@final
class HeadAggregate(AST):
    """
    An aggregate in a rule head.

    For example, `#count { X: p(X): q(X) } = 1`.
    """

    __match_args__ = ("location", "left", "function", "elements", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        left: LeftGuard | None,
        function: AggregateFunction,
        elements: Sequence[HeadAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            left: The optional left guard of the aggregate.
            function: The aggregate function.
            elements: The elements of the aggregate.
            right: The optional right guard of the aggregate.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        left: LeftGuard | None = ...,
        function: AggregateFunction = ...,
        elements: Sequence[HeadAggregateElement] = ...,
        right: RightGuard | None = ...,
    ) -> HeadAggregate:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            left: The optional left guard of the aggregate.
            function: The aggregate function.
            elements: The elements of the aggregate.
            right: The optional right guard of the aggregate.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the aggregate.
        """

    @property
    def left(self) -> LeftGuard | None:
        """
        The optional left guard of the aggregate.
        """

    @property
    def function(self) -> AggregateFunction:
        """
        The aggregate function.
        """

    @property
    def elements(self) -> Sequence[HeadAggregateElement]:
        """
        The elements of the aggregate.
        """

    @property
    def right(self) -> RightGuard | None:
        """
        The optional right guard of the aggregate.
        """

@final
class HeadSetAggregate(AST):
    """
    A set aggregate in a rule head.

    Set aggregates in heads are also known as choice rules. For example, `1 { p(X): q(X) } 2`.
    """

    __match_args__ = ("location", "left", "elements", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        left: LeftGuard | None,
        elements: Sequence[SetAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            left: The optional left guard of the aggregate.
            elements: The elements of the aggregate.
            right: The optional right guard of the aggregate.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        left: LeftGuard | None = ...,
        elements: Sequence[SetAggregateElement] = ...,
        right: RightGuard | None = ...,
    ) -> HeadSetAggregate:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            left: The optional left guard of the aggregate.
            elements: The elements of the aggregate.
            right: The optional right guard of the aggregate.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the aggregate.
        """

    @property
    def left(self) -> LeftGuard | None:
        """
        The optional left guard of the aggregate.
        """

    @property
    def elements(self) -> Sequence[SetAggregateElement]:
        """
        The elements of the aggregate.
        """

    @property
    def right(self) -> RightGuard | None:
        """
        The optional right guard of the aggregate.
        """

@final
class HeadTheoryAtom(AST):
    """
    A theory atom in a rule head.

    For example, `&diff { X-Y } <= 3`.
    """

    __match_args__ = ("location", "name", "elements", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: Term,
        elements: Sequence[TheoryAtomElement],
        right: TheoryRightGuard | None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the theory atom.
            name: The name of the theory atom.
            elements: The elements of the theory atom.
            right: The optional right guard of the theory atom.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: Term = ...,
        elements: Sequence[TheoryAtomElement] = ...,
        right: TheoryRightGuard | None = ...,
    ) -> HeadTheoryAtom:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the theory atom.
            name: The name of the theory atom.
            elements: The elements of the theory atom.
            right: The optional right guard of the theory atom.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the theory atom.
        """

    @property
    def name(self) -> Term:
        """
        The name of the theory atom.
        """

    @property
    def elements(self) -> Sequence[TheoryAtomElement]:
        """
        The elements of the theory atom.
        """

    @property
    def right(self) -> TheoryRightGuard | None:
        """
        The optional right guard of the theory atom.
        """

@final
class HeadConditionalLiteral(AST):
    """
    A conditional literal in a disjunction.

    For example, `p(X): q(X)` in `p(X): q(X); r`.
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
            location: The location of the conditional literal.
            literal: The literal in front of the colon.
            condition: The condition following the colon.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        literal: Literal = ...,
        condition: Sequence[Literal] = ...,
    ) -> HeadConditionalLiteral:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the conditional literal.
            literal: The literal in front of the colon.
            condition: The condition following the colon.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the conditional literal.
        """

    @property
    def literal(self) -> Literal:
        """
        The literal in front of the colon.
        """

    @property
    def condition(self) -> Sequence[Literal]:
        """
        The condition following the colon.
        """

@final
class HeadDisjunction(AST):
    """
    A disjunction in a rule head.

    For example, `p; q(X): r(X)`. Rules with an empty disjunction as head are integrity constraints.
    """

    __match_args__ = ("location", "elements")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, elements: Sequence[DisjunctionElement]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the disjunction.
            elements: The elements of the disjunction.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        elements: Sequence[DisjunctionElement] = ...,
    ) -> HeadDisjunction:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the disjunction.
            elements: The elements of the disjunction.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the disjunction.
        """

    @property
    def elements(self) -> Sequence[DisjunctionElement]:
        """
        The elements of the disjunction.
        """

DisjunctionElement: TypeAlias = Literal | HeadConditionalLiteral
"""
An element of a disjunction.
"""
HeadLiteral: TypeAlias = (
    HeadSimpleLiteral
    | HeadAggregate
    | HeadSetAggregate
    | HeadTheoryAtom
    | HeadDisjunction
)
"""
The available head literals.
"""
