'''
This modules provides the `clingo.control.Control` class responsible for
controling grounding and solving.
'''

from typing import Any, Callable, Iterator, Optional, Sequence, Tuple, Union, cast, overload
from collections import abc
import sys

from ._internal import _CBData, _Error, _cb_error_handler, _c_call, _ffi, _handle_error, _lib, _overwritten
from .core import MessageCode
from .symbol import Symbol
from .symbolic_atoms import SymbolicAtoms
from .theory_atoms import TheoryAtom
from .solving import Model, SolveHandle, SolveResult
from .propagator import Propagator
from .backend import Backend, Observer
from .configuration import Configuration
from .statistics import StatisticsMap, _mutable_statistics, _statistics

# {{{1 control [90%]

class _SolveEventHandler:
    # pylint: disable=missing-function-docstring
    def __init__(self, on_model, on_statistics, on_finish):
        self._on_model = on_model
        self._on_statistics = on_statistics
        self._on_finish = on_finish

    def on_model(self, m):
        ret = None
        if self._on_model is not None:
            ret = self._on_model(Model(m))
        return bool(ret or ret is None)

    def on_finish(self, res):
        if self._on_finish is not None:
            self._on_finish(SolveResult(res))

    def on_statistics(self, step, accu):
        if self._on_statistics is not None:
            self._on_statistics(_mutable_statistics(step), _mutable_statistics(accu))

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_solve_event_callback(type_, event, data, goon):
    '''
    Low-level solve event handler.
    '''
    handler = _ffi.from_handle(data).data
    if type_ == _lib.clingo_solve_event_type_finish:
        handler.on_finish(_ffi.cast('clingo_solve_result_bitset_t*', event)[0])

    if type_ == _lib.clingo_solve_event_type_model:
        goon[0] = handler.on_model(_ffi.cast('clingo_model_t*', event))

    if type_ == _lib.clingo_solve_event_type_statistics:
        p_stats = _ffi.cast('clingo_statistics_t**', event)
        handler.on_statistics(p_stats[0], p_stats[1])

    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_ground_callback(location, name, arguments, arguments_size, data, symbol_callback, symbol_callback_data):
    '''
    Low-level ground callback.
    '''
    # Note: location could be attached to error message
    # pylint: disable=protected-access,unused-argument
    context = _ffi.from_handle(data).data
    py_name = _ffi.string(name).decode()
    fun = getattr(sys.modules['__main__'] if context is None else context, py_name)

    args = []
    for i in range(arguments_size):
        args.append(Symbol(arguments[i]))

    ret = fun(*args)
    symbols = list(ret) if isinstance(ret, abc.Iterable) else [ret]

    c_symbols = _ffi.new('clingo_symbol_t[]', len(symbols))
    for i, sym in enumerate(symbols):
        c_symbols[i] = sym._rep
    _handle_error(symbol_callback(c_symbols, len(symbols), symbol_callback_data))

    return True

class Control:
    '''
    Control object for the grounding/solving process.

    Parameters
    ----------
    arguments : Sequence[str]
        Arguments to the grounder and solver.
    logger : Callable[[MessageCode,str],None]=None
        Function to intercept messages normally printed to standard error.
    message_limit : int=20
        The maximum number of messages passed to the logger.

    Notes
    -----
    Note that only gringo options (without `--text`) and clasp's search options are
    supported. Furthermore, a `Control` object is blocked while a search call is
    active; you must not call any member function during search.

    The second overload is for internal purposes.
    '''
    @overload
    def __init__(self, arguments: Union[Sequence[str],Any]=[],
                 logger: Callable[[MessageCode,str],None]=None, message_limit: int=20):
        # pylint: disable=dangerous-default-value
        ...
    @overload
    def __init__(self, rep: Any):
        ...
    def __init__(self, arguments=[], logger=None, message_limit=20):
        # pylint: disable=protected-access,dangerous-default-value
        self._free = False
        self._mem = []
        if isinstance(arguments, abc.Sequence):
            if logger is not None:
                c_handle = _ffi.new_handle(logger)
                c_cb = _lib.pyclingo_logger_callback
                self._mem.append(c_handle)
            else:
                c_handle = _ffi.NULL
                c_cb = _ffi.NULL
            c_mem = []
            c_args = _ffi.new('char*[]', len(arguments))
            for i, arg in enumerate(arguments):
                c_mem.append(_ffi.new("char[]", arg.encode()))
                c_args[i] = c_mem[-1]
            self._rep = _c_call('clingo_control_t *', _lib.clingo_control_new,
                                c_args, len(arguments), c_cb, c_handle, message_limit)
            self._free = True
        else:
            self._rep = arguments

        self._handler = None
        self._statistics = None
        self._statistics_call = -1.0
        self._error = _Error()

    def __del__(self):
        if self._free:
            _lib.clingo_control_free(self._rep)

    def add(self, name: str, parameters: Sequence[str], program: str) -> None:
        '''
        Extend the logic program with the given non-ground logic program in string form.

        Parameters
        ----------
        name : str
            The name of program block to add.
        parameters : Sequence[str]
            The parameters of the program block to add.
        program : str
            The non-ground program in string form.

        Returns
        -------
        None

        See Also
        --------
        Control.ground
        '''
        c_mem = []
        c_params = _ffi.new('char*[]', len(parameters))
        for i, param in enumerate(parameters):
            c_mem.append(_ffi.new("char[]", param.encode()))
            c_params[i] = c_mem[-1]
        _handle_error(_lib.clingo_control_add(self._rep, name.encode(), c_params, len(parameters), program.encode()))

    def _program_atom(self, lit: Union[Symbol,int]) -> int:
        if isinstance(lit, int):
            return lit
        satom = self.symbolic_atoms[lit]
        return 0 if satom is None else satom.literal

    def assign_external(self, external: Union[Symbol,int], truth: Optional[bool]) -> None:
        '''
        Assign a truth value to an external atom.

        Parameters
        ----------
        external : Union[Symbol,int]
            A symbol or program literal representing the external atom.
        truth : Optional[bool]
            A Boolean fixes the external to the respective truth value; and None leaves
            its truth value open.

        Returns
        -------
        None

        See Also
        --------
        Control.release_external, clingo.solving.SolveControl.symbolic_atoms,
        clingo.symbolic_atoms.SymbolicAtom.is_external

        Notes
        -----
        The truth value of an external atom can be changed before each solve call. An
        atom is treated as external if it has been declared using an `#external`
        directive, and has not been released by calling release_external() or defined
        in a logic program with some rule. If the given atom is not external, then the
        function has no effect.

        For convenience, the truth assigned to atoms over negative program literals is
        inverted.
        '''

        if truth is None:
            val = _lib.clingo_external_type_free
        elif truth:
            val = _lib.clingo_external_type_true
        else:
            val = _lib.clingo_external_type_false
        _handle_error(_lib.clingo_control_assign_external(self._rep, self._program_atom(external), val))

    def backend(self) -> Backend:
        '''
        Returns a `Backend` object providing a low level interface to extend a logic
        program.

        Returns
        -------
        Backend
        '''
        return Backend(_c_call('clingo_backend_t*', _lib.clingo_control_backend, self._rep), self._error)

    def cleanup(self) -> None:
        '''
        Cleanup the domain used for grounding by incorporating information from the
        solver.

        This function cleans up the domain used for grounding.  This is done by first
        simplifying the current program representation (falsifying released external
        atoms).  Afterwards, the top-level implications are used to either remove atoms
        from the domain or mark them as facts.

        Returns
        -------
        None

        See Also
        --------
        Control.enable_cleanup

        Notes
        -----
        Any atoms falsified are completely removed from the logic program. Hence, a
        definition for such an atom in a successive step introduces a fresh atom.

        With the current implementation, the function only has an effect if called
        after solving and before any function is called that starts a new step.

        Typically, it is not necessary to call this function manually because automatic
        cleanups are enabled by default.
        '''
        _handle_error(_lib.clingo_control_cleanup(self._rep))

    def get_const(self, name: str) -> Optional[Symbol]:
        '''
        Return the symbol for a constant definition of form: `#const name = symbol.`

        Parameters
        ----------
        name : str
            The name of the constant to retrieve.

        Returns
        -------
        Optional[Symbol]
            The function returns `None` if no matching constant definition exists.
        '''
        if not _c_call('bool', _lib.clingo_control_has_const, self._rep, name.encode()):
            return None

        return Symbol(_c_call('clingo_symbol_t', _lib.clingo_control_get_const, self._rep, name.encode()))

    def ground(self, parts: Sequence[Tuple[str,Sequence[Symbol]]], context: Any=None) -> None:
        '''
        Ground the given list of program parts specified by tuples of names and arguments.

        Parameters
        ----------
        parts : Sequence[Tuple[str,Sequence[Symbol]]]
            List of tuples of program names and program arguments to ground.
        context : Any=None
            A context object whose methods are called during grounding using the
            `@`-syntax (if omitted methods, those from the main module are used).

        Notes
        -----
        Note that parts of a logic program without an explicit `#program` specification
        are by default put into a program called `base` without arguments.

        Examples
        --------

            >>> import clingo
            >>> ctl = clingo.Control()
            >>> ctl.add("p", ["t"], "q(t).")
            >>> parts = []
            >>> parts.append(("p", [1]))
            >>> parts.append(("p", [2]))
            >>> ctl.ground(parts)
            >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
            Answer: q(1) q(2)
            SAT
        '''
        # pylint: disable=protected-access,dangerous-default-value
        self._error.clear()
        handler = _CBData(context, self._error)
        c_handler = _ffi.new_handle(handler)

        c_mem = []
        c_parts = _ffi.new("clingo_part_t[]", len(parts))
        for part, c_part in zip(parts, c_parts):
            c_mem.append(_ffi.new("char[]", part[0].encode()))
            c_part.name = c_mem[-1]
            c_mem.append(_ffi.new("clingo_symbol_t[]", len(part[1])))
            c_part.params = c_mem[-1]
            for i, sym in enumerate(part[1]):
                c_part.params[i] = sym._rep
            c_part.size = len(part[1])

        _handle_error(_lib.clingo_control_ground(
            self._rep, c_parts, len(parts), _lib.pyclingo_ground_callback, c_handler), handler)

    def interrupt(self) -> None:
        '''
        Interrupt the active solve call.

        Returns
        -------
        None

        Notes
        -----
        This function is thread-safe and can be called from a signal handler. If no
        search is active, the subsequent call to `Control.solve` is interrupted. The
        result of the `Control.solve` method can be used to query if the search was
        interrupted.
        '''
        _lib.clingo_control_interrupt(self._rep)

    def load(self, path: str) -> None:
        '''
        Extend the logic program with a (non-ground) logic program in a file.

        Parameters
        ----------
        path : str
            The path of the file to load.

        Returns
        -------
        None
        '''
        _handle_error(_lib.clingo_control_load(self._rep, path.encode()))

    def register_observer(self, observer: Observer, replace: bool=False) -> None:
        '''
        Registers the given observer to inspect the produced grounding.

        Parameters
        ----------
        observer : Observer
            The observer to register. See below for a description of the requirede
            interface.
        replace : bool=False
            If set to true, the output is just passed to the observer and nolonger to
            the underlying solver (or any previously registered observers).

        Returns
        -------
        None

        Notes
        -----
        Not all functions the `Observer` interface have to be implemented and can be
        omitted if not needed.
        '''
        # pylint: disable=protected-access,line-too-long
        c_observer = _ffi.new('clingo_ground_program_observer_t*', (
            _lib.pyclingo_observer_init_program if _overwritten(Observer, observer, "init_program") else _ffi.NULL,
            _lib.pyclingo_observer_begin_step if _overwritten(Observer, observer, "begin_step") else _ffi.NULL,
            _lib.pyclingo_observer_end_step if _overwritten(Observer, observer, "end_step") else _ffi.NULL,
            _lib.pyclingo_observer_rule if _overwritten(Observer, observer, "rule") else _ffi.NULL,
            _lib.pyclingo_observer_weight_rule if _overwritten(Observer, observer, "weight_rule") else _ffi.NULL,
            _lib.pyclingo_observer_minimize if _overwritten(Observer, observer, "minimize") else _ffi.NULL,
            _lib.pyclingo_observer_project if _overwritten(Observer, observer, "project") else _ffi.NULL,
            _lib.pyclingo_observer_output_atom if _overwritten(Observer, observer, "output_atom") else _ffi.NULL,
            _lib.pyclingo_observer_output_term if _overwritten(Observer, observer, "output_term") else _ffi.NULL,
            _lib.pyclingo_observer_output_csp if _overwritten(Observer, observer, "output_csp") else _ffi.NULL,
            _lib.pyclingo_observer_external if _overwritten(Observer, observer, "external") else _ffi.NULL,
            _lib.pyclingo_observer_assume if _overwritten(Observer, observer, "assume") else _ffi.NULL,
            _lib.pyclingo_observer_heuristic if _overwritten(Observer, observer, "heuristic") else _ffi.NULL,
            _lib.pyclingo_observer_acyc_edge if _overwritten(Observer, observer, "acyc_edge") else _ffi.NULL,
            _lib.pyclingo_observer_theory_term_number if _overwritten(Observer, observer, "theory_term_number") else _ffi.NULL,
            _lib.pyclingo_observer_theory_term_string if _overwritten(Observer, observer, "theory_term_string") else _ffi.NULL,
            _lib.pyclingo_observer_theory_term_compound if _overwritten(Observer, observer, "theory_term_compound") else _ffi.NULL,
            _lib.pyclingo_observer_theory_element if _overwritten(Observer, observer, "theory_element") else _ffi.NULL,
            _lib.pyclingo_observer_theory_atom if _overwritten(Observer, observer, "theory_atom") else _ffi.NULL,
            _lib.pyclingo_observer_theory_atom_with_guard if _overwritten(Observer, observer, "theory_atom_with_guard") else _ffi.NULL))
        c_data = _ffi.new_handle(_CBData(observer, self._error))
        self._mem.append(c_data)
        _handle_error(_lib.clingo_control_register_observer(self._rep, c_observer, replace, c_data))

    def register_propagator(self, propagator: Propagator) -> None:
        '''
        Registers the given propagator with all solvers.

        Parameters
        ----------
        propagator : Propagator
            The propagator to register.

        Returns
        -------
        None

        Notes
        -----
        Each symbolic or theory atom is uniquely associated with a positive program
        atom in form of a positive integer. Program literals additionally have a sign
        to represent default negation. Furthermore, there are non-zero integer solver
        literals. There is a surjective mapping from program atoms to solver literals.

        All methods called during propagation use solver literals whereas
        `SymbolicAtom.literal` and `TheoryAtom.literal` return program literals. The
        function `PropagateInit.solver_literal` can be used to map program literals or
        condition ids to solver literals.

        Not all functions of the `Propagator` interface have to be implemented and can
        be omitted if not needed.
        '''
        # pylint: disable=protected-access
        c_propagator = _ffi.new('clingo_propagator_t*', (
            _lib.pyclingo_propagator_init if _overwritten(Propagator, propagator, "init") else _ffi.NULL,
            _lib.pyclingo_propagator_propagate if _overwritten(Propagator, propagator, "propagate") else _ffi.NULL,
            _lib.pyclingo_propagator_undo if _overwritten(Propagator, propagator, "undo") else _ffi.NULL,
            _lib.pyclingo_propagator_check if _overwritten(Propagator, propagator, "check") else _ffi.NULL,
            _lib.pyclingo_propagator_decide if _overwritten(Propagator, propagator, "decide") else _ffi.NULL))
        c_data = _ffi.new_handle(_CBData(propagator, self._error))
        self._mem.append(c_data)
        _handle_error(_lib.clingo_control_register_propagator(self._rep, c_propagator, c_data, False))

    def release_external(self, external: Union[Symbol,int]) -> None:
        '''
        Release an external atom represented by the given symbol or program literal.

        This function causes the corresponding atom to become permanently false if
        there is no definition for the atom in the program. Otherwise, the function has
        no effect.

        Parameters
        ----------
        external : Union[Symbol,int]
            The symbolic atom or program atom to release.

        Returns
        -------
        None

        Notes
        -----
        If the program literal is negative, the corresponding atom is released.

        Examples
        --------
        The following example shows the effect of assigning and releasing and external
        atom.

            >>> import clingo
            >>> ctl = clingo.Control()
            >>> ctl.add("base", [], "a. #external b.")
            >>> ctl.ground([("base", [])])
            >>> ctl.assign_external(clingo.Function("b"), True)
            >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
            Answer: b a
            SAT
            >>> ctl.release_external(clingo.Function("b"))
            >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
            Answer: a
            SAT
        '''
        _handle_error(_lib.clingo_control_release_external(self._rep, self._program_atom(external)))

    def solve(self,
              assumptions: Sequence[Union[Tuple[Symbol,bool],int]]=[],
              on_model: Callable[[Model],Optional[bool]]=None,
              on_statistics : Callable[[StatisticsMap,StatisticsMap],None]=None,
              on_finish: Callable[[SolveResult],None]=None,
              on_core: Callable[[Sequence[int]],None]=None,
              yield_: bool=False,
              async_: bool=False) -> Union[SolveHandle,SolveResult]:
        '''
        Starts a search.

        Parameters
        ----------
        assumptions : Sequence[Union[Tuple[Symbol,bool],int]]=[]
            List of (atom, boolean) tuples or program literals that serve
            as assumptions for the solve call, e.g., solving under
            assumptions `[(Function("a"), True)]` only admits answer sets
            that contain atom `a`.
        on_model : Callable[[Model],Optional[bool]]=None
            Optional callback for intercepting models.
            A `clingo.solving.Model` object is passed to the callback. The
            search can be interruped from the model callback by returning
            False.
        on_statistics : Callable[[StatisticsMap,StatisticsMap],None]=None
            Optional callback to update statistics.
            The step and accumulated statistics are passed as arguments.
        on_finish : Callable[[SolveResult],None]=None
            Optional callback called once search has finished.
            A `clingo.solving.SolveResult` also indicating whether the solve
            call has been intrrupted is passed to the callback.
        on_core : Callable[[Sequence[int]],None]=None
            Optional callback called with the assumptions that made a problem
            unsatisfiable.
        yield_ : bool=False
            The resulting `clingo.solving.SolveHandle` is iterable yielding
            `clingo.solving.Model` objects.
        async_ : bool=False
            The solve call and the method `clingo.solving.SolveHandle.resume`
            of the returned handle are non-blocking.

        Returns
        -------
        Union[SolveHandle,SolveResult]
            The return value depends on the parameters. If either `yield_` or `async_`
            is true, then a handle is returned. Otherwise, a
            `clingo.solving.SolveResult` is returned.

        Notes
        -----
        If neither `yield_` nor `async_` is set, the function returns a
        `clingo.solving.SolveResult` right away.

        Note that in gringo or in clingo with lparse or text output enabled
        this function just grounds and returns a `clingo.solving.SolveResult`
        where `clingo.solving.SolveResult.unknown` is true.

        If this function is used in embedded Python code, you might want to start
        clingo using the `--outf=3` option to disable all output from clingo.

        Note that asynchronous solving is only available in clingo with thread support
        enabled. Furthermore, the on_model and on_finish callbacks are called from
        another thread. To ensure that the methods can be called, make sure to not use
        any functions that block Python's GIL indefinitely.

        This function as well as blocking functions on the
        `clingo.solving.SolveHandle` release the GIL but are not thread-safe.

        Examples
        --------

        The following example shows how to intercept models with a callback:

            >>> import clingo
            >>> ctl = clingo.Control("0")
            >>> ctl.add("p", [], "1 { a; b } 1.")
            >>> ctl.ground([("p", [])])
            >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
            Answer: a
            Answer: b
            SAT

        The following example shows how to yield models:

            >>> import clingo
            >>> ctl = clingo.Control("0")
            >>> ctl.add("p", [], "1 { a; b } 1.")
            >>> ctl.ground([("p", [])])
            >>> with ctl.solve(yield_=True) as handle:
            ...     for m in handle: print("Answer: {}".format(m))
            ...     handle.get()
            ...
            Answer: a
            Answer: b
            SAT

        The following example shows how to solve asynchronously:

            >>> import clingo
            >>> ctl = clingo.Control("0")
            >>> ctl.add("p", [], "1 { a; b } 1.")
            >>> ctl.ground([("p", [])])
            >>> with ctl.solve(on_model=lambda m: print("Answer: {}".format(m)), async_=True) as handle:
            ...     while not handle.wait(0): pass
            ...     handle.get()
            ...
            Answer: a
            Answer: b
            SAT
        '''
        # pylint: disable=protected-access,dangerous-default-value
        self._error.clear()
        handler = _SolveEventHandler(on_model, on_statistics, on_finish)
        data = _CBData(handler, self._error)
        self._handler = _ffi.new_handle(data)

        p_ass = _ffi.NULL
        if assumptions:
            atoms = None
            p_ass = _ffi.new('clingo_literal_t[]', len(assumptions))
            for i, lit in enumerate(assumptions):
                if isinstance(lit, int):
                    p_ass[i] = lit
                else:
                    if atoms is None:
                        atoms = self.symbolic_atoms
                    atom = self.symbolic_atoms[lit[0]]
                    slit = -1 if atom is None else atom.literal
                    p_ass[i] = slit if lit[1] else -slit

        mode = 0
        if yield_:
            mode |= _lib.clingo_solve_mode_yield
        if async_:
            mode |= _lib.clingo_solve_mode_async

        handle = SolveHandle(
            _c_call('clingo_solve_handle_t*', _lib.clingo_control_solve,
                self._rep, mode,
                p_ass, len(assumptions),
                _lib.pyclingo_solve_event_callback, self._handler,
                handler=data),
            handler)

        if not yield_ and not async_:
            with handle:
                ret = handle.get()
                if on_core is not None and ret.unsatisfiable:
                    on_core(handle.core())
                return ret
        return handle

    @property
    def configuration(self) -> Configuration:
        '''
        Object to change the configuration.
        '''
        conf = _c_call('clingo_configuration_t*', _lib.clingo_control_configuration, self._rep)
        key = _c_call('clingo_id_t', _lib.clingo_configuration_root, conf)
        return Configuration(conf, key)

    @property
    def enable_cleanup(self) -> bool:
        '''
        Whether to enable automatic calls to `Control.cleanup`.
        '''
        return _lib.clingo_control_get_enable_cleanup(self._rep)

    @enable_cleanup.setter
    def enable_cleanup(self, value: bool) -> None:
        _handle_error(_lib.clingo_control_set_enable_cleanup(self._rep, value))

    @property
    def enable_enumeration_assumption(self) -> bool:
        '''
        Whether do discard or keep learnt information from enumeration modes.

        If the enumeration assumption is enabled, then all information learnt from
        clasp's various enumeration modes is removed after a solve call. This includes
        enumeration of cautious or brave consequences, enumeration of answer sets with
        or without projection, or finding optimal models; as well as clauses added with
        `clingo.solving.SolveControl.add_clause`.

        Notes
        -----
        Initially the enumeration assumption is enabled.

        In general, the enumeration assumption should be enabled whenever there are
        multiple calls to solve. Otherwise, the behavior of the solver will be
        unpredictable because there are no guarantees which information exactly is
        kept. There might be small speed benefits when disabling the enumeration
        assumption for single shot solving.
        '''
        return _lib.clingo_control_get_enable_enumeration_assumption(self._rep)

    @enable_enumeration_assumption.setter
    def enable_enumeration_assumption(self, value: bool) -> None:
        _handle_error(_lib.clingo_control_set_enable_enumeration_assumption(self._rep, value))

    @property
    def is_conflicting(self) -> bool:
        '''
        Whether the internal program representation is conflicting.

        If this (read-only) property is true, solve calls return immediately with an
        unsatisfiable solve result.

        Notes
        -----
        Conflicts first have to be detected, e.g., initial unit propagation results in
        an empty clause, or later if an empty clause is resolved during solving. Hence,
        the property might be false even if the problem is unsatisfiable.
        '''
        return _lib.clingo_control_is_conflicting(self._rep)

    @property
    def statistics(self) -> dict:
        '''
        A `dict` containing solve statistics of the last solve call.

        Notes
        -----
        The statistics correspond to the `--stats` output of clingo. The detail of the
        statistics depends on what level is requested on the command line. Furthermore,
        there are some functions like `Control.release_external` that start a new
        solving step resetting the current step statistics. It is best to access the
        statistics right after solving.

        This property is only available in clingo.

        Examples
        --------
        The following example shows how to dump the solving statistics in json format:

            >>> import json
            >>> import clingo
            >>> ctl = clingo.Control()
            >>> ctl.add("base", [], "{a}.")
            >>> ctl.ground([("base", [])])
            >>> ctl.solve()
            SAT
            >>> print(json.dumps(ctl.statistics['solving'], sort_keys=True, indent=4,
            ... separators=(',', ': ')))
            {
                "solvers": {
                    "choices": 1.0,
                    "conflicts": 0.0,
                    "conflicts_analyzed": 0.0,
                    "restarts": 0.0,
                    "restarts_last": 0.0
                }
            }

        '''
        stats = _c_call('clingo_statistics_t*', _lib.clingo_control_statistics, self._rep)

        p_key = _ffi.new('uint64_t*')
        key_root = _c_call(p_key, _lib.clingo_statistics_root, stats)

        key_summary = _c_call(p_key, _lib.clingo_statistics_map_at, stats, key_root, "summary".encode())
        key_call = _c_call(p_key, _lib.clingo_statistics_map_at, stats, key_summary, "call".encode())
        call = _c_call('double', _lib.clingo_statistics_value_get, stats, key_call)
        if self._statistics is not None and call != self._statistics_call:
            self._statistics = None

        if self._statistics is None:
            self._statistics_call = call
            self._statistics = _statistics(stats, key_root)

        return cast(dict, self._statistics)

    @property
    def symbolic_atoms(self) -> SymbolicAtoms:
        '''
        `SymbolicAtoms` object to inspect the symbolic atoms.
        '''
        return SymbolicAtoms(_c_call('clingo_symbolic_atoms_t*', _lib.clingo_control_symbolic_atoms, self._rep))

    @property
    def theory_atoms(self) -> Iterator[TheoryAtom]:
        '''
        An iterator over the theory atoms.
        '''
        atoms = _c_call('clingo_theory_atoms_t*', _lib.clingo_control_theory_atoms, self._rep)
        size = _c_call('size_t', _lib.clingo_theory_atoms_size, atoms)

        for idx in range(size):
            yield TheoryAtom(atoms, idx)
