"""
Unit tests for clingo.control.Control.profile.
"""

from clingo.control import Control
from clingo.core import Library


class TestProfile:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the stats module.
    """

    def setup_method(self, method):
        """
        Create lib.
        """
        assert method is not None
        self._lib = Library()
        self._ctl = Control(self._lib, ["0", "--profile"])

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

    def test_profile(self):
        """
        Test profiling.
        """
        self.ctl.parse_string(
            """
            #const n = 10.
            { q(1..n,Y) } = 1 :- Y=1..n.
            { q(X,1..n) } = 1 :- X=1..n.
            :- q(X,Y), q(X',Y'), (X,Y)<(X',Y'), X'-X == |Y-Y'|.
            """
        )
        self.ctl.ground()

        profile = self.ctl.profile
        assert len(profile) == 3

        # one of the generators
        assert profile[0]["type"] == "internal"
        assert profile[0]["children"][0]["type"] == "internal"

        ## the aggregate
        assert profile[0]["children"][0]["children"][0]["type"] == "internal"
        leaf = profile[0]["children"][0]["children"][0]["children"][0]
        assert leaf["type"] == "leaf"
        assert leaf["instances"] == 100
        assert leaf["matches"] >= 100

        ## the statement
        leaf = profile[0]["children"][0]["children"][-1]
        assert leaf["type"] == "leaf"
        assert leaf["instances"] == 10
        assert leaf["matches"] >= 10

        # the integrity constraint
        assert profile[2]["type"] == "internal"
        assert profile[2]["children"][0]["type"] == "leaf"
        assert profile[2]["children"][0]["instances"] == 570
        assert profile[2]["children"][0]["matches"] >= 570
        assert profile[2]["children"][0]["time_instantiate"] >= 0
        assert profile[2]["children"][0]["time_propagate"] >= 0
