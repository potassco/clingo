"""
AST nodes for body literals.
"""

from collections.abc import Sequence
from typing import TypeAlias, final

from clingo.ast import AST
from clingo.ast.aggregates import AggregateFunction, SetAggregateElement
from clingo.ast.literals import LeftGuard, Literal, RightGuard, Sign
from clingo.ast.terms import Term
from clingo.ast.theory import TheoryAtomElement, TheoryRightGuard
from clingo.core import Library, Location

@final
class BodySimpleLiteral(AST):
    """
    A literal in a rule body.

    For example, `not p(X)`.
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

    def update(self, lib: Library, *, literal: Literal = ...) -> BodySimpleLiteral:
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
class BodyAggregateElement(AST):
    """
    An element of a body aggregate.

    For example, `X: p(X)` in `#sum { X: p(X) } > 2`.
    """

    __match_args__ = ("location", "tuple", "condition")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        tuple: Sequence[Term],
        condition: Sequence[Literal],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The terms of the element.
            condition: The condition of the element.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        tuple: Sequence[Term] = ...,
        condition: Sequence[Literal] = ...,
    ) -> BodyAggregateElement:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The terms of the element.
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
    def condition(self) -> Sequence[Literal]:
        """
        The condition of the element.
        """

@final
class BodyAggregate(AST):
    """
    An aggregate in a rule body.

    For example, `not #sum { X: p(X) } > 2`.
    """

    __match_args__ = ("location", "sign", "left", "function", "elements", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        sign: Sign,
        left: LeftGuard | None,
        function: AggregateFunction,
        elements: Sequence[BodyAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            sign: The sign of the aggregate.
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
        sign: Sign = ...,
        left: LeftGuard | None = ...,
        function: AggregateFunction = ...,
        elements: Sequence[BodyAggregateElement] = ...,
        right: RightGuard | None = ...,
    ) -> BodyAggregate:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            sign: The sign of the aggregate.
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
    def sign(self) -> Sign:
        """
        The sign of the aggregate.
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
    def elements(self) -> Sequence[BodyAggregateElement]:
        """
        The elements of the aggregate.
        """

    @property
    def right(self) -> RightGuard | None:
        """
        The optional right guard of the aggregate.
        """

@final
class BodySetAggregate(AST):
    """
    A set aggregate in a rule body.

    For example, `1 { p(X): q(X) } 2`.
    """

    __match_args__ = ("location", "sign", "left", "elements", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        sign: Sign,
        left: LeftGuard | None,
        elements: Sequence[SetAggregateElement],
        right: RightGuard | None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            sign: The sign of the aggregate.
            left: The optional left guard of the aggregate.
            elements: The elements of the aggregate.
            right: The optional right guard of the aggregate.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        sign: Sign = ...,
        left: LeftGuard | None = ...,
        elements: Sequence[SetAggregateElement] = ...,
        right: RightGuard | None = ...,
    ) -> BodySetAggregate:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the aggregate.
            sign: The sign of the aggregate.
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
    def sign(self) -> Sign:
        """
        The sign of the aggregate.
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
class BodyTheoryAtom(AST):
    """
    A theory atom in a rule body.

    For example, `&sum { X: p(X) } >= 0`.
    """

    __match_args__ = ("location", "sign", "name", "elements", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        sign: Sign,
        name: Term,
        elements: Sequence[TheoryAtomElement],
        right: TheoryRightGuard | None,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the theory atom.
            sign: The sign of the theory atom.
            name: The name of the theory atom.
            elements: The elements of the theory atom.
            right: The optional right guard of the theory atom.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        sign: Sign = ...,
        name: Term = ...,
        elements: Sequence[TheoryAtomElement] = ...,
        right: TheoryRightGuard | None = ...,
    ) -> BodyTheoryAtom:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the theory atom.
            sign: The sign of the theory atom.
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
    def sign(self) -> Sign:
        """
        The sign of the theory atom.
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
class BodyConditionalLiteral(AST):
    """
    A conditional literal in a rule body.

    For example, `p(X): q(X)`.
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
    ) -> BodyConditionalLiteral:
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
class BodySort(AST):
    """
    A sort literal in a rule body.

    For example, `(X,Y) = #sort { Z: q(Z) }`.
    """

    __match_args__ = ("location", "left", "elements")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        left: Term,
        elements: Sequence[BodyAggregateElement],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            left: The pair of output terms.
            elements: The elements of the sort literal.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        left: Term = ...,
        elements: Sequence[BodyAggregateElement] = ...,
    ) -> BodySort:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the literal.
            left: The pair of output terms.
            elements: The elements of the sort literal.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the literal.
        """

    @property
    def left(self) -> Term:
        """
        The pair of output terms.
        """

    @property
    def elements(self) -> Sequence[BodyAggregateElement]:
        """
        The elements of the sort literal.
        """

BodyLiteral: TypeAlias = (
    BodySimpleLiteral
    | BodyAggregate
    | BodySort
    | BodySetAggregate
    | BodyTheoryAtom
    | BodyConditionalLiteral
)
"""
The available body literals.
"""
