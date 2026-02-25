"""
Unit tests for the clingo.stats module.
"""

import pytest
from clingo.control import Control
from clingo.core import Library
from clingo.stats import (
    Stats,
    StatsArray,
    StatsArrayView,
    StatsMap,
    StatsMapView,
    StatsType,
    StatsView,
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

    def test_stats_view(self):
        """
        Test read-only stats API.
        """
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("1 { a; b; c; d } 1. #minimize { 1,a:a;1,b:b;1,c:c;1,d:d }.")
        ctl.ground()
        ctl.solve()
        stats = ctl.stats
        assert isinstance(stats, StatsView)
        assert isinstance(stats.map, StatsMapView)
        assert not isinstance(stats, Stats)
        assert not isinstance(stats.map, StatsMap)
        for x in stats:
            assert isinstance(x, str)
            if x == "solving":
                assert "solvers" in stats[x]
                assert "choices" in stats[x]["solvers"]
                break
        choices = stats["solving"]["solvers"]["choices"]
        assert isinstance(choices, StatsView)
        assert choices.value > 0
        for x in stats["solving"].map.keys():
            assert isinstance(x, str)
            if x == "solvers":
                break
        for x in stats["solving"]["solvers"].map.values():
            assert isinstance(x, StatsView)
            break
        for k, v in stats["solving"]["solvers"].map.items():
            assert isinstance(k, str)
            assert isinstance(v, StatsView)
            if k == "choices":
                assert v.value == choices.value
                break
        assert "costs" in stats["summary"]
        costs = stats["summary"]["costs"]
        assert isinstance(costs, StatsView)
        assert costs.nestify() == [1.0]
        expected = float(1.0)
        for x in costs:
            assert isinstance(x, StatsView)
            assert isinstance(x.value, float)
            assert x.value == expected
        assert isinstance(costs.array, StatsArrayView)
        assert not isinstance(costs.array, StatsArray)
        for x in costs.array:
            assert isinstance(x, StatsView)

    def test_stats(self):
        """
        Test mutable stats API.
        """
        ctl = Control(self.lib, ["0"])
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()

        def on_stats(step: Stats, accu: Stats):
            assert isinstance(step, Stats)
            assert isinstance(step, StatsView)
            assert isinstance(accu, Stats)
            assert isinstance(accu, StatsView)

            step.update({"map": {"val": 10.0, "arr": [1.0, 0.0, 3.0, {"nested": 1.0}]}})
            assert "map" in step
            assert isinstance(step["map"].map, StatsMap)
            assert sorted(list(step["map"].map.keys())) == ["arr", "val"]
            assert len(step["map"]["arr"]) == 4
            assert step["map"]["arr"][1].value == 0.0
            step["map"]["arr"][1].value = 2.0
            for k, v in step["map"].map.items():
                if k == "val":
                    v.value += v.value
                elif k == "arr":
                    assert isinstance(v.array, StatsArray)
                    for item in v.array:
                        if item.type == StatsType.Value:
                            item.value *= 2
                            with pytest.raises(TypeError):
                                for _ in item:
                                    pass
                        else:
                            assert item.type == StatsType.Map
                            with pytest.raises(TypeError):
                                item.value *= 2
                            for v in item.map.values():
                                v.value *= 4711.0

            assert step["map"]["val"].value == 20.0
            assert step["map"]["arr"].nestify() == [2.0, 4.0, 6.0, {"nested": 4711.0}]

            step.map["tested"] = 1.0
            accu.map["tested"] = 1.0

        assert ctl.solve(on_stats=on_stats).satisfiable
        stats = ctl.stats
        assert stats["user_step"]["tested"].value > 0
        assert stats["user_accu"]["tested"].value > 0

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
        assert isinstance(stats, StatsView)
        cpu = stats["summary"]["times"]["cpu"]
        assert isinstance(cpu, StatsView) and cpu.value >= 0.0

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
            step.update({"a": 10.0})
            step.update({"b": [10.0]})
            step.update({"c": {"x": 1.0}})
            accu.update({"Test": {"x": 10.0, "y": [1.0, 2.0, 3.0]}})
            accu.update({"Test": {"x": lambda x: x + 2}})
            accu.update({"Test": {"y": lambda x: [y + 1 for y in x]}})

        assert ctl.solve(on_model=mcb, on_stats=on_stats).satisfiable
        assert mcb.symbols == res

        stats = ctl.stats
        assert stats["user_step"].nestify() == {"a": 10.0, "b": [10.0], "c": {"x": 1.0}}
        assert stats["user_accu"].nestify() == {
            "Test": {"x": 12.0, "y": [2.0, 3.0, 4.0]}
        }
