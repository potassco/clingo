"""
Unit tests for error propagation across various modules.
"""

import gc
import re
import subprocess
import sys
from typing import Callable, Sequence

import pytest
from clingo.app import App, AppOptions, clingo_main
from clingo.backend import Observer
from clingo.control import Control
from clingo.core import Library
from clingo.propagate import (
    Assignment,
    CheckMode,
    PropagateControl,
    PropagateInit,
    Propagator,
)
from clingo.script import Script, register
from clingo.solve import Model
from clingo.symbol import Symbol


class ErrorApp(App):
    """
    Test throwing errors in apps.
    """

    _mode: str

    def __init__(self, mode: str) -> None:
        super().__init__("test")
        self._mode = mode

    def parse_option(self, value: str):
        """
        Test throwing errons in option parsers.
        """
        assert len(value) >= 0
        if "o" in self._mode:
            raise RuntimeError("option")

    def validate_options(self) -> None:
        """
        Test throwing errors in validate_options.
        """
        if "v" in self._mode:
            raise RuntimeError("validate")
        if "V" in self._mode:
            raise ValueError("Validate")

    def register_options(self, options: AppOptions) -> None:
        """
        Test throwing errors in register options.
        """
        if "r" in self._mode:
            raise RuntimeError("register")
        options.add("Test", "test", "Test option.", self.parse_option)

    def print_model(self, model: Model, default_printer: Callable[[], None]) -> None:
        """
        Test throwing errors in print model.
        """
        assert model
        if "p" in self._mode:
            raise RuntimeError("print")
        default_printer()

    def main(
        self,
        control: Control,
        files: Sequence[str],
    ) -> None:
        """
        Test throwing errors in main.
        """
        assert len(files) == 0
        if "m" in self._mode:
            raise RuntimeError("main")
        control.parse_string("a.")
        control.main()


class Prop(Propagator):
    """
    Test for errors in propagators.
    """

    throw: str

    def __init__(self, throw) -> None:
        super().__init__()
        self.throw = throw

    def init(self, assignment: Assignment, init: PropagateInit) -> None:
        """
        Test throwing errors in init.
        """
        assert assignment
        if "i" in self.throw:
            raise RuntimeError("prop: init")
        init.check_mode = CheckMode.Total
        init.add_watch(init.add_literal())

    def propagate(
        self,
        assignment: Assignment,
        control: PropagateControl,
        changes: Sequence[int],
    ) -> None:
        """
        Test throwing errors in propagate.
        """
        assert assignment
        assert changes
        assert control
        if "p" in self.throw:
            raise RuntimeError("prop: propagate")

    def decide(self, assignment: Assignment, fallback: int) -> int:
        """
        Test throwing errors in decide.
        """
        assert assignment
        if "d" in self.throw:
            raise RuntimeError("prop: decide")
        return fallback


class Obs(Observer):
    # pylint: disable=too-few-public-methods
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

    def call(
        self, lib: Library, name: str, arguments: Sequence[Symbol]
    ) -> Sequence[Symbol]:
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
        with pytest.raises(RuntimeError, match="fun called with 1"):
            ctl.ground(context=self)

    def test_error_script(self):
        """
        Test errors in scripts.
        """
        ctl = Control(self.lib)
        register(self.lib, MyScript())
        ctl.parse_string("p(@fun(1)). q.")
        with pytest.raises(RuntimeError, match="fun called with 1"):
            ctl.ground()
        with pytest.raises(RuntimeError, match="fun called with 1"):
            ctl.main()

    def test_obs(self):
        """
        Test errors in the observer.
        """
        obs = Obs()
        ctl = Control(self.lib)
        ctl.parse_string("{a}.")
        ctl.ground()
        with pytest.raises(RuntimeError, match=re.escape("rule: [1] :- []. [True]")):
            ctl.observe(obs)

    def test_error_on_model(self):
        """
        Test errors in on_model.
        """

        def on_model(m):
            raise RuntimeError(f"on_model: {m}")

        ctl = Control(self.lib)
        register(self.lib, MyScript())
        ctl.parse_string("a.")
        ctl.ground()
        with pytest.raises(RuntimeError, match="on_model: a"):
            ctl.solve(on_model=on_model)

    def test_error_propagate(self):
        """
        Test errors in propagators.
        """
        for throw, msg in [
            ("i", "prop: init"),
            ("p", "prop: propagate"),
            ("d", "prop: decide"),
        ]:
            ctl = Control(self.lib, ["0"])
            ctl.register_propagator(Prop(throw))
            with pytest.raises(RuntimeError, match=msg):
                ctl.main()

            ctl = Control(self.lib, ["0"])
            ctl.register_propagator(Prop(throw))
            with pytest.raises(RuntimeError, match=msg):
                ctl.solve()

    def run_app_test(self, mode, pattern: str):
        """
        Run the test app in a subprocess.
        """
        output = subprocess.run(
            [sys.executable, __file__, "test-error-app", mode],
            capture_output=True,
            text=True,
            check=False,
            timeout=10,
        ).stderr
        return bool(re.search(pattern, output, re.DOTALL))

    @pytest.mark.parametrize(
        "mode",
        [
            "main",
            "validate",
            "register",
            "print",
            "option",
        ],
    )
    def test_error_app(self, mode):
        """
        Test errors in propagators.
        """
        msg = f"mode `{mode}` failed"
        assert self.run_app_test(mode[0], f"RuntimeError: {mode}"), msg

    def test_error_validate(self):
        """
        Test special handling of ValueErrors in validate.
        """
        assert self.run_app_test("V", re.escape("*** ERROR: (test): Validate"))


def error_app_main(mode: str):
    """
    Start the test app in the given mode.
    """
    with Library() as lib:
        clingo_main(lib, ["--test", "value"], ErrorApp(mode))


if __name__ == "__main__" and len(sys.argv) == 3 and sys.argv[1] == "test-error-app":
    error_app_main(sys.argv[2])
