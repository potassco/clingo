'''
This modules contains functions and classes to work with symbolic atoms.
'''

from typing import Collection, Generic, Iterator, List, Optional, Tuple, TypeVar
from abc import abstractmethod

from ._internal import _c_call, _ffi, _handle_error, _lib, _to_str
from .symbol import Symbol

class SymbolicAtom:
    '''
    Captures a symbolic atom and provides properties to inspect its state.
    '''
    def __init__(self, rep, it):
        self._rep = rep
        self._it = it

    def match(self, name: str, arity: int, positive: bool=True) -> bool:
        '''
        Check if the atom matches the given signature.

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

        See Also
        --------
        Symbol.match
        '''
        return self.symbol.match(name, arity, positive)

    @property
    def is_external(self) -> bool:
        '''
        Whether the atom is an external atom.
        '''
        return _c_call('bool', _lib.clingo_symbolic_atoms_is_external, self._rep, self._it)

    @property
    def is_fact(self) -> bool:
        '''
        Whether the atom is a fact.
        '''
        return _c_call('bool', _lib.clingo_symbolic_atoms_is_fact, self._rep, self._it)

    @property
    def literal(self) -> int:
        '''
        The program literal associated with the atom.
        '''
        return _c_call('clingo_literal_t', _lib.clingo_symbolic_atoms_literal, self._rep, self._it)

    @property
    def symbol(self) -> Symbol:
        '''
        The representation of the atom in form of a symbol.
        '''
        return Symbol(_c_call('clingo_symbol_t', _lib.clingo_symbolic_atoms_symbol, self._rep, self._it))

Key = TypeVar('Key')
Value = TypeVar('Value')
class Lookup(Generic[Key, Value], Collection[Value]):
    '''
    A collection of values with additional lookup by key.
    '''
    @abstractmethod
    def __getitem__(self, key: Key) -> Optional[Value]:
        pass

class SymbolicAtoms(Lookup[Symbol,SymbolicAtom]):
    '''
    This class provides read-only access to the atom base of the grounder.

    Implements: `Lookup[Symbol,SymbolicAtom]`.

    Examples
    --------

        >>> import clingo
        >>> prg = clingo.Control()
        >>> prg.add('base', [], """\
        ... p(1).
        ... { p(3) }.
        ... #external p(1..3).
        ...
        ... q(X) :- p(X).
        ... """)
        >>> prg.ground([("base", [])])
        >>> len(prg.symbolic_atoms)
        6
        >>> prg.symbolic_atoms[clingo.Function("p", [2])] is not None
        True
        >>> prg.symbolic_atoms[clingo.Function("p", [4])] is None
        True
        >>> prg.symbolic_atoms.signatures
        [('p', 1L, True), ('q', 1L, True)]
        >>> [(x.symbol, x.is_fact, x.is_external)
        ...  for x in prg.symbolic_atoms.by_signature("p", 1)]
        [(p(1), True, False), (p(3), False, False), (p(2), False, True)]
    '''
    def __init__(self, rep):
        self._rep = rep

    def _iter(self, p_sig) -> Iterator[SymbolicAtom]:
        p_it = _ffi.new('clingo_symbolic_atom_iterator_t*')
        p_valid = _ffi.new('bool*')
        _handle_error(_lib.clingo_symbolic_atoms_begin(self._rep, p_sig, p_it))
        while _c_call(p_valid, _lib.clingo_symbolic_atoms_is_valid, self._rep, p_it[0]):
            yield SymbolicAtom(self._rep, p_it[0])
            _handle_error(_lib.clingo_symbolic_atoms_next(self._rep, p_it[0], p_it))

    def __iter__(self) -> Iterator[SymbolicAtom]:
        yield from self._iter(_ffi.NULL)

    def __contains__(self, symbol) -> bool:
        if not isinstance(symbol, Symbol):
            return False

        it = _c_call('clingo_symbolic_atom_iterator_t', _lib.clingo_symbolic_atoms_find, self._rep, symbol._rep)

        return _c_call('bool', _lib.clingo_symbolic_atoms_is_valid, self._rep, it)

    def __getitem__(self, symbol: Symbol) -> Optional[SymbolicAtom]:
        it = _c_call('clingo_symbolic_atom_iterator_t', _lib.clingo_symbolic_atoms_find, self._rep, symbol._rep)

        if not _c_call('bool', _lib.clingo_symbolic_atoms_is_valid, self._rep, it):
            return None

        return SymbolicAtom(self._rep, it)

    def __len__(self) -> int:
        return _c_call('size_t', _lib.clingo_symbolic_atoms_size, self._rep)

    def by_signature(self, name: str, arity: int, positive: bool=True) -> Iterator[SymbolicAtom]:
        '''
        Return an iterator over the symbolic atoms with the given signature.

        Arguments
        ---------
        name : str
            The name of the signature.
        arity : int
            The arity of the signature.
        positive : bool=True
            The sign of the signature.

        Returns
        -------
        Iterator[SymbolicAtom]
        '''
        p_sig = _ffi.new('clingo_signature_t*')
        _handle_error(_lib.clingo_signature_create(name.encode(), arity, positive, p_sig))
        yield from self._iter(p_sig)

    @property
    def signatures(self) -> List[Tuple[str,int,bool]]:
        '''
        The list of predicate signatures occurring in the program.

        The Boolean indicates the sign of the signature.
        '''
        size = _c_call('size_t', _lib.clingo_symbolic_atoms_signatures_size, self._rep)

        p_sigs = _ffi.new('clingo_signature_t[]', size)
        _handle_error(_lib.clingo_symbolic_atoms_signatures(self._rep, p_sigs, size))

        return [ (_to_str(_lib.clingo_signature_name(c_sig)),
                  _lib.clingo_signature_arity(c_sig),
                  _lib.clingo_signature_is_positive(c_sig)) for c_sig in p_sigs ]
