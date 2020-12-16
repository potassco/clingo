'''
This modules contains functions and classes related to solving.
'''

from typing import Iterator, List, Optional, Sequence, Tuple, Union
from enum import Enum

from ._internal import _c_call, _c_call2, _ffi, _handle_error, _lib
from .symbol import Symbol
from .symbolic_atoms import SymbolicAtoms

__all__ = [ 'Model', 'ModelType', 'SolveControl', 'SolveHandle', 'SolveResult' ]

class SolveResult:
    '''
    Captures the result of a solve call.

    `SolveResult` objects cannot be constructed from Python. Instead they are
    returned by the solve methods of the Control object.
    '''
    def __init__(self, rep):
        self._rep = rep

    @property
    def exhausted(self) -> bool:
        '''
        True if the search space was exhausted.
        '''
        return (_lib.clingo_solve_result_exhausted & self._rep) != 0

    @property
    def interrupted(self) -> bool:
        '''
        True if the search was interrupted.
        '''
        return (_lib.clingo_solve_result_interrupted & self._rep) != 0

    @property
    def satisfiable(self) -> Optional[bool]:
        '''
        True if the problem is satisfiable, False if the problem is unsatisfiable, or
        None if the satisfiablity is not known.
        '''
        if (_lib.clingo_solve_result_satisfiable & self._rep) != 0:
            return True
        if (_lib.clingo_solve_result_unsatisfiable & self._rep) != 0:
            return False
        return None

    @property
    def unknown(self) -> bool:
        '''
        True if the satisfiablity is not known.

        This is equivalent to satisfiable is None.
        '''
        return self.satisfiable is None

    @property
    def unsatisfiable(self) -> Optional[bool]:
        '''
        True if the problem is unsatisfiable, false if the problem is satisfiable, or
        `None` if the satisfiablity is not known.
        '''
        if (_lib.clingo_solve_result_unsatisfiable & self._rep) != 0:
            return True
        if (_lib.clingo_solve_result_satisfiable & self._rep) != 0:
            return False
        return None

    def __str__(self):
        if self.satisfiable:
            return "SAT"
        if self.unsatisfiable:
            return "UNSAT"
        return "UNKNOWN"

class SolveControl:
    '''
    Object that allows for controlling a running search.

    `SolveControl` objects cannot be constructed from Python. Instead they are
    available via `Model.context`.
    '''
    def __init__(self, rep):
        self._rep = rep

    def add_clause(self, literals: Sequence[Union[Tuple[Symbol,bool],int]]) -> None:
        '''
        Add a clause that applies to the current solving step during the search.

        Parameters
        ----------
        literals : Sequence[Union[Tuple[Symbol,bool],int]]
            List of literals either represented as pairs of symbolic atoms and Booleans
            or as program literals.

        Notes
        -----
        This function can only be called in a model callback or while iterating when
        using a `SolveHandle`.
        '''
        atoms = self.symbolic_atoms
        p_lits = _ffi.new('clingo_literal_t[]', len(literals))
        for i, lit in enumerate(literals):
            if isinstance(lit, int):
                p_lits[i] = lit
            else:
                atom = atoms[lit[0]]
                if atom is not None:
                    slit = atom.literal
                else:
                    slit = -1
                p_lits[i] = slit if lit[1] else -slit

        _handle_error(_lib.clingo_solve_control_add_clause(self._rep, p_lits, len(literals)))

    def _invert(self, lit: Union[Tuple[Symbol,bool],int]) -> Union[Tuple[Symbol,bool],int]:
        if isinstance(lit, int):
            return -lit
        return lit[0], not lit[1]

    def add_nogood(self, literals: Sequence[Union[Tuple[Symbol,bool],int]]) -> None:
        '''
        Equivalent to `SolveControl.add_clause` with the literals inverted.
        '''
        self.add_clause([self._invert(lit) for lit in literals])

    @property
    def symbolic_atoms(self) -> SymbolicAtoms:
        '''
        `SymbolicAtoms` object to inspect the symbolic atoms.
        '''
        atoms = _c_call('clingo_symbolic_atoms_t*', _lib.clingo_solve_control_symbolic_atoms, self._rep)
        return SymbolicAtoms(atoms)

class ModelType(Enum):
    '''
    Enumeration of the different types of models.

    Attributes
    ----------
    StableModel : ModelType
        The model captures a stable model.
    BraveConsequences : ModelType
        The model stores the set of brave consequences.
    CautiousConsequences : ModelType
        The model stores the set of cautious consequences.
    '''
    BraveConsequences = _lib.clingo_model_type_brave_consequences
    CautiousConsequences = _lib.clingo_model_type_cautious_consequences
    StableModel = _lib.clingo_model_type_stable_model

class Model:
    '''
    Provides access to a model during a solve call and provides a `SolveContext`
    object to provided limited support to influence the running search.

    Notes
    -----
    The string representation of a model object is similar to the output of models
    by clingo using the default output.

    `Model` objects cannot be constructed from Python. Instead they are obained
    during solving (see `Control.solve`). Furthermore, the lifetime of a model
    object is limited to the scope of the callback it was passed to or until the
    search for the next model is started. They must not be stored for later use.

    Examples
    --------
    The following example shows how to store atoms in a model for usage after
    solving:

        >>> import clingo
        >>> ctl = clingo.Control()
        >>> ctl.add("base", [], "{a;b}.")
        >>> ctl.ground([("base", [])])
        >>> ctl.configuration.solve.models="0"
        >>> models = []
        >>> with ctl.solve(yield_=True) as handle:
        ...     for model in handle:
        ...         models.append(model.symbols(atoms=True))
        ...
        >>> sorted(models)
        [[], [a], [a, b], [b]]
    '''
    def __init__(self, rep):
        self._rep = rep

    def contains(self, atom: Symbol) -> bool:
        '''
        Efficiently check if an atom is contained in the model.

        Parameters
        ----------
        atom : Symbol
            The atom to lookup.

        Returns
        -------
        bool
            Whether the given atom is contained in the model.

        Notes
        -----
        The atom must be represented using a function symbol.
        '''
        # pylint: disable=protected-access
        return _c_call('bool', _lib.clingo_model_contains, self._rep, atom._rep)

    def extend(self, symbols: Sequence[Symbol]) -> None:
        '''
        Extend a model with the given symbols.

        Parameters
        ----------
        symbols : Sequence[Symbol]
            The symbols to add to the model.

        Returns
        -------
        None

        Notes
        -----
        This only has an effect if there is an underlying clingo application, which
        will print the added symbols.
        '''
        # pylint: disable=protected-access
        c_symbols = _ffi.new('clingo_symbol_t[]', len(symbols))
        for i, sym in enumerate(symbols):
            c_symbols[i] = sym._rep
        _handle_error(_lib.clingo_model_extend(self._rep, c_symbols, len(symbols)))

    def is_true(self, literal: int) -> bool:
        '''
        Check if the given program literal is true.

        Parameters
        ----------
        literal : int
            The given program literal.

        Returns
        -------
        bool
            Whether the given program literal is true.
        '''
        return _c_call('bool', _lib.clingo_model_is_true, self._rep, literal)

    def symbols(self, atoms: bool=False, terms: bool=False, shown: bool=False, csp: bool=False,
                theory: bool=False, complement: bool=False) -> List[Symbol]:
        '''
        Return the list of atoms, terms, or CSP assignments in the model.

        Parameters
        ----------
        atoms : bool=False
            Select all atoms in the model (independent of `#show` statements).
        terms : bool=False
            Select all terms displayed with `#show` statements in the model.
        shown : bool=False
            Select all atoms and terms as outputted by clingo.
        csp : bool=False
            Select all csp assignments (independent of `#show` statements).
        theory : bool=False
            Select atoms added with `Model.extend`.
        complement : bool=False
            Return the complement of the answer set w.r.t. to the atoms known to the
            grounder. (Does not affect csp assignments.)

        Returns
        -------
        List[Symbol]
            The selected symbols.

        Notes
        -----
        Atoms are represented using functions (`Symbol` objects), and CSP assignments
        are represented using functions with name `"$"` where the first argument is the
        name of the CSP variable and the second its value.
        '''
        show = 0
        if atoms:
            show |= _lib.clingo_show_type_atoms
        if terms:
            show |= _lib.clingo_show_type_terms
        if shown:
            show |= _lib.clingo_show_type_shown
        if csp:
            show |= _lib.clingo_show_type_csp
        if theory:
            show |= _lib.clingo_show_type_theory
        if complement:
            show |= _lib.clingo_show_type_complement

        size = _c_call('size_t', _lib.clingo_model_symbols_size, self._rep, show)

        p_symbols = _ffi.new('clingo_symbol_t[]', size)
        _handle_error(_lib.clingo_model_symbols(self._rep, show, p_symbols, size))

        symbols = []
        for c_symbol in p_symbols:
            symbols.append(Symbol(c_symbol))
        return symbols

    def __str__(self):
        return " ".join(map(str, self.symbols(shown=True)))

    @property
    def context(self) -> SolveControl:
        '''
        Object that allows for controlling the running search.
        '''
        ctl = _c_call('clingo_solve_control_t*', _lib.clingo_model_context, self._rep)
        return SolveControl(ctl)

    @property
    def cost(self) -> List[int]:
        '''
        Return the list of integer cost values of the model.

        The return values correspond to clasp's cost output.
        '''
        size = _c_call('size_t', _lib.clingo_model_cost_size, self._rep)

        p_costs = _ffi.new('int64_t[]', size)
        _handle_error(_lib.clingo_model_cost(self._rep, p_costs, size))

        return list(p_costs)

    @property
    def number(self) -> int:
        '''
        The running number of the model.
        '''
        return _c_call('uint64_t', _lib.clingo_model_number, self._rep)

    @property
    def optimality_proven(self) -> bool:
        '''
        Whether the optimality of the model has been proven.
        '''
        return _c_call('bool', _lib.clingo_model_optimality_proven, self._rep)

    @property
    def thread_id(self) -> int:
        '''
        The id of the thread which found the model.
        '''
        return _c_call('clingo_id_t', _lib.clingo_model_thread_id, self._rep)

    @property
    def type(self) -> ModelType:
        '''
        The type of the model.
        '''
        return ModelType(_c_call('clingo_model_type_t', _lib.clingo_model_type, self._rep))

class SolveHandle:
    '''
    Handle for solve calls.

    `SolveHandle` objects cannot be created from Python. Instead they are returned
    by `Control.solve`. They can be used to control solving, like, retrieving
    models or cancelling a search.

    Implements: `ContextManager[SolveHandle]`.

    See Also
    --------
    Control.solve

    Notes
    -----
    A `SolveHandle` is a context manager and must be used with Python's `with`
    statement.

    Blocking functions in this object release the GIL. They are not thread-safe
    though.
    '''
    def __init__(self, rep, handler):
        self._rep = rep
        self._handler = handler

    def __iter__(self) -> Iterator[Model]:
        while True:
            self.resume()
            m = self.model()
            if m is None:
                break
            yield m

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        _handle_error(_lib.clingo_solve_handle_close(self._rep), self._handler)
        return False

    def cancel(self) -> None:
        '''
        Cancel the running search.

        Returns
        -------
        None

        See Also
        --------
        Control.interrupt
        '''
        _handle_error(_lib.clingo_solve_handle_cancel(self._rep), self._handler)

    def core(self) -> List[int]:
        '''
        The subset of assumptions that made the problem unsatisfiable.

        Returns
        -------
        List[int]
        '''
        core, size = _c_call2('clingo_literal_t*', 'size_t', _lib.clingo_solve_handle_core,
                              self._rep, handler=self._handler)
        return [core[i] for i in range(size)]

    def get(self) -> SolveResult:
        '''
        Get the result of a solve call.

        If the search is not completed yet, the function blocks until the result is
        ready.

        Returns
        -------
        SolveResult
        '''
        res = _c_call('clingo_solve_result_bitset_t', _lib.clingo_solve_handle_get, self._rep, handler=self._handler)
        return SolveResult(res)

    def model(self) -> Optional[Model]:
        '''
        Get the current model if there is any.

        Examples
        --------
        The following example shows how to implement a custom solve loop. While more
        cumbersome than using a for loop, this kind of loop allows for fine grained
        timeout handling between models:

            >>> import clingo
            >>> ctl = clingo.Control()
            >>> ctl.configuration.solve.models = 0
            >>> ctl.add("base", [], "1 {a;b}.")
            >>> ctl.ground([("base", [])])
            >>> with prg.solve(yield_=True, async_=True) as hnd:
            ...     while True:
            ...         hnd.resume()
            ...         _ = hnd.wait()
            ...         m = hnd.model()
            ...         print(m)
            ...         if m is None:
            ...             break
            b
            a
            a b
            None
        '''
        p_model = _ffi.new('clingo_model_t**')
        _handle_error(
            _lib.clingo_solve_handle_model(self._rep, p_model),
            self._handler)
        if p_model[0] == _ffi.NULL:
            return None
        return Model(p_model[0])

    def resume(self) -> None:
        '''
        Discards the last model and starts searching for the next one.

        Notes
        -----
        If the search has been started asynchronously, this function starts the search
        in the background.
        '''
        _handle_error(_lib.clingo_solve_handle_resume(self._rep), self._handler)

    def wait(self, timeout: Optional[float]=None) -> bool:
        '''
        Wait for solve call to finish or the next result with an optional timeout.

        Parameters
        ----------
        timeout : Optional[float]=None
            If a timeout is given, the function blocks for at most timeout seconds.

        Returns
        -------
        bool
            Returns a Boolean indicating whether the solve call has finished or the
            next result is ready.
        '''
        p_res = _ffi.new('bool*')
        _lib.clingo_solve_handle_wait(self._rep, 0 if timeout is None else timeout, p_res)
        return p_res[0]
