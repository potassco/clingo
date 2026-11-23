"""
AST nodes for terms.
"""

from collections.abc import Sequence
from enum import IntEnum
from typing import TypeAlias, final

from clingo.ast import AST
from clingo.core import Library, Location
from clingo.symbol import Symbol

__all__: list[str] = [
    "ArgumentTuple",
    "BinaryOperator",
    "FormatField",
    "FormatFieldExpression",
    "FormatFieldLiteral",
    "Projection",
    "Term",
    "TermAbsolute",
    "TermBinaryOperation",
    "TermFormatString",
    "TermFunction",
    "TermOrArgumentTuple",
    "TermOrProjection",
    "TermSymbolic",
    "TermTuple",
    "TermUnaryOperation",
    "TermVariable",
    "UnaryOperator",
]

class UnaryOperator(IntEnum):
    """
    Enumeration of unary operators.
    """

    Minus = 0
    """
    The unary minus `-`.
    """
    Negation = 1
    """
    The bitwise negation `~`.
    """

class BinaryOperator(IntEnum):
    """
    Enumeration of binary operators.
    """

    And = 0
    """
    The bitwise and `&`.
    """
    Division = 1
    """
    The integer division `/`.
    """
    Minus = 2
    """
    The subtraction `-`.
    """
    Modulo = 3
    """
    The modulo operation `\\`.
    """
    Multiplication = 4
    """
    The multiplication `*`.
    """
    Or = 5
    """
    The bitwise or `?`.
    """
    Plus = 6
    """
    The addition `+`.
    """
    Power = 7
    """
    The exponentiation `**`.
    """
    Xor = 8
    """
    The bitwise exclusive or `^`.
    """
    Interval = 9
    """
    The interval `..`.
    """

@final
class FormatFieldLiteral(AST):
    """
    A literal part of a format string.

    For example, `a` in `f"a{X}"`.
    """

    __match_args__ = ("location", "value")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, location: Location, value: str) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the part.
            value: The text of the part.
        """

    def update(
        self, lib: Library, *, location: Location = ..., value: str = ...
    ) -> FormatFieldLiteral:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the part.
            value: The text of the part.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the part.
        """

    @property
    def value(self) -> str:
        """
        The text of the part.
        """

@final
class FormatFieldExpression(AST):
    """
    An expression part of a format string.

    For example, `{X:x}` in `f"a{X:x}"`.
    """

    __match_args__ = ("location", "left", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, left: Term, right: str
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the part.
            left: The term to format.
            right: The format specification, for example `:x`, or the empty string.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        left: Term = ...,
        right: str = ...,
    ) -> FormatFieldExpression:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the part.
            left: The term to format.
            right: The format specification, for example `:x`, or the empty string.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the part.
        """

    @property
    def left(self) -> Term:
        """
        The term to format.
        """

    @property
    def right(self) -> str:
        """
        The format specification, for example `:x`, or the empty string.
        """

@final
class Projection(AST):
    """
    A placeholder for a projected argument.

    Projected arguments are written as `*`, for example, in `p(X,*)`.
    """

    __match_args__ = ("location",)
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, location: Location) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the placeholder.
        """

    def update(self, lib: Library, *, location: Location = ...) -> Projection:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the placeholder.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the placeholder.
        """

@final
class TermVariable(AST):
    """
    A variable term.

    For example, `X` or the anonymous variable `_`.
    """

    __match_args__ = ("location", "name", "anonymous")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, name: str, anonymous: bool = False
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the variable.
            name: The name of the variable.
            anonymous: Whether the variable is anonymous.

                Anonymous variables receive a unique name during preprocessing.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        anonymous: bool = ...,
    ) -> TermVariable:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the variable.
            name: The name of the variable.
            anonymous: Whether the variable is anonymous.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the variable.
        """

    @property
    def name(self) -> str:
        """
        The name of the variable.
        """

    @property
    def anonymous(self) -> bool:
        """
        Whether the variable is anonymous.

        Anonymous variables receive a unique name during preprocessing.
        """

@final
class TermSymbolic(AST):
    """
    A symbolic term.

    For example, `1`, `a`, or `"s"`.
    """

    __match_args__ = ("location", "symbol")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, location: Location, symbol: Symbol) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            symbol: The symbol of the term.
        """

    def update(
        self, lib: Library, *, location: Location = ..., symbol: Symbol = ...
    ) -> TermSymbolic:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            symbol: The symbol of the term.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the symbol.
        """

    @property
    def symbol(self) -> Symbol:
        """
        The symbol of the term.
        """

@final
class TermFormatString(AST):
    """
    A format string term.

    For example, `f"a{X:x}b"`.
    """

    __match_args__ = ("location", "elements")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, elements: Sequence[FormatField]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the format string.
            elements: The parts of the format string.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        elements: Sequence[FormatField] = ...,
    ) -> TermFormatString:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the format string.
            elements: The parts of the format string.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the format string.
        """

    @property
    def elements(self) -> Sequence[FormatField]:
        """
        The parts of the format string.
        """

@final
class TermAbsolute(AST):
    """
    An absolute value term.

    For example, `|X-Y|`.
    """

    __match_args__ = ("location", "pool")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, location: Location, pool: Sequence[Term]) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the term.
            pool: The argument pool of the term.

                If there is more than one element in the pool, the term is unpooled during preprocessing.
        """

    def update(
        self, lib: Library, *, location: Location = ..., pool: Sequence[Term] = ...
    ) -> TermAbsolute:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the term.
            pool: The argument pool of the term.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the term.
        """

    @property
    def pool(self) -> Sequence[Term]:
        """
        The argument pool of the term.

        If there is more than one element in the pool, the term is unpooled during preprocessing.
        """

@final
class TermUnaryOperation(AST):
    """
    A unary operation.

    For example, `-X`.
    """

    __match_args__ = ("location", "operator_type", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        operator_type: UnaryOperator,
        right: Term,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the operation.
            operator_type: The operator of the operation.
            right: The argument of the operation.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        operator_type: UnaryOperator = ...,
        right: Term = ...,
    ) -> TermUnaryOperation:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the operation.
            operator_type: The operator of the operation.
            right: The argument of the operation.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the operation.
        """

    @property
    def operator_type(self) -> UnaryOperator:
        """
        The operator of the operation.
        """

    @property
    def right(self) -> Term:
        """
        The argument of the operation.
        """

@final
class TermBinaryOperation(AST):
    """
    A binary operation.

    For example, `X+1` or the interval `1..3`.
    """

    __match_args__ = ("location", "left", "operator_type", "right")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        left: Term,
        operator_type: BinaryOperator,
        right: Term,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the operation.
            left: The left argument of the operation.
            operator_type: The operator of the operation.
            right: The right argument of the operation.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        left: Term = ...,
        operator_type: BinaryOperator = ...,
        right: Term = ...,
    ) -> TermBinaryOperation:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the operation.
            left: The left argument of the operation.
            operator_type: The operator of the operation.
            right: The right argument of the operation.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the operation.
        """

    @property
    def left(self) -> Term:
        """
        The left argument of the operation.
        """

    @property
    def operator_type(self) -> BinaryOperator:
        """
        The operator of the operation.
        """

    @property
    def right(self) -> Term:
        """
        The right argument of the operation.
        """

@final
class TermTuple(AST):
    """
    A tuple term.

    For example, `(a,b)` or the pool `(a;b,c)`.
    """

    __match_args__ = ("location", "pool")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, pool: Sequence[TermOrArgumentTuple]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the tuple.
            pool: The argument pool of the tuple.

                Elements are terms or argument tuples. If there is more than one element in the pool, the term is unpooled during preprocessing.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        pool: Sequence[TermOrArgumentTuple] = ...,
    ) -> TermTuple:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the tuple.
            pool: The argument pool of the tuple.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the tuple.
        """

    @property
    def pool(self) -> Sequence[TermOrArgumentTuple]:
        """
        The argument pool of the tuple.

        Elements are terms or argument tuples. If there is more than one element in the pool, the term is unpooled during preprocessing.
        """

@final
class TermFunction(AST):
    """
    A function term.

    For example, `f(X,a)`, the constant `a`, or the external function `@f(X)`.
    """

    __match_args__ = ("location", "name", "pool", "external")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        pool: Sequence[ArgumentTuple],
        external: bool = False,
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the function.
            name: The name of the function.
            pool: The argument pool of the function.

                If there is more than one element in the pool, the term is unpooled during preprocessing.
            external: Whether the function is an external function written with a leading `@`.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        pool: Sequence[ArgumentTuple] = ...,
        external: bool = ...,
    ) -> TermFunction:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the function.
            name: The name of the function.
            pool: The argument pool of the function.
            external: Whether the function is an external function written with a leading `@`.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the function.
        """

    @property
    def name(self) -> str:
        """
        The name of the function.
        """

    @property
    def pool(self) -> Sequence[ArgumentTuple]:
        """
        The argument pool of the function.

        If there is more than one element in the pool, the term is unpooled during preprocessing.
        """

    @property
    def external(self) -> bool:
        """
        Whether the function is an external function written with a leading `@`.
        """

@final
class ArgumentTuple(AST):
    """
    A tuple of arguments in a pool.

    For example, `a,b` in `f(a,b;c)`.
    """

    __match_args__ = ("arguments",)
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, arguments: Sequence[TermOrProjection] = ()
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            arguments: The arguments of the tuple.
        """

    def update(
        self, lib: Library, *, arguments: Sequence[TermOrProjection] = ...
    ) -> ArgumentTuple:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            arguments: The arguments of the tuple.

        Returns:
            The updated node.
        """

    @property
    def arguments(self) -> Sequence[TermOrProjection]:
        """
        The arguments of the tuple.
        """

Term: TypeAlias = (
    TermVariable
    | TermSymbolic
    | TermAbsolute
    | TermUnaryOperation
    | TermBinaryOperation
    | TermTuple
    | TermFunction
    | TermFormatString
)
"""
The available terms.
"""
FormatField: TypeAlias = FormatFieldLiteral | FormatFieldExpression
"""
The available parts of a format string.
"""
TermOrProjection: TypeAlias = Term | Projection
"""
A term or a placeholder for a projected argument.
"""
TermOrArgumentTuple: TypeAlias = Term | ArgumentTuple
"""
A term or an argument tuple in the pool of a tuple term.
"""
