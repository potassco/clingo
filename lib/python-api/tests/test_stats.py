"""
Unit tests for the clingo.stats module.
"""

import pytest
from clingo.control import Control
from clingo.core import Library
from clingo.stats import (
    Stats,
    StatsType,
    StatsMap,
    StatsMapView,
    StatsArray,
    StatsArrayView,
)
from util import MCB


class TestStats:
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

    def teardown_method(self, method):
        """
        Destroy lib.
        """
        assert method is not None
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    def test_solve(self):
        """
        Test the solver stats.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()
        mcb = MCB()
        with ctl.start_solve(on_model=mcb, async_=True, yield_=True) as hnd:
            with pytest.raises(ValueError):
                _ = ctl.stats
            for _ in hnd:
                pass
            assert hnd.get().satisfiable
        assert mcb.symbols == res
        stats = ctl.stats
        cpu = stats["summary"]["times"]["cpu"].value
        print(f"Choices: {stats["solving"]["solvers"]["choices"].value}")
        assert isinstance(cpu, float) and cpu >= 0.0

    def test_user(self):
        """
        Test the user stats.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()
        mcb = MCB()

        def on_stats(step: Stats, accu: Stats):
            assert step.type == StatsType.Map
            assert isinstance(step.map, StatsMap)
            step.update({"a": 10.0})
            step.update({"b": [10.0]})
            step.update({"c": {"x": 1.0}})
            accu.update({"Test": {"x": 10.0, "y": [1.0, 2.0, 3.0]}})
            accu.update({"Test": {"x": lambda x: x + 2}})
            accu.update({"Test": {"y": lambda x: [y + 1 for y in x]}})
            assert isinstance(accu["Test"]["y"].array, StatsArray)

        assert ctl.solve(on_model=mcb, on_stats=on_stats).satisfiable
        assert mcb.symbols == res

        stats = ctl.stats
        assert stats.type == StatsType.Map
        assert isinstance(stats.map, StatsMapView)
        print(f"Times: {ctl.stats['summary']['times']}")
        if (
            "cpu" in ctl.stats["summary"]["times"]
            and "cpu" in ctl.stats["summary"]["times"].map
        ):
            print("FOUND cpu")
        for k in ctl.stats["summary"]["times"].map:
            print(f"key: {k}")

        for v in ctl.stats["summary"]["times"].map.values():
            print(f"val: {v}")
        for k, v in ctl.stats["summary"]["times"].map.items():
            print(f"{k}:{v}")

        print(f"str: {ctl.stats["user_accu"]["Test"]["y"]}")

        for i in ctl.stats["user_accu"]["Test"]["y"]:
            print(f"item: {i}/{i.type}")

        # TBD: Should we make these strings available in the Python-API?
        assert stats["user_step"] == {"a": 10.0, "b": [10.0], "c": {"x": 1.0}}
        assert stats["user_accu"] == {"Test": {"x": 12.0, "y": [2.0, 3.0, 4.0]}}
