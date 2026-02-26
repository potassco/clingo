"""
Unit tests for the clingo.config module.
"""

import re
from textwrap import dedent

from clingo.control import Control
from clingo.core import Library
from util import MCB


class PluginConfig:
    """
    Simple test config class.
    """

    value: str | None
    array: list[str | None]
    group: str | None

    def __init__(self):
        self.value = "default value"
        self.array = ["default value"]
        self.group = "some group"

    def set_val(self, value: str) -> None:
        """
        Set the value of the value config.
        """
        self.value = value

    def get_val(self) -> str | None:
        """
        Get the value of the value config.
        """
        return self.value

    def set_arr_len(self, value: str) -> None:
        """
        Set the length of the config array.
        """
        while len(self.array) < int(value):
            self.array.append("default value")

    def get_arr_len(self) -> int:
        """
        Get the length of the config array.
        """
        return len(self.array)

    def set_arr_val(self, value: str, index: int | None = None) -> None:
        """
        Set the array value at index to value.
        """
        if index is None:
            index = 0
        self.array[index] = value

    def get_arr_val(self, index: int | None = None) -> str | None:
        """
        Get the value at index of the config array.
        """
        if index is None or index >= len(self.array):
            return "undefined"
        return self.array[index]

    def get_group(self) -> str | None:
        """
        Set the value of the group config.
        """
        return self.group

    def set_group(self, value: str | None) -> None:
        """
        Get the value of the group config.
        """
        self.group = value


class TestConfig:
    # pylint: disable=attribute-defined-outside-init
    """
    Tests for the config module.
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

    def test_config(self):
        """
        Test inspection of config objects.
        """
        ctl = Control(self.lib)
        cfg = ctl.config
        assert "solve" in cfg.attributes
        assert re.search("^solve:", str(ctl.config), re.MULTILINE) is not None
        assert "models" in cfg.solve.attributes
        assert cfg.solve.models.is_value
        assert not cfg.solve.models.is_sequence
        assert cfg.solve.models.description.startswith("Compute")
        assert "%A" not in cfg.solve.models.description
        cfg.solve.models.value = -1
        assert str(cfg.solve.models) == '"-1"'
        assert cfg.solve.models.value == "-1"
        assert "solver" in cfg.attributes
        assert cfg.solver.is_sequence
        assert not cfg.solver.is_value
        assert len(cfg.solver) >= 1
        assert "heuristic" in cfg.solver[0].attributes

    def test_solve(self):
        """
        Test if config updates apply to solving.
        """
        res = [["a"], ["b"], ["c"], ["d"]]
        ctl = Control(self.lib)
        ctl.parse_string("1 { a; b; c; d } 1.")
        ctl.ground()

        ctl.config.solve.models = 0
        mcb = MCB()
        assert ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == res

        ctl.config.solve.models = 2
        mcb = MCB()
        assert ctl.solve(on_model=mcb).satisfiable
        assert len(mcb.symbols) == 2

        ctl.config.solve.models = 0
        mcb = MCB()
        assert ctl.solve(on_model=mcb).satisfiable
        assert mcb.symbols == res

    def test_extend(self):
        """
        Test extending config objects.
        """
        ctl = Control(self.lib)
        cfg = ctl.config
        pc = PluginConfig()

        cfg.add_entry("plugin", "a plugin that needs configuration")
        cfg.add_entry("plugin.group", "a group", pc.get_group, pc.set_group)
        cfg.add_entry("plugin.group.val", "a value", pc.get_val, pc.set_val)
        cfg.add_entry(
            "plugin.array[]", "an array", set=pc.set_arr_len, size=pc.get_arr_len
        )
        cfg.add_entry(
            "plugin.array.val", "an array value", pc.get_arr_val, pc.set_arr_val
        )

        assert str(cfg.plugin.group.val.value) == "default value"
        cfg.plugin.group.val = "new value"
        assert str(cfg.plugin.group.value) == "some group"
        assert str(cfg.plugin.group.val) == '"new value"'
        assert cfg.plugin.group.val.value == "new value"
        cfg.plugin.group.value = "test"
        assert cfg.plugin.group.value == "test"

        assert len(cfg.plugin.array) == 1
        cfg.plugin.array.value = "2"
        assert len(cfg.plugin.array) == 2

        assert cfg.plugin.array[0].val.value == "default value"
        assert cfg.plugin.array.val.value == "undefined"
        cfg.plugin.array[1].val = "5"

        assert str(cfg.plugin) == dedent("""\
            array:
              - val: "default value"
              - val: "5"
            group: "test"
                   val: "new value"
            """.rstrip())
