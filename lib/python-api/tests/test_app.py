"""
Test clingo's Application class.
"""

import json
import os
import re
import subprocess
import sys
from collections.abc import Sequence
from tempfile import NamedTemporaryFile
from typing import Any

import pytest
from clingo.app import App, AppOptions, Flag, clingo_main
from clingo.control import Control
from clingo.core import Library, MessageType
from util import MCB, needs_processes

# prefix of output lines reporting test events
PREFIX = "TEST: "


def emit(*event: Any) -> None:
    """
    Report a test event on standard output.
    """
    print(PREFIX + json.dumps(event), flush=True)


class AppTest(App):
    """
    Test application covering most of the Application related API.
    """

    _df: str
    program_name = "test"
    version = "1.2.3"
    message_limit = 17

    def __init__(self, df: str):
        super().__init__(AppTest.program_name, AppTest.version)
        self._df = df
        self._flag = Flag()

    def _parse_test(self, value) -> None:
        emit("parse", value)

    def register_options(self, options: AppOptions) -> None:
        """
        Register test options.
        """
        emit("register")
        group = "Clingo.Test"
        options.add(group, "test", "test description", self._parse_test)
        options.add_flag(group, "flag", "test description", self._flag)
        # Apply default value
        options.set_default_value("configuration", self._df)

    def validate_options(self) -> None:
        """
        Validate the options.
        """
        emit("validate")
        emit("flag", self._flag.value)

    def main(
        self,
        control: Control,
        files: Sequence[str],
    ) -> None:
        """
        Run the main loop.
        """
        emit("main")
        emit("configuration", control.config.configuration.value)
        control.parse_files(files)
        control.ground(control.parts)
        mcb = MCB()
        control.solve(on_model=mcb)
        emit("models", mcb.symbols)


class SingleShotApp(App):
    """
    Application with a main function that grounds and solves once.
    """

    def __init__(self):
        super().__init__("test")

    def main(self, control: Control, files: Sequence[str]) -> None:
        """
        Run the main loop.
        """
        control.parse_files(files)
        control.ground()
        control.solve()


def app_main(name: str, args: list[str]) -> int:
    """
    Run the given application; arguments before -- are passed to it.
    """
    sep = args.index("--")
    app_args, args = args[:sep], args[sep + 1 :]
    app: App = AppTest(*app_args) if name == "test" else SingleShotApp()

    def logger(code: MessageType, msg: str):
        emit("message", code.name, re.sub("^.*:(?=[0-9]+:)", "", msg))

    return int(clingo_main(Library(logger=logger), args, app))


AppResult = tuple[int, list[Any], list[str]]


def run_app(
    app: str, program: str, *args: str, app_args: Sequence[str] = ()
) -> AppResult:
    """
    Run a clingo application in a subprocess.

    Returns the exit code, the reported test events, and the remaining output
    lines.
    """
    with NamedTemporaryFile(mode="wt", delete=False) as fp:
        name = fp.name
        fp.write(program)
    try:
        res = subprocess.run(
            [sys.executable, __file__, "run-app", app, *app_args, "--", name, *args],
            capture_output=True,
            text=True,
            check=False,
            timeout=10,
        )
    finally:
        os.unlink(name)
    events, output = [], []
    for line in res.stdout.splitlines():
        if line.startswith(PREFIX):
            events.append(json.loads(line[len(PREFIX) :]))
        else:
            output.append(line)
    assert res.returncode >= 0, res.stderr
    return res.returncode, events, output


class TestApplication:
    # pylint: disable=too-few-public-methods
    """
    Tests for clingo's application class.
    """

    @needs_processes
    def test_app(self):
        """
        Test application.
        """
        with NamedTemporaryFile(mode="wt", delete=False) as fp:
            name = fp.name
        try:
            ret, events, _ = run_app(
                "test",
                "1 {a; b; c(1/0)}.",
                "--outf=3",
                "0",
                "--test=x",
                "--flag",
                app_args=[name],
            )
            assert ret == 30
            assert events == [
                ["register"],
                ["parse", "x"],
                ["validate"],
                ["flag", True],
                ["main"],
                ["configuration", name],
                [
                    "message",
                    MessageType.OperationUndefined.name,
                    "1:12-15: info: operation undefined:\n  1/0\n",
                ],
                ["models", [["a"], ["a", "b"], ["b"]]],
            ]
        finally:
            os.unlink(name)

    @needs_processes
    @pytest.mark.parametrize(
        "args, header",
        [([], "asp 2 0 0 incremental"), (["--single-shot"], "asp 2 0 0")],
    )
    def test_app_single_shot(self, args: list[str], header: str):
        """
        Test that a main function respects the --single-shot option.
        """
        _, _, output = run_app("single-shot", "a.", "--convert=aspif", *args)
        assert output[:1] == [header]


if __name__ == "__main__" and len(sys.argv) >= 3 and sys.argv[1] == "run-app":
    sys.exit(app_main(sys.argv[2], sys.argv[3:]))
