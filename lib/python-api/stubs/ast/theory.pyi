"""
AST nodes for theory terms and atoms.
"""

from collections.abc import Sequence
from enum import IntEnum
from typing import TypeAlias, final

from clingo.ast import AST
from clingo.ast.literals import Literal
from clingo.core import Library, Location
from clingo.symbol import Symbol

__all__: list[str] = [
    "TheoryAtomElement",
    "TheoryRightGuard",
    "TheoryTerm",
    "TheoryTermFunction",
    "TheoryTermSymbolic",
    "TheoryTermTuple",
    "TheoryTermUnparsed",
    "TheoryTermVariable",
    "TheoryTupleType",
    "UnparsedElement",
]

class TheoryTupleType(IntEnum):
    """
    Enumeration of theory tuple types.
    """

    Tuple = 0
    """
    A tuple `(t1,...,tn)`.
    """
    Set = 1
    """
    A set `{t1,...,tn}`.
    """
    List = 2
    """
    A list `[t1,...,tn]`.
    """

@final
class UnparsedElement(AST):
    """
    An element of an unparsed theory term.

    It consists of a sequence of operators followed by a theory term.
    """

    __match_args__ = ("operators", "term")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, operators: Sequence[str], term: TheoryTerm
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            operators: The operators preceding the theory term.
            term: The theory term.
        """

    def update(
        self, lib: Library, *, operators: Sequence[str] = ..., term: TheoryTerm = ...
    ) -> UnparsedElement:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            operators: The operators preceding the theory term.
            term: The theory term.

        Returns:
            The updated node.
        """

    @property
    def operators(self) -> Sequence[str]:
        """
        The operators preceding the theory term.
        """

    @property
    def term(self) -> TheoryTerm:
        """
        The theory term.
        """

@final
class TheoryTermVariable(AST):
    """
    A theory term representing a variable.

    For example, `X`.
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
    ) -> TheoryTermVariable:
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
class TheoryTermSymbolic(AST):
    """
    A theory term representing a symbol.

    For example, `1` or `a`.
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
            symbol: The symbol.
        """

    def update(
        self, lib: Library, *, location: Location = ..., symbol: Symbol = ...
    ) -> TheoryTermSymbolic:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the symbol.
            symbol: The symbol.

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
        The symbol.
        """

@final
class TheoryTermTuple(AST):
    """
    A theory term representing a tuple, set, or list.

    For example, `(a,b)`, `{a,b}`, or `[a,b]`.
    """

    __match_args__ = ("location", "tuple_type", "arguments")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        tuple_type: TheoryTupleType,
        arguments: Sequence[TheoryTerm],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the tuple.
            tuple_type: The kind of the tuple.
            arguments: The arguments of the tuple.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        tuple_type: TheoryTupleType = ...,
        arguments: Sequence[TheoryTerm] = ...,
    ) -> TheoryTermTuple:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the tuple.
            tuple_type: The kind of the tuple.
            arguments: The arguments of the tuple.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the tuple.
        """

    @property
    def tuple_type(self) -> TheoryTupleType:
        """
        The kind of the tuple.
        """

    @property
    def arguments(self) -> Sequence[TheoryTerm]:
        """
        The arguments of the tuple.
        """

@final
class TheoryTermFunction(AST):
    """
    A theory term representing a function.

    For example, `f(X,a)`.
    """

    __match_args__ = ("location", "name", "arguments")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        name: str,
        arguments: Sequence[TheoryTerm],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the function.
            name: The name of the function.
            arguments: The arguments of the function.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        name: str = ...,
        arguments: Sequence[TheoryTerm] = ...,
    ) -> TheoryTermFunction:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the function.
            name: The name of the function.
            arguments: The arguments of the function.

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
    def arguments(self) -> Sequence[TheoryTerm]:
        """
        The arguments of the function.
        """

@final
class TheoryTermUnparsed(AST):
    """
    A theory term whose operators have not been parsed yet.

    The priorities and associativities of the operators have not been applied yet; the elements are simply stored as a list. For example, `- X + Y * Z`.
    """

    __match_args__ = ("location", "elements")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self, lib: Library, location: Location, elements: Sequence[UnparsedElement]
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the theory term.
            elements: The elements of the theory term.

                There is at least one element and all but the first element have at least one operator.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        elements: Sequence[UnparsedElement] = ...,
    ) -> TheoryTermUnparsed:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the theory term.
            elements: The elements of the theory term.

        Returns:
            The updated node.
        """

    @property
    def location(self) -> Location:
        """
        The location of the theory term.
        """

    @property
    def elements(self) -> Sequence[UnparsedElement]:
        """
        The elements of the theory term.

        There is at least one element and all but the first element have at least one operator.
        """

@final
class TheoryAtomElement(AST):
    """
    An element of a theory atom.

    For example, `X: p(X)` in `&sum { X: p(X) } >= 0`.
    """

    __match_args__ = ("location", "tuple", "condition")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(
        self,
        lib: Library,
        location: Location,
        tuple: Sequence[TheoryTerm],
        condition: Sequence[Literal],
    ) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The theory terms of the element.
            condition: The condition of the element.
        """

    def update(
        self,
        lib: Library,
        *,
        location: Location = ...,
        tuple: Sequence[TheoryTerm] = ...,
        condition: Sequence[Literal] = ...,
    ) -> TheoryAtomElement:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            location: The location of the element.
            tuple: The theory terms of the element.
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
    def tuple(self) -> Sequence[TheoryTerm]:
        """
        The theory terms of the element.
        """

    @property
    def condition(self) -> Sequence[Literal]:
        """
        The condition of the element.
        """

@final
class TheoryRightGuard(AST):
    """
    The right guard of a theory atom.

    For example, `>= 0` in `&sum { X: p(X) } >= 0`.
    """

    __match_args__ = ("theory_operator", "term")
    """
    The names of the arguments for positional patterns.
    """
    def __init__(self, lib: Library, theory_operator: str, term: TheoryTerm) -> None:
        """
        Construct the node.

        Args:
            lib: The library object for storing symbols.
            theory_operator: The theory operator of the guard.
            term: The theory term of the guard.
        """

    def update(
        self, lib: Library, *, theory_operator: str = ..., term: TheoryTerm = ...
    ) -> TheoryRightGuard:
        """
        Create a copy of the node with the given arguments replaced.

        Arguments that are not given keep their current values.

        Args:
            lib: The library object for storing symbols.
            theory_operator: The theory operator of the guard.
            term: The theory term of the guard.

        Returns:
            The updated node.
        """

    @property
    def theory_operator(self) -> str:
        """
        The theory operator of the guard.
        """

    @property
    def term(self) -> TheoryTerm:
        """
        The theory term of the guard.
        """

TheoryTerm: TypeAlias = (
    TheoryTermVariable
    | TheoryTermSymbolic
    | TheoryTermTuple
    | TheoryTermFunction
    | TheoryTermUnparsed
)
"""
The available theory terms.
"""
