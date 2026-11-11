"""
Functions and classes to work with atom and term bases.

# Examples

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Function, Number
>>> from clingo.control import Control
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string(\"\"\"\\
... p(1).
... { p(3) }.
... #external p(1..3).
...
... q(X) :- p(X).
... \"\"\")
>>> ctl.ground()
>>> bse = ctl.base
>>> len(bse)
2
>>> p = bse[("p", 1)]
>>> Function(lib, "p", [Number(lib, 2)]) in p
True
>>> Function(lib, "p", [Number(lib, 4)]) in p
False
>>> [sig for sig in bse]
[('p', 1, False), ('q', 1, False)]
>>> [(str(x.symbol), bse.is_fact(x.literal), bse.is_external(x.literal))
...  for x in p.values()]
[('p(1)', True, False), ('p(3)', False, False), ('p(2)', False, True)]
```
"""

import enum
import typing
from collections.abc import Iterator, Sequence

from clingo.symbol import Symbol

__all__: list[str] = [
    "Atom",
    "AtomBase",
    "Base",
    "Term",
    "TermBase",
    "TheoryAtom",
    "TheoryBase",
    "TheoryElement",
    "TheoryTerm",
    "TheoryTermType",
]

_T = typing.TypeVar("_T")

class TheoryTermType(enum.IntEnum):
    """
    Enumeration of theory term types.
    """

    Function = 3
    """
    For function theory terms.
    """
    List = 1
    """
    For list theory terms.
    """
    Number = 4
    """
    For numeric theory terms.
    """
    Set = 2
    """
    For set theory terms.
    """
    Symbol = 5
    """
    For symbolic theory terms (simple strings).
    """
    Tuple = 0
    """
    For tuple theory terms.
    """

class Atom:
    """
    A class providing information about symbolic atoms.
    """

    def __eq__(self, other: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, other: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __hash__(self) -> int:
        """
        Compute a hash for the object.

        Returns:
            The hash value of the object.
        """

    @property
    def literal(self) -> int:
        """
        Get the program literal of the atom.
        """

    @property
    def symbol(self) -> Symbol:
        """
        Get the symbol of the atom.
        """

class AtomBase:
    """
    A class providing information about symbolic atoms.

    Implements a mapping from symbols to atoms.
    """

    def __contains__(self, key: Symbol, /) -> bool:
        """
        Check if the map contains the given key.

        Args:
            key: The key to check.

        Returns:
            Whether the key is present in the map.
        """

    def __getitem__(self, key: Symbol, /) -> Atom:
        """
        Get the value for the given key.

        Args:
            key: The key to look up.

        Returns:
            The value for the given key.
        """

    def __iter__(self) -> Iterator[Symbol]:
        """
        Get an iterator over the keys in the map.

        Returns:
            An iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number elements in the map.

        Returns:
            The number of elements in the map.
        """

    @typing.overload
    def get(self, key: Symbol, default: None = None) -> Atom | None:
        """
        Get the value for the given key or None if absent.
        """

    @typing.overload
    def get(self, key: Symbol, default: _T) -> Atom | _T:
        """
        Get the value for the given key or the default if absent.
        """

    def get(self, key: Symbol, default: _T | None = None) -> Atom | _T | None:
        """
        Get the value for the given key or the default if absent.

        If the key is not present in the map, the default value is returned.
        If no default value is given, `None` is returned.

        Args:
            key: The key to look up.
            default: The default value to return if the key is absent.

        Returns:
            The value for the given key or the default if absent.
        """

    def items(self) -> Iterator[tuple[Symbol, Atom]]:
        """
        Get an iterator over the items in the map.

        Returns:
            An iterator over the items in the map.
        """

    def keys(self) -> Iterator[Symbol]:
        """
        Get an iterator over the keys in the map.

        Returns:
            An iterator over the keys in the map.
        """

    def values(self) -> Iterator[Atom]:
        """
        Get an iterator over the values in the map.

        Returns:
            An iterator over the values in the map.
        """

class Term:
    """
    A class providing information about terms.
    """

    def __eq__(self, other: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, other: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __hash__(self) -> int:
        """
        Compute a hash for the object.

        Returns:
            The hash value of the object.
        """

    @property
    def condition(self) -> Sequence[Sequence[int]]:
        """
        Get the condition of the term.
        """

    @property
    def symbol(self) -> Symbol:
        """
        Get the symbol of the term.
        """

class TermBase:
    """
    A class providing information about shown terms.

    The base is established by the show directives occurring in a program.

    Implements a mapping from symbols to terms.
    """

    def __contains__(self, key: Symbol, /) -> bool:
        """
        Check if the map contains the given key.

        Args:
            key: The key to check.

        Returns:
            Whether the key is present in the map.
        """

    def __getitem__(self, key: Symbol, /) -> Term:
        """
        Get the value for the given key.

        Args:
            key: The key to look up.

        Returns:
            The value for the given key.
        """

    def __iter__(self) -> Iterator[Symbol]:
        """
        Get an iterator over the keys in the map.

        Returns:
            An iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number elements in the map.

        Returns:
            The number of elements in the map.
        """

    @typing.overload
    def get(self, key: Symbol, default: None = None) -> Term | None:
        """
        Get the value for the given key or None if absent.
        """

    @typing.overload
    def get(self, key: Symbol, default: _T) -> Term | _T:
        """
        Get the value for the given key or the default if absent.
        """

    def get(self, key: Symbol, default: _T | None = None) -> Term | _T | None:
        """
        Get the value for the given key or the default if absent.

        If the key is not present in the map, the default value is returned.
        If no default value is given, `None` is returned.

        Args:
            key: The key to look up.
            default: The default value to return if the key is absent.

        Returns:
            The value for the given key or the default if absent.
        """

    def items(self) -> Iterator[tuple[Symbol, Term]]:
        """
        Get an iterator over the items in the map.

        Returns:
            An iterator over the items in the map.
        """

    def keys(self) -> Iterator[Symbol]:
        """
        Get an iterator over the keys in the map.

        Returns:
            An iterator over the keys in the map.
        """

    def values(self) -> Iterator[Term]:
        """
        Get an iterator over the values in the map.

        Returns:
            An iterator over the values in the map.
        """

class TheoryTerm:
    """
    A view to inspect a theory term.
    """

    def __eq__(self, other: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, other: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __hash__(self) -> int:
        """
        Compute a hash for the object.

        Returns:
            The hash value of the object.
        """

    @property
    def arguments(self) -> Sequence[TheoryTerm]:
        """
        Get the arguments of a function, tuple, list, or set theory term.
        """

    @property
    def name(self) -> str:
        """
        Get the name of a theory symbol or function.
        """

    @property
    def number(self) -> int:
        """
        Get the value of a numeric theory term.
        """

    @property
    def type(self) -> TheoryTermType:
        """
        Get the type of the theory term.
        """

class TheoryElement:
    """
    A view to inspect a theory element.
    """

    def __eq__(self, other: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, other: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __hash__(self) -> int:
        """
        Compute a hash for the object.

        Returns:
            The hash value of the object.
        """

    @property
    def condition(self) -> Sequence[int]:
        """
        Get the condition of a theory element.
        """

    @property
    def condition_id(self) -> int | None:
        """
        Get the condition id of a theory element.

        True conditions do not have a condition id. A condition id is only valid for
        the current solving step. However, they can be mapped to persistent solver
        literals using `clingo.propagate.PropagateInit.solver_literal`.
        """

    @property
    def tuple(self) -> Sequence[TheoryTerm]:
        """
        Get the term tuple of a theory element.
        """

class TheoryAtom:
    """
    A view to inspect a theory atom.
    """

    def __eq__(self, other: object, /) -> bool:
        """
        Compare with another object for equality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are equal.
        """

    def __ne__(self, other: object, /) -> bool:
        """
        Compare with another object for inequality.

        Args:
            other: The object to compare with.

        Returns:
            Whether the two objects are not equal.
        """

    def __hash__(self) -> int:
        """
        Compute a hash for the object.

        Returns:
            The hash value of the object.
        """

    @property
    def elements(self) -> Sequence[TheoryElement]:
        """
        Get the elements of a theory atom.
        """

    @property
    def guard(self) -> tuple[str, TheoryTerm] | None:
        """
        Get optional guard of a theory atom.
        """

    @property
    def literal(self) -> int:
        """
        Get the literal of the theory atom (zero for directives).
        """

    @property
    def name(self) -> TheoryTerm:
        """
        Get the name of a theory atom.
        """

class TheoryBase:
    """
    A class providing information about theory atoms.

    Implements a sequence of theory atoms.
    """

    def __contains__(self, value: TheoryAtom, /) -> bool:
        """
        Check whether the sequence contains the given value.

        Args:
            value: The value to check.

        Returns:
            Whether the value is present in the sequence.
        """

    def __getitem__(self, index: int, /) -> TheoryAtom:
        """
        Get the value at the given index.

        Args:
            index: The index to look up.

        Returns:
            The value at the given index.
        """

    def __iter__(self) -> Iterator[TheoryAtom]:
        """
        Get an iterator for the sequence.

        Returns:
            An iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.

        Returns:
            The size of the sequence.
        """

class Base:
    """
    A class providing information about symbolic and theory atoms and shown terms.

    Implements a mapping from signatures to atom bases and provides access to
    the term and theory bases.
    """

    def __contains__(
        self, key: tuple[str, int, bool] | tuple[str, int] | Symbol, /
    ) -> bool:
        """
        Check if there is an atom base for the given signature or an atom for the given symbol.

        Signatures are given as a tuple of the form `(name, arity, sign)` or
        `(name, arity)`. The latter form is a shortcut assuming the sign is
        positive.

        Args:
            key: The signature or symbol to look up.

        Returns:
            Whether the signature or symbol is present in the base.
        """

    @typing.overload
    def __getitem__(self, signature: tuple[str, int, bool], /) -> AtomBase:
        """
        Get the atom base with the given signature.
        """

    @typing.overload
    def __getitem__(self, signature: tuple[str, int], /) -> AtomBase:
        """
        Get the atom base with the given (short) signature.

        This function provides a shortcut assuming the sign is positive.
        """

    @typing.overload
    def __getitem__(self, symbol: Symbol, /) -> Atom:
        """
        Get the atom with the given symbol.
        """

    def __getitem__(
        self, key: tuple[str, int, bool] | tuple[str, int] | Symbol, /
    ) -> AtomBase | Atom:
        """
        Get the atom base for the given signature or the atom for the given symbol.

        Signatures are given as a tuple of the form `(name, arity, sign)` or
        `(name, arity)`. They are mapped to the corresponding atom base. The
        latter form is a shortcut assuming the sign is positive.

        Symbols are mapped to the corresponding atom.

        Args:
            key: The signature or symbol to look up.

        Returns:
            The atom base or atom for the given signature or symbol.
        """

    def __iter__(self) -> Iterator[tuple[str, int, bool]]:
        """
        Get an iterator over the keys in the map.

        Returns:
            An iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number elements in the map.

        Returns:
            The number of elements in the map.
        """

    @typing.overload
    def get(self, key: tuple[str, int, bool], default: None = None) -> AtomBase | None:
        """
        Get the atom base for the given signature or None if absent.
        """

    @typing.overload
    def get(self, key: tuple[str, int, bool], default: _T) -> AtomBase | _T:
        """
        Get the atom base for the given signature or the default if absent.
        """

    @typing.overload
    def get(self, key: tuple[str, int], default: None = None) -> AtomBase | None:
        """
        Get the atom base for the given (short) signature or the default if absent.
        """

    @typing.overload
    def get(self, key: tuple[str, int], default: _T) -> AtomBase | _T:
        """
        Get the atom base for the given (short) signature or the default if absent.
        """

    @typing.overload
    def get(self, key: Symbol, default: None = None) -> Atom | None:
        """
        Get the atom for the given symbol or None if absent.
        """

    @typing.overload
    def get(self, key: Symbol, default: _T) -> Atom | _T:
        """
        Get the atom for the given symbol or the default if absent.
        """

    def get(
        self,
        key: tuple[str, int, bool] | tuple[str, int] | Symbol,
        default: _T | None = None,
    ) -> AtomBase | Atom | _T | None:
        """
        Get the atom base or atom for the given signature or symbol or the default if absent.

        Signatures are given as a tuple of the form `(name, arity, sign)` or
        `(name, arity)`. They are mapped to the corresponding atom base. The
        latter form is a shortcut assuming the sign is positive.

        Symbols are mapped to the corresponding atom.

        If the key is not present in the base, the default value is returned.
        If no default value is given, `None` is returned.

        Args:
            key: The signature or symbol to look up.
            default: The default value to return if the key is absent.

        Returns:
            The atom base or atom for the given signature or symbol, or the default if absent.
        """

    def is_current(self, literal: int, /) -> bool:
        """
        Check whether a literal has been introduced in the current step.

        Note that all literals introduced before the last solve call are considered
        from a previous step.

        Args:
            literal: The literal to check.

        Returns:
            Whether the literal is subject to projection.
        """

    def is_external(self, literal: int, /) -> bool:
        """
        Check whether the given program literal corresponds to an external.

        Args:
            literal: The literal to check.

        Returns:
            Whether the literal is external.
        """

    def is_fact(self, literal: int, /) -> bool:
        """
        Check whether the literal is a fact.

        Args:
            literal: The literal to check.

        Returns:
            Whether the literal is a fact.
        """

    def is_projected(self, literal: int, /) -> bool:
        """
        Check whether the literal is part of a `#project` directive.

        Args:
            literal: The literal to check.

        Returns:
            Whether the literal is subject to projection.
        """

    def is_shown(self, literal: int, /) -> bool:
        """
        Check whether the literal is shown via a `#show` directive.

        Args:
            literal: The literal to check.

        Returns:
            Whether the literal is shown.
        """

    def items(self) -> Iterator[tuple[tuple[str, int, bool], AtomBase]]:
        """
        Get an iterator over the items in the map.

        Returns:
            An iterator over the items in the map.
        """

    def keys(self) -> Iterator[tuple[str, int, bool]]:
        """
        Get an iterator over the keys in the map.

        Returns:
            An iterator over the keys in the map.
        """

    def values(self) -> Iterator[AtomBase]:
        """
        Get an iterator over the values in the map.

        Returns:
            An iterator over the values in the map.
        """

    @property
    def terms(self) -> TermBase:
        """
        The term base (given by show directives).
        """

    @property
    def theory(self) -> TheoryBase:
        """
        The theory base.
        """
