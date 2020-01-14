from typing import *

from clingo_abc import Application, Observer, Propagator


Infimum: Symbol
Supremum: Symbol

__version__: str

def Function(name: str, arguments: Iterable[Symbol] = (), positive: bool = True) -> Symbol: ...
def Number(number: int) -> Symbol: ...
def String(string: str) -> Symbol: ...
def Tuple(arguments: Iterable[Symbol]) -> Symbol: ...
def clingo_main(application: Application, files: Iterable[str] = ()) -> int: ...
def parse_program(program: str, callback: Callable[[AST], None]) -> None: ...
def parse_term(string: str, logger: Callable[[MessageCode, str], None] = None, message_limit: int = 20) -> Symbol: ...

class ApplicationOptions:
    def add_flag(self, group: str, option: str, description: str, parser: Callable[[str], bool], multi: bool = False, argument: str = None) -> None: ...
    def add_flag(self, group: str, option: str, description: str, target: Flag) -> None: ...

class Assignment:
    decision_level: int
    has_conflict: bool
    is_total: bool
    max_size: int
    root_level: int
    size: int

    def decision(self, level: int) -> int: ...
    def has_literal(self, literal: int) -> bool: ...
    def is_false(self, literal: int) -> bool: ...
    def is_fixed(self, literal: int) -> bool: ...
    def is_true(self, literal: int) -> bool: ...
    def level(self, literal: int) -> int: ...
    def value(self, literal) -> Optional[bool]: ...

class Backend:
    def add_acyc_edge(self, node_u: int, node_v: int, condition: Iterable[int]) -> None: ...
    def add_assume(self, literals: Iterable[int]) -> None: ...
    def add_atom(self, symbol: Optional[Symbol] = None) -> int: ...
    def add_external(self, atom: int, value: TruthValue = TruthValue.False_) -> None: ...
    def add_heuristic(self, atom: int, type: HeuristicType, bias: int, priority: int, condition: Iterable[int]) -> None: ...
    def add_minimize(self, priority: int, literals: Iterable[Tuple[int, int]]) -> None: ...
    def add_project(self, atoms: Iterable[int]) -> None: ...
    def add_rule(self, head: Iterable[int], body: Iterable[int] = (), choice: bool = False) -> None: ...
    def add_weight_rule(self, head: Iterable[int], lower: int, body: Iterable[Tuple[int, int]], choice: bool = False) -> None: ...

class Configuration:
    keys: Optional[Iterable[str]]

class Control:
    def __init__(self, arguments: Iterable[str] = (), logger: Callable[[MessageCode, str], None] = None, message_limit: int = 20): ...

    configuration: Configuration
    is_conflicting: bool
    statistics: dict
    symbolic_atoms: SymbolicAtoms
    theory_atoms: TheoryAtomIter
    use_enumeration_assumption: bool

    def add(self, name: str, parameters: Iterable[str], program: str) -> None: ...
    def assign_external(self, external: Union[Symbol, int], truth: Optional[bool]) -> None: ...
    def backend(self) -> Backend: ...
    def builder(self) -> ProgramBuilder: ...
    def cleanup(self) -> None: ...
    def get_const(self, name: str) -> Optional[Symbol]: ...
    def ground(self, parts: Iterable[Tuple[str, List[Symbol]]], context: Any = None) -> None: ...
    def interrupt(self) -> None: ...
    def load(self, path: str) -> None: ...
    def register_observer(self, observer: Observer, replace: bool = False) -> None: ...
    def register_propagator(self, propagator: Propagator) -> None: ...
    def release_external(self, symbol: Union[Symbol, int]) -> None: ...
    def solve(self, assumptions: Iterable[Union[Tuple[Symbol, bool], int]] = (), on_model: Callable[[Model], Optional[bool]] = None, on_statistics: Callable[[StatisticsMap, StatisticsMap], None] = None, on_finish: Callable[[SolveResult], None] = None, yield_: bool = False, async_: bool = False) -> Union[SolveHandle, SolveResult]: ...

class Flag:
    def __init__(self, value: bool = False):
        self.value = value

class HeuristicType:
    Level: HeuristicType
    Sign: HeuristicType
    Factor: HeuristicType
    Init: HeuristicType
    True_: HeuristicType
    False_: HeuristicType

class MessageCode:
    OperationUndefined: MessageCode
    RuntimeError: MessageCode
    AtomUndefined: MessageCode
    FileIncluded: MessageCode
    VariableUnbounded: MessageCode
    GlobalVariable: MessageCode
    Other: MessageCode

class Model:
    context: SolveControl
    cost: Iterable[int]
    number: int
    optimality_proven: bool
    thread_id: int
    type: ModelType

    def contains(self, atom: Symbol) -> bool: ...
    def extend(self, symbols: Iterable[Symbol]) -> None: ...
    def is_true(self, literal: int) -> bool: ...
    def symbols(self, atoms: bool = False, terms: bool = False, shown: bool = False, csp: bool = False, complement: bool = False) -> Iterable[Symbol]: ...

class ModelType:
    BraveConsequences: ModelType
    CautiousConsequences: ModelType
    StableModel: ModelType

class ProgramBuilder:
    def add(self, statement: AST) -> None: ...

class PropagateControl:
    assignment: Assignment
    thread_id: int

    def add_clause(self, clause: Iterable[int], tag: bool = False, lock: bool = False) -> bool: ...
    def add_literal(self) -> int: ...
    def add_nogood(self, clause: Iterable[int], tag: bool = False, lock: bool = False) -> bool: ...
    def add_watch(self, literal: int) -> None: ...
    def has_watch(self, literal: int) -> bool: ...
    def propagate(self) -> bool: ...
    def remove_watch(self, literal: int) -> None: ...

class PropagateInit:
    assignment: Assignment
    check_mode: PropagatorCheckMode
    number_of_threads: int
    symbolic_atoms: SymbolicAtoms
    theory_atoms: TheoryAtomIter

    def add_clause(self, clause: Iterable[int]) -> None: ...
    def add_watch(self, literal: int, thread_id: Optional[int] = None) -> None: ...
    def solver_literal(self, literal: int) -> int: ...

class PropagatorCheckMode:
    Off: PropagatorCheckMode
    Total: PropagatorCheckMode
    Fixpoint: PropagatorCheckMode

class SolveControl:
    symbolic_atoms: SymbolicAtoms

    def add_clause(self, literals: Iterable[Union[Tuple[Symbol, bool], int]]) -> None: ...
    def add_nogood(self, literals: Iterable[Union[Tuple[Symbol, bool], int]]) -> None: ...

class SolveHandle:
    def cancel(self) -> None: ...
    def get(self) -> SolveResult: ...
    def resume(self) -> None: ...
    def wait(self, timeout: Optional[float] = None) -> Optional[bool]: ...

class SolveResult:
    exhausted: bool
    interrupted: bool
    satisfiable: Optional[bool]
    unknown: bool
    unsatisfiable: Optional[bool]

class StatisticsArray:
    def append(self, value: Any) -> None: ...
    def extend(self, values: Sequence[Any]) -> None: ...
    def update(self, values: Sequence[Any]) -> None: ...

class StatisticsMap:
    def items(self) -> Iterable[Tuple[str, Union[StatisticsArray, StatisticsMap, float]]]: ...
    def keys(self) -> Iterable[str]: ...
    def update(self, values: Mappping[str, Any]) -> None: ...
    def values(self) -> Iterable[Union[StatisticsArray, StatisticsMap, float]]: ...


class Symbol:
    arguments: Iterable[Symbol]
    name: str
    negative: bool
    number: int
    positive: bool
    string: str
    type: SymbolType

    def match(self, name: str, arity: int) -> bool: ...

class SymbolType:
    Number: SymbolType
    String: SymbolType
    Function: SymbolType
    Infimum: SymbolType
    Supremum: SymbolType

class SymbolicAtom:
    is_external: bool
    is_fact: bool
    literal: int
    symbol: Symbol

    def match(self, name: str, arity: int) -> bool: ...

class SymbolicAtomIter(Iterator[SymbolicAtom]):
    pass

class SymbolicAtoms:
    signatures: Iterable[Tuple[str, int, bool]]

    def by_signature(self, name: str, arity: int, positive: bool = True) -> Iterator[Symbol]: ...

class TheoryAtom:
    elements: Iterable[TheoryElement]
    guard: Tuple[str, TheoryTerm]
    literal: int
    term: TheoryTerm

class TheoryAtomIter(Iterator[SymbolicAtom]):
    pass

class TheoryElement:
    condition: Iterable[TheoryTerm]
    condition_id: int
    terms: Iterable[TheoryTerm]

class TheoryTerm:
    arguments: Iterable[Symbol]
    name: str
    number: int
    type: TheoryTermType

class TheoryTermType:
    Function: TheoryTermType
    Number: TheoryTermType
    Symbol: TheoryTermType
    List: TheoryTermType
    Tuple: TheoryTermType
    Set: TheoryTermType

class TruthValue:
    True_: TruthValue
    False_: TruthValue
    Free: TruthValue
    Release: TruthValue
