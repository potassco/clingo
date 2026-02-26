"""
Functions and classes for symbol manipulation.

Examples
--------

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function, Number, parse_term
>>>
>>> lib = Library()
>>>
>>> num = Number(lib, 42)
>>> num.number
42
>>> fun = Function(lib, "f", [num])
>>> fun.name
'f'
>>> [ str(arg) for arg in fun.arguments ]
['42']
>>> parse_term(lib, str(fun)) == fun
True
>>> parse_term(lib, 'p(1+2)')
p(3)
```
"""

from __future__ import annotations

import enum
import typing

import clingo.core

__all__: list[str] = [
    "Function",
    "Infimum",
    "Number",
    "String",
    "Supremum",
    "Symbol",
    "SymbolType",
    "Tuple_",
    "parse_term",
]

def Function(
    lib: clingo.core.Library,
    name: str,
    arguments: typing.Sequence[Symbol] = [],
    is_positive: bool = True,
) -> Symbol:
    """
    Construct a function symbol.

    This includes constants and tuples. Constants have an empty argument list and
    tuples have an empty name. Functions can represent classically negated atoms.
    Argument `is_positive` has to be set to `False` to represent such atoms.

    Args:
        lib: A library object to store the function in.
        name: The name of the function.
        arguments: The arguments in the form of a list of symbols.
        is_positive: Whether the function is positive.
    """

def Number(lib: clingo.core.Library, number: int) -> Symbol:
    """
    Construct a numeric symbol given a number.

    Args:
        lib: A library object to store the number in.
        number: The given number.
    """

def String(lib: clingo.core.Library, string: str) -> Symbol:
    """
    Construct a string symbol given a string.

    Args:
        lib: A library object to store the string in.
        string: The given string.
    """

def Tuple_(lib: clingo.core.Library, arguments: typing.Sequence[Symbol]) -> Symbol:
    """
    Construct a tuple symbol.

    This is a shortcut for `Function(lib, "", arguments)`.

    Args:
        lib: A library object to store the tuple in.
        arguments: The arguments in form of a list of symbols.
    """

def parse_term(lib: clingo.core.Library, string: str) -> Symbol:
    """
    Parse the given string using clingo's term parser for ground terms.

    The function also evaluates arithmetic functions.

    Args:
        lib: A library object to store parsed symbols in.
        string: The string to be parsed.
    """

Infimum: Symbol  # value = Infimum
Supremum: Symbol  # value = Supremum

class SymbolType(enum.IntEnum):
    """
    Enumeration of symbols types.
    """

    Function: typing.ClassVar[SymbolType]  # value = <SymbolType.Function: 5>
    Infimum: typing.ClassVar[SymbolType]  # value = <SymbolType.Infimum: 2>
    Number: typing.ClassVar[SymbolType]  # value = <SymbolType.Number: 0>
    String: typing.ClassVar[SymbolType]  # value = <SymbolType.String: 3>
    Supremum: typing.ClassVar[SymbolType]  # value = <SymbolType.Supremum: 1>
    Tuple: typing.ClassVar[SymbolType]  # value = <SymbolType.Tuple: 4>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Symbol:
    """
    Represents a clingo symbol.

    This includes `#inf` and `#sup`, numbers, strings, tuples, functions (including
    constants with `len(arguments) == 0`.

    Symbol objects implement Python's rich comparison operators and are ordered
    like in clingo. They can also be used as keys in dictionaries. Their string
    representation corresponds to their clingo representation.

    Note that this class does not have a constructor. Instead there are the
    preconstructed symbols `Infimum` and `Supremum` and the functions `Number`,
    `String`, `Tuple_`, and `Function` to construct symbol objects.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __ge__(self, arg0: typing.Any) -> bool: ...
    def __gt__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __le__(self, arg0: typing.Any) -> bool: ...
    def __lt__(self, arg0: typing.Any) -> bool: ...
    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...
    @typing.overload
    def match(self, name: str, arity: int = 0, is_positive: bool = True) -> bool:
        """
        Check if this is a function symbol with the given signature.

        Args:
            name: The name of the function.
            arity: The arity of the function.
            is_positive: Whether to match positive or negative signatures.

        Returns:
            Whether the function matches.
        """

    @typing.overload
    def match(self, arity: int = 0) -> bool:
        """
        Check if this is a tuple symbol with the given arity.

        Args:
            arity: The arity of the function.

        Returns:
            Whether the tuple matches.
        """

    @property
    def arguments(self) -> typing.Sequence[Symbol]:
        """
        The list of arguments.
        """

    @property
    def arity(self) -> int:
        """
        The arity of a function or tuple.
        """

    @property
    def is_negative(self) -> bool:
        """
        Whether the symbol is negative, e.g, `-1` or `-p`.
        """

    @property
    def is_positive(self) -> bool:
        """
        Whether the symbol is positive, e.g., `1` or `p`.
        """

    @property
    def name(self) -> str:
        """
        The name.
        """

    @property
    def number(self) -> int:
        """
        The numeric value.
        """

    @property
    def signature(self) -> tuple[str, int, bool] | None:
        """
        Get the signature of function symbols.

        The Boolean in the signature is True if the associated symbol is positive.
        """

    @property
    def string(self) -> str:
        """
        The string value.
        """

    @property
    def type(self) -> SymbolType:
        """
        The type of the symbol.
        """
