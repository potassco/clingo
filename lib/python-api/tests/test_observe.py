"""
Unit tests for clingo.backend module.
"""

from typing import Sequence

from clingo.backend import Observer
from clingo.control import Control
from clingo.core import Library


class ExampleObserver(Observer):
    def __init__(self) -> None:
        super().__init__()
        self.rules = []
        self.weight_rules = []
        self.projections = []

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
        """
        Test observe.
        """
        self.ctl.parse_string("#program a. {a}. {c}. b :- a, c.")
        self.ctl.ground([("a", [])])
        obs = ExampleObserver()
        self.ctl.observe(obs)

        assert len(obs.rules) == 2
        assert sum(1 for _, _, choice in obs.rules if choice) == 1
        assert sum(1 for _, _, choice in obs.rules if not choice) == 1

        self.ctl.parse_string("#program b. {p(1..20)}. :- #sum { X: p(X) } >= 40.")
        self.ctl.ground([("b", [])])
        self.ctl.observe(obs)
        assert len(obs.weight_rules) == 1
        assert len(obs.weight_rules[0][0]) == 0
        assert obs.weight_rules[0][1] == 40
        assert len(obs.weight_rules[0][2]) == 20
        assert not obs.weight_rules[0][3]

        self.ctl.parse_string("#program c. #project p/1.")
        self.ctl.ground([("c", [])])
        self.ctl.observe(obs)
        assert len(obs.projections) == 1
        assert len(obs.projections[0]) == 20
