'''
This module contains functions and classes to inspect or add ground statements.
'''

from typing import ContextManager, Sequence, Optional, Tuple
from enum import Enum
from abc import ABCMeta

from ._internal import _Error, _c_call, _cb_error_handler, _ffi, _handle_error, _lib, _to_str
from .core import TruthValue
from .symbol import Symbol

class HeuristicType(Enum):
    '''
    Enumeration of the different heuristic types.

    Attributes
    ----------
    Level : HeuristicType
        Heuristic modification to set the level of an atom.
    Sign : HeuristicType
        Heuristic modification to set the sign of an atom.
    Factor : HeuristicType
        Heuristic modification to set the decaying factor of an atom.
    Init : HeuristicType
        Heuristic modification to set the inital score of an atom.
    True_ : HeuristicType
        Heuristic modification to make an atom true.
    False_ : HeuristicType
        Heuristic modification to make an atom false.
    '''
    Factor = _lib.clingo_heuristic_type_factor
    False_ = _lib.clingo_heuristic_type_false
    Init = _lib.clingo_heuristic_type_init
    Level = _lib.clingo_heuristic_type_level
    Sign = _lib.clingo_heuristic_type_sign
    True_ = _lib.clingo_heuristic_type_true

class Observer(metaclass=ABCMeta):
    '''
    Interface that has to be implemented to inspect rules produced during
    grounding.
    '''
    def init_program(self, incremental: bool) -> None:
        '''
        Called once in the beginning.

        Parameters
        ----------
        incremental : bool
            Whether the program is incremental. If the incremental flag is
            true, there can be multiple calls to `Control.solve`.

        Returns
        -------
        None
        '''

    def begin_step(self) -> None:
        '''
        Marks the beginning of a block of directives passed to the solver.

        Returns
        -------
        None
        '''

    def rule(self, choice: bool, head: Sequence[int], body: Sequence[int]) -> None:
        '''
        Observe rules passed to the solver.

        Parameters
        ----------
        choice : bool
            Determines if the head is a choice or a disjunction.
        head : Sequence[int]
            List of program atoms forming the rule head.
        body : Sequence[int]
            List of program literals forming the rule body.

        Returns
        -------
        None
        '''

    def weight_rule(self, choice: bool, head: Sequence[int], lower_bound: int,
                    body: Sequence[Tuple[int,int]]) -> None:
        '''
        Observe rules with one weight constraint in the body passed to the
        solver.

        Parameters
        ----------
        choice : bool
            Determines if the head is a choice or a disjunction.
        head : Sequence[int]
            List of program atoms forming the head of the rule.
        lower_bound:
            The lower bound of the weight constraint in the rule body.
        body : Sequence[Tuple[int,int]]
            List of weighted literals (pairs of literal and weight) forming the
            elements of the weight constraint.

        Returns
        -------
        None
        '''

    def minimize(self, priority: int, literals: Sequence[Tuple[int,int]]) -> None:
        '''
        Observe minimize directives (or weak constraints) passed to the
        solver.

        Parameters
        ----------
        priority : int
            The priority of the directive.
        literals : Sequence[Tuple[int,int]]
            List of weighted literals whose sum to minimize (pairs of literal
            and weight).

        Returns
        -------
        None
        '''

    def project(self, atoms: Sequence[int]) -> None:
        '''
        Observe projection directives passed to the solver.

        Parameters
        ----------
        atoms : Sequence[int]
            The program atoms to project on.

        Returns
        -------
        None
        '''

    def output_atom(self, symbol: Symbol, atom: int) -> None:
        '''
        Observe shown atoms passed to the solver.  Facts do not have an
        associated program atom. The value of the atom is set to zero.

        Parameters
        ----------
        symbol : Symbolic
            The symbolic representation of the atom.
        atom : int
            The associated program atom (0 for facts).

        Returns
        -------
        None
        '''

    def output_term(self, symbol: Symbol, condition: Sequence[int]) -> None:
        '''
        Observe shown terms passed to the solver.

        Parameters
        ----------
        symbol : Symbol
            The symbolic representation of the term.
        condition : Sequence[int]
            List of program literals forming the condition when to show the
            term.

        Returns
        -------
        None
        '''

    def output_csp(self, symbol: Symbol, value: int,
                   condition: Sequence[int]) -> None:
        '''
        Observe shown csp variables passed to the solver.

        Parameters
        ----------
        symbol : Symbol
            The symbolic representation of the variable.
        value : int
            The integer value of the variable.
        condition : Sequence[int]
            List of program literals forming the condition when to show the
            variable with its value.

        Returns
        -------
        None
        '''

    def external(self, atom: int, value: TruthValue) -> None:
        '''
        Observe external statements passed to the solver.

        Parameters
        ----------
        atom : int
            The external atom in form of a program literal.
        value : TruthValue
            The truth value of the external statement.

        Returns
        -------
        None
        '''

    def assume(self, literals: Sequence[int]) -> None:
        '''
        Observe assumption directives passed to the solver.

        Parameters
        ----------
        literals : Sequence[int]
            The program literals to assume (positive literals are true and
            negative literals false for the next solve call).

        Returns
        -------
        None
        '''

    def heuristic(self, atom: int, type_: HeuristicType, bias: int,
                  priority: int, condition: Sequence[int]) -> None:
        '''
        Observe heuristic directives passed to the solver.

        Parameters
        ----------
        atom : int
            The program atom heuristically modified.
        type_ : HeuristicType
            The type of the modification.
        bias : int
            A signed integer.
        priority : int
            An unsigned integer.
        condition : Sequence[int]
            List of program literals.

        Returns
        -------
        None
        '''

    def acyc_edge(self, node_u: int, node_v: int,
                  condition: Sequence[int]) -> None:
        '''
        Observe edge directives passed to the solver.

        Parameters
        ----------
        node_u : int
            The start vertex of the edge (in form of an integer).
        node_v : int
            Тhe end vertex of the edge (in form of an integer).
        condition : Sequence[int]
            The list of program literals forming th condition under which to
            add the edge.

        Returns
        -------
        None
        '''

    def theory_term_number(self, term_id: int, number: int) -> None:
        '''
        Observe numeric theory terms.

        Parameters
        ----------
        term_id : int
            The id of the term.
        number : int
            The value of the term.

        Returns
        -------
        None
        '''

    def theory_term_string(self, term_id : int, name : str) -> None:
        '''
        Observe string theory terms.

        Parameters
        ----------
        term_id : int
            The id of the term.
        name : str
            The string value of the term.

        Returns
        -------
        None
        '''

    def theory_term_compound(self, term_id: int, name_id_or_type: int,
                             arguments: Sequence[int]) -> None:
        '''
        Observe compound theory terms.

        Parameters
        ----------
        term_id : int
            The id of the term.
        name_id_or_type : int
            The name or type of the term where
            - if it is -1, then it is a tuple
            - if it is -2, then it is a set
            - if it is -3, then it is a list
            - otherwise, it is a function and name_id_or_type refers to the id
            of the name (in form of a string term)
        arguments : Sequence[int]
            The arguments of the term in form of a list of term ids.

        Returns
        -------
        None
        '''

    def theory_element(self, element_id: int, terms: Sequence[int],
                       condition: Sequence[int]) -> None:
        '''
        Observe theory elements.

        Parameters
        ----------
        element_id : int
            The id of the element.
        terms : Sequence[int]
            The term tuple of the element in form of a list of term ids.
        condition : Sequence[int]
            The list of program literals forming the condition.

        Returns
        -------
        None
        '''

    def theory_atom(self, atom_id_or_zero: int, term_id: int,
                    elements: Sequence[int]) -> None:
        '''
        Observe theory atoms without guard.

        Parameters
        ----------
        atom_id_or_zero : int
            The id of the atom or zero for directives.
        term_id : int
            The term associated with the atom.
        elements : Sequence[int]
            The elements of the atom in form of a list of element ids.

        Returns
        -------
        None
        '''

    def theory_atom_with_guard(self, atom_id_or_zero: int, term_id: int,
                               elements: Sequence[int], operator_id: int,
                               right_hand_side_id: int) -> None:
        '''
        Observe theory atoms with guard.

        Parameters
        ----------
        atom_id_or_zero : int
            The id of the atom or zero for directives.
        term_id : int
            The term associated with the atom.
        elements : Sequence[int]
            The elements of the atom in form of a list of element ids.
        operator_id : int
            The id of the operator (a string term).
        right_hand_side_id : int
            The id of the term on the right hand side of the atom.

        Returns
        -------
        None
        '''

    def end_step(self) -> None:
        '''
        Marks the end of a block of directives passed to the solver.

        This function is called right before solving starts.

        Returns
        -------
        None
        '''

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_init_program(incremental, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.init_program(incremental)
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_begin_step(data):
    observer: Observer = _ffi.from_handle(data).data
    observer.begin_step()
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_end_step(data):
    observer: Observer = _ffi.from_handle(data).data
    observer.end_step()
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_rule(choice, head, head_size, body, body_size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.rule(
        choice,
        [ head[i] for i in range(head_size) ],
        [ body[i] for i in range(body_size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_weight_rule(choice, head, head_size, lower_bound, body, body_size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.weight_rule(
        choice,
        [ head[i] for i in range(head_size) ],
        lower_bound,
        [ (body[i].literal, body[i].weight) for i in range(body_size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_minimize(priority, literals, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.minimize(priority, [ (literals[i].literal, literals[i].weight) for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_project(atoms, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.project([ atoms[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_output_atom(symbol, atom, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.output_atom(Symbol(symbol), atom)
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_output_term(symbol, condition, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.output_term(Symbol(symbol), [ condition[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_output_csp(symbol, value, condition, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.output_csp(Symbol(symbol), value, [ condition[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_external(atom, type_, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.external(atom, TruthValue(type_))
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_assume(literals, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.assume([ literals[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_heuristic(atom, type_, bias, priority, condition, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.heuristic(atom, HeuristicType(type_), bias, priority, [ condition[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_acyc_edge(node_u, node_v, condition, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.acyc_edge(node_u, node_v, [ condition[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_theory_term_number(term_id, number, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.theory_term_number(term_id, number)
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_theory_term_string(term_id, name, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.theory_term_string(term_id, _to_str(name))
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_theory_term_compound(term_id, name_id_or_type, arguments, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.theory_term_compound(term_id, name_id_or_type, [ arguments[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_theory_element(element_id, terms, terms_size, condition, condition_size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.theory_element(
        element_id,
        [ terms[i] for i in range(terms_size) ],
        [ condition[i] for i in range(condition_size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_theory_atom(atom_id_or_zero, term_id, elements, size, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.theory_atom(atom_id_or_zero, term_id, [ elements[i] for i in range(size) ])
    return True

@_ffi.def_extern(onerror=_cb_error_handler('data'))
def pyclingo_observer_theory_atom_with_guard(atom_id_or_zero, term_id, elements, size,
                                            operator_id, right_hand_side_id, data):
    observer: Observer = _ffi.from_handle(data).data
    observer.theory_atom_with_guard(
        atom_id_or_zero, term_id,
        [ elements[i] for i in range(size) ],
        operator_id, right_hand_side_id)
    return True

class Backend(ContextManager['Backend']):
    '''
    Backend object providing a low level interface to extend a logic program.

    This class allows for adding statements in ASPIF format.

    See Also
    --------
    clingo.control.Control.backend

    Notes
    -----
    The `Backend` is a context manager and must be used with Python's `with`
    statement.

    Examples
    --------
    The following example shows how to add a fact to a program:

        >>> import clingo
        >>> ctl = clingo.Control()
        >>> sym_a = clingo.Function("a")
        >>> with ctl.backend() as backend:
        ...     atm_a = backend.add_atom(sym_a)
        ...     backend.add_rule([atm_a])
        ...
        >>> ctl.symbolic_atoms[sym_a].is_fact
        True
        >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
        Answer: a
        SAT
    '''
    def __init__(self, rep, error: _Error):
        self._rep = rep
        self._error = error

    def __enter__(self):
        self._error.clear()
        _handle_error(_lib.clingo_backend_begin(self._rep), handler=self._error)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._error.clear()
        _handle_error(_lib.clingo_backend_end(self._rep), handler=self._error)
        return False

    def add_acyc_edge(self, node_u: int, node_v: int, condition: Sequence[int]) -> None:
        '''
        Add an edge directive to the program.

        Parameters
        ----------
        node_u : int
            The start node represented as an unsigned integer.
        node_v : int
            The end node represented as an unsigned integer.
        condition : Sequence[int]
            List of program literals.

        Returns
        -------
        None
        '''
        _handle_error(_lib.clingo_backend_acyc_edge(self._rep, node_u, node_v, condition, len(condition)))

    def add_assume(self, literals: Sequence[int]) -> None:
        '''
        Add assumptions to the program.

        Parameters
        ----------
        literals : Sequence[int]
            The list of literals to assume true.

        Returns
        -------
        None
        '''
        _handle_error(_lib.clingo_backend_assume(self._rep, literals, len(literals)))

    def add_atom(self, symbol: Optional[Symbol]=None) -> int:
        '''
        Return a fresh program atom or the atom associated with the given symbol.

        If the given symbol does not exist in the atom base, it is added first. Such
        atoms will be used in subequents calls to ground for instantiation.

        Parameters
        ----------
        symbol : Optional[Symbol]=None
            The symbol associated with the atom.

        Returns
        -------
        int
            The program atom representing the atom.
        '''
        # pylint: disable=protected-access
        if symbol is None:
            p_sym = _ffi.NULL
        else:
            p_sym = _ffi.new('clingo_symbol_t*', symbol._rep)

        self._error.clear()
        return _c_call('clingo_atom_t', _lib.clingo_backend_add_atom, self._rep, p_sym, handler=self._error)

    def add_external(self, atom : int, value : TruthValue=TruthValue.False_) -> None:
        '''
        Mark a program atom as external optionally fixing its truth value.

        Parameters
        ----------
        atom : int
            The program atom to mark as external.
        value : TruthValue=TruthValue.False_
            Optional truth value.

        Returns
        -------
        None

        Notes
        -----
        Can also be used to release an external atom using `TruthValue.Release`.
        '''
        _handle_error(_lib.clingo_backend_external(self._rep, atom, value.value))

    def add_heuristic(self, atom: int, type_: HeuristicType, bias: int, priority: int,
                      condition: Sequence[int]) -> None:
        '''
        Add a heuristic directive to the program.

        Parameters
        ----------
        atom : int
            Program atom to heuristically modify.
        type_ : HeuristicType
            The type of modification.
        bias : int
            A signed integer.
        priority : int
            An unsigned integer.
        condition : Sequence[int]
            List of program literals.

        Returns
        -------
        None
        '''
        _handle_error(_lib.clingo_backend_heuristic(self._rep, atom, type_.value, bias, priority,
                      condition, len(condition)))

    def add_minimize(self, priority: int, literals: Sequence[Tuple[int,int]]) -> None:
        '''
        Add a minimize constraint to the program.

        Parameters
        ----------
        priority : int
            Integer for the priority.
        literals : Sequence[Tuple[int,int]]
            List of pairs of program literals and weights.

        Returns
        -------
        None
        '''
        _handle_error(_lib.clingo_backend_minimize(self._rep, priority, literals, len(literals)))

    def add_project(self, atoms: Sequence[int]) -> None:
        '''
        Add a project statement to the program.

        Parameters
        ----------
        atoms : Sequence[int]
            List of program atoms to project on.

        Returns
        -------
        None
        '''
        _handle_error(_lib.clingo_backend_project(self._rep, atoms, len(atoms)))

    def add_rule(self, head: Sequence[int], body: Sequence[int]=[], choice: bool=False) -> None:
        '''
        Add a disjuntive or choice rule to the program.

        Parameters
        ----------
        head : Sequence[int]
            The program atoms forming the rule head.
        body : Sequence[int]=[]
            The program literals forming the rule body.
        choice : bool=False
            Whether to add a disjunctive or choice rule.

        Returns
        -------
        None

        Notes
        -----
        Integrity constraints and normal rules can be added by using an empty or
        singleton head list, respectively.
        '''
        # pylint: disable=dangerous-default-value
        _handle_error(_lib.clingo_backend_rule(self._rep, choice, head, len(head), body, len(body)))

    def add_weight_rule(self, head: Sequence[int], lower: int, body: Sequence[Tuple[int,int]],
                        choice: bool=False) -> None:
        '''
        Add a disjuntive or choice rule with one weight constraint with a lower bound
        in the body to the program.

        Parameters
        ----------
        head : Sequence[int]
            The program atoms forming the rule head.
        lower : int
            The lower bound.
        body : Sequence[Tuple[int,int]]
            The pairs of program literals and weights forming the elements of the
            weight constraint.
        choice : bool=False
            Whether to add a disjunctive or choice rule.

        Returns
        -------
        None
        '''
        _handle_error(_lib.clingo_backend_weight_rule(self._rep, choice, head, len(head), lower, body, len(body)))
