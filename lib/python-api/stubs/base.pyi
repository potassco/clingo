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

from __future__ import annotations

import collections.abc
import enum
import typing

import clingo.symbol

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

class TheoryTermType(enum.IntEnum):
    """
    Enumeration of theory term types.
    """

    Function: typing.ClassVar[TheoryTermType]  # value = <TheoryTermType.Function: 3>
    List: typing.ClassVar[TheoryTermType]  # value = <TheoryTermType.List: 1>
    Number: typing.ClassVar[TheoryTermType]  # value = <TheoryTermType.Number: 4>
    Set: typing.ClassVar[TheoryTermType]  # value = <TheoryTermType.Set: 2>
    Symbol: typing.ClassVar[TheoryTermType]  # value = <TheoryTermType.Symbol: 5>
    Tuple: typing.ClassVar[TheoryTermType]  # value = <TheoryTermType.Tuple: 0>
    @classmethod
    def __new__(cls, value): ...
    def __format__(self, format_spec):
        """
        Convert to a string according to format_spec.
        """

class Atom:
    """
    A class providing information about symbolic atoms.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __ne__(self, arg0: typing.Any) -> bool: ...
    @property
    def literal(self) -> int:
        """
        Get the program literal of the atom.
        """

    @property
    def symbol(self) -> clingo.symbol.Symbol:
        """
        Get the symbol of the atom.
        """

class AtomBase:
    """
    An class providing information about symbolic atoms.

    Implements `Mapping[Symbol, Atom]`.
    """

    def __contains__(self, key: clingo.symbol.Symbol) -> bool:
        """
        Check if the map contains the given key.
        """

    def __getitem__(self, key: clingo.symbol.Symbol) -> Atom:
        """
        Get the value for the given key.
        """

    def __iter__(self) -> collections.abc.Iterator[clingo.symbol.Symbol]:
        """
        Get an iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number elements in the map.
        """

    def get(
        self, key: clingo.symbol.Symbol, default: Atom | None = None
    ) -> Atom | None:
        """
        Get the value for the given key or the default if absent.
        """

    def items(self) -> collections.abc.Iterator[tuple[clingo.symbol.Symbol, Atom]]:
        """
        Get an iterator over the items in the map.
        """

    def keys(self) -> collections.abc.Iterator[clingo.symbol.Symbol]:
        """
        Get an iterator over the keys in the map.
        """

    def values(self) -> collections.abc.Iterator[Atom]:
        """
        Get an iterator over the values in the map.
        """

class Base:
    """

    A class providing information about symbolic and theory atoms and shown terms.

    Implements `Mapping[tuple[str, int, bool], AtomBase]` providing additional
    overloads to directly lookup symbols and short signatures (assuming a positive
    sign):
    - `__getitem__: Callable[[Symbol], Atom]`
    - `__contains__: Callable[[Symbol], bool]`
    - `__getitem__: Callable[[tuple[str, int]], AtomBase]`
    - `__contains__: Callable[[tuple[str, int]], bool]`
    """

    @typing.overload
    def __contains__(self, key: tuple[str, int, bool]) -> bool:
        """
        Check if the map contains the given key.
        """

    @typing.overload
    def __contains__(self, signature: tuple[str, int]) -> bool:
        """
        Check if there is an atom base with the given (short) signature.
        """

    @typing.overload
    def __contains__(self, symbol: clingo.symbol.Symbol) -> bool:
        """
        Check if there is an atom with the given symbol.
        """

    @typing.overload
    def __getitem__(self, key: tuple[str, int, bool]) -> AtomBase:
        """
        Get the value for the given key.
        """

    @typing.overload
    def __getitem__(self, symbol: clingo.symbol.Symbol) -> Atom:
        """
        Get the atom with the given symbol.
        """

    @typing.overload
    def __getitem__(self, signature: tuple[str, int]) -> AtomBase:
        """
        Get the atom base with the given (short) signature.

        This function provides a shortcut assuming the sign is positive.
        """

    def __iter__(self) -> collections.abc.Iterator[tuple[str, int, bool]]:
        """
        Get an iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number elements in the map.
        """

    def get(
        self, key: tuple[str, int, bool], default: AtomBase | None = None
    ) -> AtomBase | None:
        """
        Get the value for the given key or the default if absent.
        """

    def is_current(self, literal: int) -> bool:
        """
        Check whether a literal has been introduced in the current step.

        Note that all literals introduced before the last solve call are considered
        from a previous step.

        Args:
            literal: The literal to check.
        Returns:
            Whether the literal is subject to projection.
        """

    def is_external(self, literal: int) -> bool:
        """
        Check whether the given program literal corresponds to an external.

        Args:
            literal: The literal to check.
        Returns:
            Whether the literal is external.
        """

    def is_fact(self, literal: int) -> bool:
        """
        Check whether the literal is a fact.

        Args:
            literal: The literal to check.
        Returns:
            Whether the literal is a fact.
        """

    def is_projected(self, literal: int) -> bool:
        """
        Check whether the literal is part of a `#project` directive.

        Args:
            literal: The literal to check.
        Returns:
            Whether the literal is subject to projection.
        """

    def is_shown(self, literal: int) -> bool:
        """
        Check whether the literal is shown via a `#show` directive.

        Args:
            literal: The literal to check.
        Returns:
            Whether the literal is shown.
        """

    def items(self) -> collections.abc.Iterator[tuple[tuple[str, int, bool], AtomBase]]:
        """
        Get an iterator over the items in the map.
        """

    def keys(self) -> collections.abc.Iterator[tuple[str, int, bool]]:
        """
        Get an iterator over the keys in the map.
        """

    def values(self) -> collections.abc.Iterator[AtomBase]:
        """
        Get an iterator over the values in the map.
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

class Term:
    """
    A class providing information about terms.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __ne__(self, arg0: typing.Any) -> bool: ...
    @property
    def condition(self) -> typing.Sequence[typing.Sequence[int]]:
        """
        Get the condition of the term.
        """

    @property
    def symbol(self) -> clingo.symbol.Symbol:
        """
        Get the symbol of the term.
        """

class TermBase:
    """
    A class providing information about shown terms.

    The base is established by the show directives occurring in a program.

    Implements `Mapping[Symbol, Term]`.
    """

    def __contains__(self, key: clingo.symbol.Symbol) -> bool:
        """
        Check if the map contains the given key.
        """

    def __getitem__(self, key: clingo.symbol.Symbol) -> Term:
        """
        Get the value for the given key.
        """

    def __iter__(self) -> collections.abc.Iterator[clingo.symbol.Symbol]:
        """
        Get an iterator over the keys in the map.
        """

    def __len__(self) -> int:
        """
        Get the number elements in the map.
        """

    def get(
        self, key: clingo.symbol.Symbol, default: Term | None = None
    ) -> Term | None:
        """
        Get the value for the given key or the default if absent.
        """

    def items(self) -> collections.abc.Iterator[tuple[clingo.symbol.Symbol, Term]]:
        """
        Get an iterator over the items in the map.
        """

    def keys(self) -> collections.abc.Iterator[clingo.symbol.Symbol]:
        """
        Get an iterator over the keys in the map.
        """

    def values(self) -> collections.abc.Iterator[Term]:
        """
        Get an iterator over the values in the map.
        """

class TheoryAtom:
    """
    A view to inspect a theory atom.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str:
        """
        Get a string representation of the atom.
        """

    @property
    def elements(self) -> typing.Sequence[TheoryElement]:
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
    A class  prooviding information about theory atoms.

    Implements `Sequence[TheoryAtom]`.
    """

    def __contains__(self, value: TheoryAtom) -> bool:
        """
        Check whether the sequence contains the given value.
        """

    def __getitem__(self, index: int) -> TheoryAtom:
        """
        Get the value at the given index.
        """

    def __iter__(self) -> collections.abc.Iterator[TheoryAtom]:
        """
        Get an iterator for the sequence.
        """

    def __len__(self) -> int:
        """
        Get the size of the sequence.
        """

    def __reversed__(self) -> collections.abc.Iterator[TheoryAtom]:
        """
        Get a reverse iterator for the sequence.
        """

    def count(self, value: TheoryAtom) -> int:
        """
        Count how often the given value occurs in the sequence.
        """

    def index(self, value: TheoryAtom) -> int:
        """
        Get the index of the given value in the sequence.
        """

class TheoryElement:
    """
    A view to inspect a theory element.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str:
        """
        Get a string representation of the element.
        """

    @property
    def condition(self) -> typing.Sequence[int]:
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
    def tuple(self) -> typing.Sequence[TheoryTerm]:
        """
        Get the term tuple of a theory element.
        """

class TheoryTerm:
    """
    A view to inspect a theory term.
    """

    def __eq__(self, arg0: typing.Any) -> bool: ...
    def __hash__(self) -> int:
        """
        Compute a hash for the object.
        """

    def __ne__(self, arg0: typing.Any) -> bool: ...
    def __str__(self) -> str:
        """
        Get a string representation of the term.
        """

    @property
    def arguments(self) -> typing.Sequence[TheoryTerm]:
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
