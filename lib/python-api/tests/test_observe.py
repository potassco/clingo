"""
Unit tests for clingo.backend module.
"""

from typing import Sequence

from clingo.backend import ExternalType, HeuristicType, Observer
from clingo.base import Base
from clingo.control import Control
from clingo.core import Library
from clingo.symbol import Function, Symbol


class ExampleObserver(Observer):
    # pylint: disable=missing-function-docstring,redefined-builtin,too-many-instance-attributes
    """
    Example observer for testing that tracks function calls.
    """

    assumptions: list[Sequence[int]]
    begin_steps: int
    edges: list[tuple[int, int, Sequence[int]]]
    externals: list[tuple[int, ExternalType]]
    heuristics: list[tuple[int, HeuristicType, int, int, Sequence[int]]]
    incremental: bool | None
    minimizes: list[tuple[Sequence[tuple[int, int]], int]]
    projections: list[Sequence[int]]
    rules: list[tuple[Sequence[int], Sequence[int], bool]]
    weight_rules: list[tuple[Sequence[int], int, Sequence[tuple[int, int]], bool]]
    symbols: list[str]

    def __init__(self) -> None:
        super().__init__()
        self.assumptions = []
        self.begin_steps = 0
        self.edges = []
        self.end_steps = 0
        self.externals = []
        self.heuristics = []
        self.incremental = None
        self.minimizes = []
        self.projections = []
        self.rules = []
        self.weight_rules = []
        self.symbols = []

    def init_program(self, incremental: bool) -> None:
        if self.incremental is not None:
            raise RuntimeError("multiple calls to init_program")
        self.incremental = incremental

    def begin_step(self) -> None:
        self.begin_steps += 1

    def end_step(self, base: Base) -> None:
        symbols: list[Symbol] = []
        for atoms in base.values():
            symbols.extend(
                atom.symbol for atom in atoms.values() if base.is_current(atom.literal)
            )
        symbols.sort()
        self.symbols = [str(sym) for sym in symbols]

        assert base is not None
        self.end_steps += 1

    def assume(self, literals: Sequence[int]) -> None:
        self.assumptions.append(literals)

    def rule(self, head: Sequence[int], body: Sequence[int], choice: bool) -> None:
        self.rules.append((head, body, choice))

    def weight_rule(
        self,
        head: Sequence[int],
        lower_bound: int,
        body: Sequence[tuple[int, int]],
        choice: bool,
    ) -> None:
        self.weight_rules.append((head, lower_bound, body, choice))

    def project(self, atoms: Sequence[int]) -> None:
        self.projections.append(atoms)

    def heuristic(
        self,
        atom: int,
        type: HeuristicType,
        weight: int,
        priority: int,
        condition: Sequence[int],
    ) -> None:
        self.heuristics.append((atom, type, weight, priority, condition))

    def external(self, atom: int, type: ExternalType) -> None:
        self.externals.append((atom, type))

    def edge(self, node_u: int, node_v: int, condition: Sequence[int]) -> None:
        self.edges.append((node_u, node_v, condition))

    def minimize(self, literals: Sequence[tuple[int, int]], priority: int) -> None:
        self.minimizes.append((literals, priority))


class TestObserve:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the backend module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()
        self._ctl = Control(self._lib, ["0"])

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._ctl = None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    @property
    def ctl(self) -> Control:
        """
        Get the control object.
        """
        assert self._ctl is not None
        return self._ctl

    def test_observe(self):
        # pylint: disable=too-many-statements
        """
        Test observe.
        """
        self.ctl.parse_string("#program a. {a}. {c}. b :- a, c.")
        self.ctl.ground([("a", [])])
        obs = ExampleObserver()
        self.ctl.observe(obs)
        assert obs.incremental
        assert obs.begin_steps == obs.end_steps == 1
        assert len(obs.rules) == 2
        assert sum(1 for _, _, choice in obs.rules if choice) == 1
        assert sum(1 for _, _, choice in obs.rules if not choice) == 1
        assert obs.symbols == ["a", "b", "c"]

        self.ctl.parse_string("#program b. {p(1..20)}. :- #sum { X: p(X) } >= 40.")
        self.ctl.ground([("b", [])])
        self.ctl.observe(obs)
        assert obs.begin_steps == obs.end_steps == 2
        assert len(obs.weight_rules) == 1
        assert len(obs.weight_rules[0][0]) == 0
        assert obs.weight_rules[0][1] == 40
        assert len(obs.weight_rules[0][2]) == 20
        assert not obs.weight_rules[0][3]
        assert len(obs.symbols) == 20

        self.ctl.parse_string("#program c. #project p/1.")
        self.ctl.ground([("c", [])])
        self.ctl.observe(obs)
        assert obs.begin_steps == obs.end_steps == 3
        assert len(obs.projections) == 1
        assert len(obs.projections[0]) == 20
        assert len(obs.symbols) == 0

        with self.ctl.backend as bck:
            a = bck.atom(Function(self.lib, "a"))
            b = bck.atom(Function(self.lib, "b"))
            c = bck.atom(Function(self.lib, "c"))
            bck.assume([a, b, c])
        self.ctl.observe(obs)
        assert obs.begin_steps == obs.end_steps == 4
        assert len(obs.assumptions) == 1
        assert len(obs.assumptions[0]) == 3
        assert len(obs.symbols) == 0

        self.ctl.parse_string("#program d. #external a. [true] #external d. [true]")
        self.ctl.ground([("d", [])])
        self.ctl.observe(obs)
        assert obs.begin_steps == obs.end_steps == 5
        assert len(obs.externals) == 1
        assert obs.externals[0][1] == ExternalType.True_
        assert len(obs.symbols) == 1

        self.ctl.parse_string("#program e. #heuristic a. [1@2,sign]")
        self.ctl.ground([("e", [])])
        self.ctl.observe(obs)
        assert obs.begin_steps == obs.end_steps == 6
        assert len(obs.heuristics) == 1
        assert obs.heuristics[0][1] == HeuristicType.Sign
        assert obs.heuristics[0][2] == 1
        assert obs.heuristics[0][3] == 2
        assert len(obs.heuristics[0][4]) == 0
        assert len(obs.symbols) == 0

        self.ctl.parse_string("#program f. #edge (1, 2) : a, c.")
        self.ctl.ground([("f", [])])
        self.ctl.observe(obs)
        assert len(obs.edges) == 1
        assert obs.edges[0][0] != obs.edges[0][1]
        assert len(obs.edges[0][2]) == 2
        assert len(obs.symbols) == 0

        self.ctl.parse_string("#program g. #minimize{ 1@2: a; 2@3: b }.")
        self.ctl.ground([("g", [])])
        self.ctl.observe(obs)
        assert len(obs.minimizes) == 2
        m2, m3 = sorted(obs.minimizes, key=lambda x: x[1])
        assert m2[1] == 2
        assert m3[1] == 3
        assert len(obs.symbols) == 0

    def test_observe_preprocessing(self):
        """
        Test program observation with preprocessing enabled.
        """
        self.ctl.parse_string("{a;c}. b :- a. :- a.")
        self.ctl.ground()

        obs = ExampleObserver()
        self.ctl.observe(obs, preprocess=False)
        assert obs.incremental
        assert obs.begin_steps == obs.end_steps == 1
        assert obs.symbols == ["a", "b", "c"]
        assert len(obs.rules) == 3

        obs = ExampleObserver()
        self.ctl.observe(obs, preprocess=True)
        assert obs.incremental
        assert obs.begin_steps == obs.end_steps == 1
        assert obs.symbols == ["a", "b", "c"]
        assert len(obs.rules) == 1
        assert obs.rules[0][0] == [2]
        assert obs.rules[0][1] == []
        assert obs.rules[0][2]

        self.ctl.solve()
        obs = ExampleObserver()
        self.ctl.observe(obs)
        assert obs.begin_steps == obs.end_steps == 1
        assert obs.symbols == ["c"]
        assert len(obs.rules) == 0
