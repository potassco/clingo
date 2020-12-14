'''
This modules contains functions and classes for symbol manipulation.
'''

from typing import Callable, List, Sequence

from enum import Enum
from functools import total_ordering

from ._internal import _c_call, _c_call2, _ffi, _lib, _str, _to_str
from .core import MessageCode

class SymbolType(Enum):
    '''
    Enumeration of the different types of symbols.

    Attributes
    ----------
    Number : SymbolType
        A numeric symbol, e.g., `1`.
    String : SymbolType
        A string symbol, e.g., `"a"`.
    Function : SymbolType
        A function symbol, e.g., `c`, `(1, "a")`, or `f(1,"a")`.
    Infimum : SymbolType
        The `#inf` symbol.
    Supremum : SymbolType
        The `#sup` symbol
    '''
    Function = _lib.clingo_symbol_type_function
    Infimum  = _lib.clingo_symbol_type_infimum
    Number  = _lib.clingo_symbol_type_number
    String  = _lib.clingo_symbol_type_string
    Supremum  = _lib.clingo_symbol_type_supremum

@total_ordering
class Symbol:
    '''
    Represents a gringo symbol.

    This includes numbers, strings, functions (including constants with
    `len(arguments) == 0` and tuples with `len(name) == 0`), `#inf` and `#sup`.

    Symbol objects implement Python's rich comparison operators and are ordered
    like in gringo. They can also be used as keys in dictionaries. Their string
    representation corresponds to their gringo representation.

    Notes
    -----
    Note that this class does not have a constructor. Instead there are the
    functions `Number`, `String`, and `Function` to construct symbol objects or the
    preconstructed symbols `Infimum` and `Supremum`.
    '''
    __slots__ = ('_rep',)

    def __init__(self, rep):
        self._rep = rep

    def __str__(self) -> str:
        return _str(_lib.clingo_symbol_to_string_size, _lib.clingo_symbol_to_string, self._rep)

    def __hash__(self) -> int:
        return _lib.clingo_symbol_hash(self._rep)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Symbol):
            return NotImplemented
        return _lib.clingo_symbol_is_equal_to(self._rep, other._rep)

    def __lt__(self, other: object) -> bool:
        if not isinstance(other, Symbol):
            return NotImplemented
        return _lib.clingo_symbol_is_less_than(self._rep, other._rep)

    def match(self, name: str, arity: int, positive: bool = True) -> bool:
        '''
        Check if this is a function symbol with the given signature.

        Parameters
        ----------
        name : str
            The name of the function.

        arity : int
            The arity of the function.

        positive : bool
            Whether to match positive or negative signatures.

        Returns
        -------
        bool
            Whether the function matches.
        '''
        return (self.type == SymbolType.Function and
                self.positive == positive and
                self.name == name and
                len(self.arguments) == arity)

    @property
    def arguments(self) -> List['Symbol']:
        '''
        The arguments of a function.
        '''
        args, size = _c_call2('clingo_symbol_t*', 'size_t', _lib.clingo_symbol_arguments, self._rep)
        return [Symbol(args[i]) for i in range(size)]

    @property
    def name(self) -> str:
        '''
        The name of a function.
        '''
        return _to_str(_c_call('char*', _lib.clingo_symbol_name, self._rep))

    @property
    def negative(self) -> bool:
        '''
        The inverted sign of a function.
        '''
        return _c_call('bool', _lib.clingo_symbol_is_negative, self._rep)

    @property
    def number(self) -> int:
        '''
        The value of a number.
        '''
        return _c_call('int', _lib.clingo_symbol_number, self._rep)

    @property
    def positive(self) -> bool:
        '''
        The sign of a function.
        '''
        return _c_call('bool', _lib.clingo_symbol_is_positive, self._rep)

    @property
    def string(self) -> str:
        '''
        The value of a string.
        '''
        return _to_str(_c_call('char*', _lib.clingo_symbol_string, self._rep))

    @property
    def type(self) -> SymbolType:
        '''
        The type of the symbol.
        '''
        return SymbolType(_lib.clingo_symbol_type(self._rep))

def Function(name: str, arguments: Sequence[Symbol]=[], positive: bool=True) -> Symbol:
    '''
    Construct a function symbol.

    This includes constants and tuples. Constants have an empty argument list and
    tuples have an empty name. Functions can represent classically negated atoms.
    Argument `positive` has to be set to false to represent such atoms.

    Parameters
    ----------
    name : str
        The name of the function (empty for tuples).
    arguments : Sequence[Symbol]=[]
        The arguments in form of a list of symbols.
    positive : bool=True
        The sign of the function (tuples must not have signs).

    Returns
    -------
    Symbol
    '''
    # pylint: disable=protected-access,invalid-name,dangerous-default-value
    c_args = _ffi.new('clingo_symbol_t[]', len(arguments))
    for i, arg in enumerate(arguments):
        c_args[i] = arg._rep
    sym = _c_call('clingo_symbol_t', _lib.clingo_symbol_create_function,
                  name.encode(), c_args, len(arguments), positive)
    return Symbol(sym)

def Number(number: int) -> Symbol:
    '''
    Construct a numeric symbol given a number.

    Parameters
    ----------
    number : int
        The given number.

    Returns
    -------
    Symbol
    '''
    # pylint: disable=invalid-name
    p_rep = _ffi.new('clingo_symbol_t*')
    _lib.clingo_symbol_create_number(number, p_rep)
    return Symbol(p_rep[0])

def String(string: str) -> Symbol:
    '''
    Construct a string symbol given a string.

    Parameters
    ----------
    string : str
        The given string.

    Returns
    -------
    Symbol
    '''
    # pylint: disable=invalid-name
    return Symbol(_c_call('clingo_symbol_t', _lib.clingo_symbol_create_string, string.encode()))

def Tuple_(arguments: Sequence[Symbol]) -> Symbol:
    '''
    A shortcut for `Function("", arguments)`.

    Parameters
    ----------
    arguments : Sequence[Symbol]
        The arguments in form of a list of symbols.

    Returns
    -------
    Symbol

    See Also
    --------
    Function
    '''
    # pylint: disable=invalid-name
    return Function("", arguments)

_p_infimum = _ffi.new('clingo_symbol_t*')
_p_supremum = _ffi.new('clingo_symbol_t*')
_lib.clingo_symbol_create_infimum(_p_infimum)
_lib.clingo_symbol_create_supremum(_p_supremum)

Infimum: Symbol = Symbol(_p_infimum[0])
Supremum: Symbol = Symbol(_p_supremum[0])

def parse_term(string: str, logger: Callable[[MessageCode,str],None]=None, message_limit: int=20) -> Symbol:
    '''
    Parse the given string using gringo's term parser for ground terms.

    The function also evaluates arithmetic functions.

    Parameters
    ----------
    string : str
        The string to be parsed.
    logger : Callable[[MessageCode,str],None]=None
        Function to intercept messages normally printed to standard error.
    message_limit : int=20
        Maximum number of messages passed to the logger.

    Returns
    -------
    Symbol

    Examples
    --------
        >>> import clingo
        >>> clingo.parse_term('p(1+2)')
        p(3)
    '''
    if logger is not None:
        # pylint: disable=protected-access
        c_handle = _ffi.new_handle(logger)
        c_cb = _lib.pyclingo_logger_callback
    else:
        c_handle = _ffi.NULL
        c_cb = _ffi.NULL
    return Symbol(_c_call('clingo_symbol_t', _lib.clingo_parse_term, string.encode(), c_cb, c_handle, message_limit))
