"""
Unit tests for error propagation across various modules.
"""

import gc
from typing import Sequence

import pytest
from clingo.backend import Observer
from clingo.control import Control
from clingo.core import Library
from clingo.script import Script, register
from clingo.symbol import Symbol


class Obs(Observer):
    """
    Test Observer.
    """

    def rule(self, head: Sequence[int], body: Sequence[int], choice: bool) -> None:
        """
        Test rule.
        """
        raise RuntimeError(f"rule: {head} :- {body}. [{choice}]")


class MyScript(Script):
    """
    Example embedded script.
    """

    def __init__(self):
        super().__init__()
        self._scope = {}

    def name(self) -> str:
        """
        Get the name of the script.
        """
        return "myScript"

    def call(self, lib: Library, name: str, arguments: list[Symbol]) -> list[Symbol]:
        """
        Call the function with the given name and arguments.
        """
        assert lib
        raise RuntimeError(f"{name} called with {', '.join(map(str, arguments))}")

    def callable(self, name: str, arguments: int) -> bool:
        """
        Check if there is a function with the given name in the scope.
        """
        assert arguments >= 0
        assert name
        return name != "main"


class TestError:
    # pylint: disable=attribute-defined-outside-init
    """
    Unit tests for error propagation.
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
        # NOTE: ensure proper symbol cleanup by destroying held objects
        gc.collect()
        self._lib = None

    @property
    def lib(self) -> Library:
        """
        Get the library object.
        """
        assert self._lib is not None
        return self._lib

    def fun(self, num):
        """
        Test function that throws an error.
        """
        raise RuntimeError(f"fun called with {num}")

    def test_error_context(self):
        """
        Test errors in grounding callbacks.
        """

        ctl = Control(self.lib)
        ctl.parse_string("p(@fun(1)). q.")
        with pytest.raises(RuntimeError) as exc_info:
            ctl.ground(context=self)
        assert str(exc_info.value) == "fun called with 1"

    def test_error_script(self):
        """
        Test errors in scripts.
        """
        ctl = Control(self.lib)
        register(self.lib, MyScript())
        ctl.parse_string("p(@fun(1)). q.")
        with pytest.raises(RuntimeError) as exc_info:
            ctl.ground()
        assert str(exc_info.value) == "fun called with 1"
        with pytest.raises(RuntimeError) as exc_info:
            ctl.main()
        assert str(exc_info.value) == "fun called with 1"

    def test_obs(self):
        """
        Test errors in the observer.
        """
        obs = Obs()
        ctl = Control(self.lib)
        ctl.parse_string("{a}.")
        ctl.ground()
        with pytest.raises(RuntimeError) as exc_info:
            ctl.observe(obs)
        assert str(exc_info.value) == "rule: [1] :- []. [True]"

    # TODO:
    # - propagator??
    # - app?
